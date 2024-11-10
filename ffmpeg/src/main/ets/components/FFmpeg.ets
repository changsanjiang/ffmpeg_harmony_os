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
    FFCommandInspector.inspect(commands);
    try {
      const executionId = await generateExecutionId();
      ffmpeg.prepare(executionId);
      options?.signal?.addEventListener('aborted', () => ffmpeg.cancel(executionId));
      const task = new taskpool.Task(performTask, new ExecutionParams(executionId, commands));
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

  /** 日志等级;
   *
   * - quiet：禁用所有日志输出，不显示任何信息。
   * - panic：仅显示 panic 级别的错误，通常是严重的内部错误，导致程序无法继续执行。
   * - fatal：显示 fatal 错误，表示严重错误，导致程序退出。
   * - error：显示错误信息，程序可能会继续执行，但需要处理错误。
   * - warning：显示警告信息，表示潜在问题，但不会影响程序继续执行。
   * - info：显示一般信息，包含较为简洁的程序输出。
   * - verbose：显示详细信息，通常包含更多的调试信息。
   * - debug：显示最详细的调试信息，用于开发和调试过程。
   * - trace：显示最详细的跟踪信息，记录每个细节和操作，通常用于极其详细的调试和分析。
   * */
  export type LogLevel = 'quiet' | 'panic' | 'fatal' | 'error' | 'warning' | 'info' | 'verbose' | 'debug' | 'trace';
  /** 设置日志等级; */
  export function setLogLevel(name: LogLevel) {
    ffmpeg.setLogLevel(name);
  }
  /** 默认为 'info'; */
  export function getLogLevel(): LogLevel {
    return ffmpeg.getLogLevel() as LogLevel;
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
    constructor(executionId: number, commands: string[]) {
      this.executionId = executionId;
      this.commands = commands;
    }
  }

  @Concurrent
  function performTask(params: ExecutionParams): number {
    const result = ffmpeg.execute(
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
  const _libInit = BuildProfile.DEBUG ? new Init() : undefined;
}

export default FFmpeg;