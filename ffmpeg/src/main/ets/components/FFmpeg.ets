import ffmpeg from 'libffmpeg.so';
import { ArkTSUtils, taskpool } from '@kit.ArkTS';
import { FFAbortSignal } from './FFAbortController';
import BuildProfile from '../../../../BuildProfile';
import { FFCommandInspector } from './FFCommandInspector';

namespace FFmpeg {
  interface Options {
    logCallback?: (level: number, msg: string) => void
    progressCallback?: (msg: string) => void
    signal?: FFAbortSignal
  }

  /**
   * 执行脚本命令;
   *
   * 例如: FFmpeg.execute(["ffmpeg", "-i", inputPath, outputPath, "-y", "-v", "info"]);
   *
   * 通过`-v`指定日志级别, 每个级别对应不同的详细信息量:
   *
   * - quiet：禁用所有日志输出，不显示任何信息。
   * - panic：仅显示 panic 级别的错误，通常是严重的内部错误，导致程序无法继续执行。
   * - fatal：显示 fatal 错误，表示严重错误，导致程序退出。
   * - error：显示错误信息，程序可能会继续执行，但需要处理错误。
   * - warning：显示警告信息，表示潜在问题，但不会影响程序继续执行。
   * - info：显示一般信息，包含较为简洁的程序输出。
   * - verbose：显示详细信息，通常包含更多的调试信息。
   * - debug：显示最详细的调试信息，用于开发和调试过程。
   *
   * \code
   * // 可通过 abortController 来取消操作;
   * let abortController = new FFAbortController();
   * let abortSignal = abortController.signal;
   * // 模拟取消操作;
   * setTimeout(() => abortController.abort(), 4000); // 4秒后取消操作;
   * // 开始执行命令;
   * FFmpeg.execute(commands, (logLevel: number, logMessage: string) => console.log(`${logMessage}`), abortSignal).then(() => {
   *  console.info("FFmpeg execution succeeded.");
   * }).catch((error: Error) => {
   *  console.error(`FFmpeg execution failed with error: ${error.message}`);
   *  });
   * \endcode
   * */
  export async function execute(commands: string[], options?: Options): Promise<void> {
    FFCommandInspector.inspect(commands);
    try {
      let executionId = await generateExecutionId();
      ffmpeg.prepare(executionId);
      options?.signal?.addEventListener('aborted', () => ffmpeg.cancel(executionId));
      let task = new taskpool.Task(performTask, new ExecutionParams(executionId, commands));
      let logCallback = options?.logCallback;
      let progressCallback = options?.progressCallback;
      if ( logCallback || progressCallback ) {
        task.onReceiveData((type: number, ...args: Object[]) => {
          switch (type) {
            case 0: {
              if ( logCallback ) logCallback(args[0] as number, args[1] as string);
              break;
            }
            case 1: {
              if ( progressCallback ) progressCallback(args[0] as string);
              break;
            }
          }
        });
      }
      let result = await taskpool.execute(task) as number;
      return result == 0 ? Promise.resolve() : Promise.reject(createError(result));
    }
    catch (e) {
      return Promise.reject(e);
    }
  }

  function createError(code: number): Error {
    switch (code) {
      case 255: {
        let err = new Error('Operation cancelled by user.');
        err.name = 'FF_CANCELLED_ERR';
        return err;
      }
      default: {
        let err = new Error(`Operation failed with code ${code}. Please check logs for potential issues and troubleshooting details.`);
        err.name = 'FF_GENERIC_ERR';
        return err;
      }
    }
  }

  let nextExecutionId = 0;
  let lock: ArkTSUtils.locks.AsyncLock = new ArkTSUtils.locks.AsyncLock();

  async function generateExecutionId(): Promise<number> {
    return lock.lockAsync(() => ++nextExecutionId);
  }

  class ExecutionParams {
    executionId: number;
    commands: string[];
    constructor(executionId: number, commands: string[]) {
      this.executionId = executionId;
      this.commands = commands;
    }
  }

  @Concurrent
  function performTask(params: ExecutionParams): number {
    let result = ffmpeg.execute(
      params.executionId,
      params.commands,
      (level: number, msg: string) => taskpool.Task.sendData(0, level, msg),
      (msg: string) => taskpool.Task.sendData(1, msg),
    );
    return result;
  }

  class Init {
    constructor() {
      ffmpeg.setPrintHandler((msg) => console.log(msg));
    }
  }
  let _libInit = BuildProfile.DEBUG ? new Init() : undefined;
}

export default FFmpeg;