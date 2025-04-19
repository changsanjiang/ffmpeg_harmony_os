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
import { audio } from '@kit.AudioKit';

export namespace FFmpeg {
  export function setFontConfigPath(dir: string);

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

export enum FFPlayWhenReadyChangeReason {
  USER_REQUEST,
  // 恢复播放
  // 此分支表示临时失去焦点后被暂停的音频流此时可以继续播放，建议应用继续播放，切换至音频播放状态
  // 若应用此时不想继续播放，可以忽略此音频焦点事件，不进行处理即可
  // 继续播放，此处主动执行start()，以标识符变量started记录start()的执行结果
  AUDIO_INTERRUPT_RESUME,
  // 此分支表示系统已将音频流暂停（临时失去焦点），为保持状态一致，应用需切换至音频暂停状态
  // 临时失去焦点：待其他音频流释放音频焦点后，本音频流会收到resume对应的音频焦点事件，到时可自行继续播放
  AUDIO_INTERRUPT_PAUSE,
  // 此分支表示系统已将音频流停止（永久失去焦点），为保持状态一致，应用需切换至音频暂停状态
  // 永久失去焦点：后续不会再收到任何音频焦点事件，若想恢复播放，需要用户主动触发。
  AUDIO_INTERRUPT_STOP,
  // 耳机插拔
  OLD_DEVICE_UNAVAILABLE,
  // 播放结束
  PLAYBACK_ENDED,
}

export interface FFAudioPlaybackOptions {
  /** 毫秒, 非负数, 表示从音频的某个时间点开始播放; */
  readonly startTimePosition?: number;  

  /** 设置 http 请求;
   *
   *    可设置的选项请参考连接: https://ffmpeg.org/ffmpeg-protocols.html#http
   *
   * 例如:
   *\code
   *    // 仅设置 请求头
   *    player.setUrl(audio.url, { httpOptions: { "headers": "Authorization: Bearer your_token\r\nUser-Agent: MyFFmpegApp/1.0" }});
   *    // 仅设置 cookies
   *    player.setUrl(audio.url, { httpOptions: { "cookies": "abc=123; path=/;" }});
   *\endcode
   */
  readonly httpOptions?: Record<string, string>;


  /** 默认是 audio.StreamUsage.STREAM_USAGE_MUSIC; */
  readonly streamUsage?: audio.StreamUsage;
}

/**
 * @class FFAudioPlayer
 * @brief 音乐播放器。
 */
export class FFAudioPlayer {
  constructor();

  public get url(): string | undefined;

  public set url(newUrl: string | undefined);

  public setUrl(newUrl?: string, options?: FFAudioPlaybackOptions);

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
  
  /** https://developer.huawei.com/consumer/cn/doc/harmonyos-references/js-apis-audio#setdefaultoutputdevice12 */
  public setDefaultOutputDevice(deviceType: audio.DeviceType);

  public on(event: 'playWhenReadyChange', callback: (playWhenReady: boolean, reason: FFPlayWhenReadyChangeReason) => void);

  public on(event: 'durationChange', callback: (duration: number) => void);

  public on(event: 'currentTimeChange', callback: (currentTime: number) => void);

  public on(event: 'playableDurationChange', callback: (playableDuration: number) => void);

  public on(event: 'errorChange', callback: (error?: Error) => void);

  public off(event: 'playWhenReadyChange');

  public off(event: 'durationChange');

  public off(event: 'currentTimeChange');

  public off(event: 'playableDurationChange');

  public off(event: 'errorChange');
}

/**
 * @class FFAudioWriter
 * @brief 编码 + 封装。根据目标文件名自动选择合适的封装格式，将 PCM 音频数据编码后写入到目标文件。
 * 
 * @note **注意:** 目标文件名必须包含正确的文件后缀（如 `.mp4`、`.aac`、`.wav`），以便自动推测封装格式。
 */
export class FFAudioWriter {
  /**
   * 创建文件写入器实例。
   * 
   * @param filePath 输出文件路径（需包含正确的后缀，如 `.mp4`、`.aac`）。
   */
  constructor(filePath: string);

  /**
   * @brief 初始化。
   * @returns Promise<void>  成功时解析，失败时抛出错误。
   */
  public prepare(audioStreamInfo: audio.AudioStreamInfo): Promise<void>;
  public prepare(audioStreamInfo: audio.AudioStreamInfo, callback: (error?: Error) => void);
  public prepareSync(audioStreamInfo: audio.AudioStreamInfo);

  /**
   * @brief 将传入的 PCM 数据进行编码，并写入到目标文件中。
   * @param buffer PCM 数据（该数据应符合init初始化时的格式）。
   * @returns Promise<void> 成功时解析，失败时抛出错误。
   * @throws 若未初始化则抛出错误。
   */
  public write(pcmBuffer: ArrayBuffer): Promise<void>;
  public write(pcmBuffer: ArrayBuffer, callback: (error?: Error) => void);
  public writeSync(pcmBuffer: ArrayBuffer);

  /**
   * @brief 关闭写入器，并写入文件尾部，释放资源。当没有更多数据时调用关闭。
   * @returns Promise<void> 成功时解析，失败时抛出错误。
   * @throws 若未初始化则抛出错误。
   */
  public close(): Promise<void>;
  public close(callback: (error?: Error) => void);
  public closeSync();
}
