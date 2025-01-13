/**
 This file is part of @sj/ffmpeg.

 @sj/ffmpeg is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 @sj/ffmpeg is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with @sj/ffmpeg. If not, see <http://www.gnu.org/licenses/>.
 * */
import ffmpeg from 'libffmpeg.so';
import { ArkTSUtils, taskpool } from '@kit.ArkTS';
import { FFAbortSignal } from './FFAbortController';
import BuildProfile from '../../../../BuildProfile';

namespace FFmpeg {
  export interface Options {
    logCallback?: (level: number, msg: string) => void
    /** 这个回调是 ffmpeg 进度消息的回调, 请在执行 ffmpeg 命令时设置; */
    progressCallback?: (msg: string) => void
    /** 这个回调是 ffprobe 输出消息的回调, 请在执行 ffprobe 命令时设置; */
    outputCallback?: (msg: string) => void
    signal?: FFAbortSignal
  }

  /**
   * 执行脚本命令;
   *
   * 例如:
   * - 执行 ffmpeg 命令: FFmpeg.execute(["ffmpeg", "-i", inputPath, outputPath, "-y"]);
   * - 执行 ffprobe 命令: FFmpeg.execute(["ffprobe", "-v", "info", "-of", "json", "-show_entries", "stream=sample_rate,bit_rate", "-i", inputPath]);
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

      const logCallback = options?.logCallback;
      const progressCallback = options?.progressCallback;
      const outputCallback = options?.outputCallback;
      const task = new taskpool.Task(performTask, new ExecutionParams(
        executionId,
        commands,
        logCallback != undefined,
        progressCallback != undefined,
        outputCallback != undefined,
      ));
      if ( logCallback || progressCallback || outputCallback ) {
        task.onReceiveData((type: number, ...args: Object[]) => {
          switch (type) {
            case 0: {
              if ( logCallback ) logCallback(args[0] as number, args[1] as string);
            }
              break;
            case 1: {
              if ( progressCallback ) progressCallback(args[0] as string);
            }
              break;
            case 2: {
              if ( outputCallback ) outputCallback(args[0] as string);
            }
              break;
          }
        });
      }
      const result = await taskpool.execute(task) as number;
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
    enabledLogCallback: boolean;
    enabledProgressCallback: boolean;
    enabledOutputCallback: boolean;
    constructor(executionId: number, commands: string[], enabledLogCallback: boolean, enabledProgressCallback: boolean, enabledOutputCallback: boolean) {
      this.executionId = executionId;
      this.commands = commands;
      this.enabledLogCallback = enabledLogCallback;
      this.enabledProgressCallback = enabledProgressCallback;
      this.enabledOutputCallback = enabledOutputCallback;
    }
  }

  @Concurrent
  function performTask(params: ExecutionParams): number {
    const result = ffmpeg.execute(
      params.executionId,
      params.commands,
      params.enabledLogCallback ? (level: number, msg: string) => taskpool.Task.sendData(0, level, msg) : undefined,
      params.enabledProgressCallback ? (msg: string) => taskpool.Task.sendData(1, msg) : undefined,
      params.enabledOutputCallback ? (msg: string) => taskpool.Task.sendData(2, msg) : undefined,
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