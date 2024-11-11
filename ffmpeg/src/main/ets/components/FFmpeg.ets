import ffmpeg from 'libffmpeg.so';
import { ArkTSUtils, taskpool } from '@kit.ArkTS';
import { FFAbortSignal } from './FFAbortController';
import BuildProfile from '../../../../BuildProfile';

namespace FFmpeg {
  export interface Options {
    logCallback?: (level: number, msg: string) => void
    progressCallback?: (msg: string) => void
    signal?: FFAbortSignal
  }

  /**
   * 执行脚本命令;
   *
   * 例如: FFmpeg.execute(["ffmpeg", "-i", inputPath, outputPath, "-y"]);
   *
   * \code
   * let abortController = new FFAbortController(); // 如执行过程中需要取消, 可创建 abortController 来控制取消操作;
   * setTimeout(() => abortController.abort(), 4000); // 模拟取消; 这里模拟取消, 延迟4s后取消操作;
   * // 开始执行命令;
   * FFmpeg.execute(commands, {
   *  logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
   *  progressCallback: (message: string) => console.log(`[progress]${JSON.stringify(FFProgressMessageParser.parse(message))}`),
   *  signal: abortController.signal //  传入 abortController.signal, 内部会监听取消信号;
   * }).then(() => {
   *  console.info("FFmpeg execution succeeded.");
   * }).catch((error: Error) => {
   *  console.error(`FFmpeg execution failed with error: ${error.message}`);
   * });
   * \endcode
   * */
  export async function execute(commands: string[], options?: Options): Promise<void> {
    try {
      const executionId = await generateExecutionId();
      ffmpeg.prepare(executionId);
      options?.signal?.addEventListener('aborted', () => ffmpeg.cancel(executionId));
      const task = new taskpool.Task(performTask, new ExecutionParams(executionId, commands, options?.logCallback != undefined, options?.progressCallback != undefined));
      const logCallback = options?.logCallback;
      const progressCallback = options?.progressCallback;
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
        const err = new Error('Operation cancelled by user.');
        err.name = 'FF_CANCELLED_ERR';
        return err;
      }
      default: {
        const err = new Error(`Operation failed with code ${code}. Please check logs for potential issues and troubleshooting details.`);
        err.name = 'FF_GENERIC_ERR';
        return err;
      }
    }
  }

  let nextExecutionId = 0;
  const lock: ArkTSUtils.locks.AsyncLock = new ArkTSUtils.locks.AsyncLock();

  async function generateExecutionId(): Promise<number> {
    return lock.lockAsync(() => ++nextExecutionId);
  }

  class ExecutionParams {
    executionId: number;
    commands: string[];
    enabledLogReception: boolean;
    enabledProgressReception: boolean;
    constructor(executionId: number, commands: string[], enabledLogReception: boolean, enabledProgressReception: boolean) {
      this.executionId = executionId;
      this.commands = commands;
      this.enabledLogReception = enabledLogReception;
      this.enabledProgressReception = enabledProgressReception;
    }
  }

  @Concurrent
  function performTask(params: ExecutionParams): number {
    const result = ffmpeg.execute(
      params.executionId,
      params.commands,
      params.enabledLogReception ? (level: number, msg: string) => taskpool.Task.sendData(0, level, msg) : undefined,
      params.enabledProgressReception ? (msg: string) => taskpool.Task.sendData(1, msg) : undefined,
    );
    return result;
  }

  class Init {
    constructor() {
      ffmpeg.setPrintHandler((msg) => console.log(msg));
    }
  }
  const _libInit = BuildProfile.DEBUG ? new Init() : undefined;
}

export default FFmpeg;