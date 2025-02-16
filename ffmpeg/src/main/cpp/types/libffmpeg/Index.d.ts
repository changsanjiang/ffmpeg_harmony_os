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
export namespace FFmpeg {
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
  export function execute(commands: string[], options?: Options): Promise<void>;
}

export class FFAbortController {
  get signal(): FFAbortSignal;
  abort(reason?: Error): void;
}

export class FFAbortSignal {
  get aborted(): boolean;
  get reason(): Error | undefined;
  addEventListener(event: 'aborted', callback: (error: Error) => void): void;
}

export class FFAudioPlayer {
  constructor();

  public get url(): string | undefined;

  public set url(newUrl: string | undefined);

  /** [0.0, 1.0]; */
  public get volume(): number;
  public set volume(newVolume: number);

  /** [0.25, 4.0] */
  public get speed(): number;
  public set speed(newSpeed: number);

  public get playWhenReady(): boolean;

  public get duration(): number;

  public get currentTime(): number;

  public get playableDuration(): number;

  public get error(): Error | undefined;

  public prepare();

  public play();

  public pause();

  /** 停止播放, 当前的播放资源及状态将会被清除和重置; error 也将被重置为undefined; */
  public stop();

  public seek(time_ms: number);

  public on(event: 'playWhenReadyChange', callback: (playWhenReady: boolean) => void);

  public on(event: 'durationChange', callback: (duration: number) => void);

  public on(event: 'currentTimeChange', callback: (currentTime: number) => void);

  public on(event: 'playableDurationChange', callback: (playableDuration: number) => void);

  public on(event: 'errorChange', callback: (error: Error) => void);

  public off(event: 'playWhenReadyChange');

  public off(event: 'durationChange');

  public off(event: 'currentTimeChange');

  public off(event: 'playableDurationChange');

  public off(event: 'errorChange');
}