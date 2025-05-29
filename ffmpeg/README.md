# FFmpeg for HarmonyOS

1. 目前移植了 fftools/ffmpeg, fftools/ffprobe, 可以在App中执行 [ffmpeg](#执行-ffmpeg-命令) 及 [ffprobe](#执行-ffprobe-命令) 相关的脚本命令, 支持并发;
2. 基于 ffmpeg + AudioRenderer 封装的[音乐播放器](#音乐播放器);
3. [音频实时编码及封装器](#音频实时编码及封装器), 可实时将传入的 PCM 音频数据进行编码并封装到指定的目标文件中;
4. [注意事项](#注意事项);

#### 安装
```shell
ohpm i @sj/ffmpeg
```

#### 在项目中引用

请在需要依赖的模块找到 oh-package.json5 文件, 新增如下依赖, 执行同步后等待安装完成;
```json
{
  "dependencies": {
    "@sj/ffmpeg": "^1.2.4"
  }
}
```

#### 执行 ffmpeg 命令:

- 与在终端使用类似, 通过拼接 ffmpeg 命令执行脚本:
  ```typescript
  import { FFProgressMessageParser, FFmpeg } from '@sj/ffmpeg';
          
  let commands = ["ffmpeg", "-i", inputPath, outputPath, "-y"];            
  FFmpeg.execute(commands, {
    logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
    progressCallback: (message: string) => console.log(`[progress]${JSON.stringify(FFProgressMessageParser.parse(message))}`),
  }).then(() => {
    console.info("FFmpeg execution succeeded.");
  }).catch((error: Error) => {
    console.error(`FFmpeg execution failed with error: ${error.message}`);
  });
  ```
- 取消操作:
  ```typescript
  import { FFProgressMessageParser, FFAbortController, FFmpeg } from '@sj/ffmpeg';
  
  let abortController = new FFAbortController(); // 创建 abortController, 在需要时终止脚本执行; 
  setTimeout(() => abortController.abort(), 4000); // 模拟取消; 这里模拟取消, 延迟4s后取消操作; 
  
  let commands = ["ffmpeg", "-i", inputPath, outputPath, "-y"];
  FFmpeg.execute(commands, {
    logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
    progressCallback: (message: string) => console.log(`[progress]${JSON.stringify(FFProgressMessageParser.parse(message))}`),
    signal: abortController.signal //  传入 abortController.signal, 内部会监听取消信号;
  }).then(() => { 
    console.info("FFmpeg execution succeeded.");
  }).catch((error: Error) => {
    console.error(`FFmpeg execution failed with error: ${error.message}`);
  });
  ```

#### 执行 ffprobe 命令:

- 同样, 与在终端使用类似通过拼接 ffprobe 命令执行脚本, 如下获取输入音频文件的采样率、比特率等信息, 输出格式指定为 Json 格式:
  ```typescript
  import { FFmpeg } from '@sj/ffmpeg';
  
  let commands = ["ffprobe", "-v", "info", "-of", "json", "-show_entries", "stream=sample_rate,bit_rate", "-i", inputPath];
  let outputJson = "";
  FFmpeg.execute(commands, {
    logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
    outputCallback: (message: string) => { outputJson += message; },
  }).then(() => {
    console.info(`Execution succeeded with output: ${outputJson}`);
  }).catch((error: Error) => {
    console.error(`Execution failed with error: ${error.message}`);
  });
  ```
- 取消操作:
  ```typescript
  import { FFAbortController, FFmpeg } from '@sj/ffmpeg';
  
  let commands = ["ffprobe", "-v", "info", "-of", "json", "-show_entries", "stream=sample_rate,bit_rate", "-i", inputPath];
  
  let abortController = new FFAbortController(); // 创建 abortController, 在需要时终止脚本执行; 
  setTimeout(() => abortController.abort(), 4000); // 模拟取消; 这里模拟取消, 延迟4s后取消操作; 
  
  let outputJson = "";
  FFmpeg.execute(commands, {
    logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
    outputCallback: (message: string) => { outputJson += message; },
    signal: abortController.signal //  传入 abortController.signal, 内部会监听取消信号;
  }).then(() => {
    console.info(`Execution succeeded with output: ${outputJson}`);
  }).catch((error: Error) => {
    console.error(`Execution failed with error: ${error.message}`);
  });
  ```

#### 音乐播放器:

- 基础操作
  ```typescript
  import { FFAudioPlayer } from '@sj/ffmpeg';
  
  // 1. 创建音乐播放器
  let audioPlayer = new FFAudioPlayer();
  this.mAudioPlayer = audioPlayer; // 设置强引用, 避免被回收;
  
  // 2. 设置播放地址
  audioPlayer.url = "http://xxx";
  // 3. 播放
  audioPlayer.play();
  
  // 暂停播放
  // audioPlayer.pause();
  // 停止播放并清理当前资源
  // audioPlayer.stop();
  ```
- Seek 跳转到指定时间播放, 单位毫秒:
  ```typescript
  // 注意处于准备阶段时执行 Seek 无效:
  audioPlayer.seek(time_ms);
  ```
- 属性
  - `playWhenReady`: 标识播放器在准备阶段完成后, 是否立即进入播放阶段; 该属性目前为只读属性, 当调用 play 进行播放时会被设置为 true;
    - 当 playWhenReady 为 true 时，播放器在完成准备阶段后, 会自动开始播放媒体内容;
    - 当 playWhenReady 为 false 时，播放器在完成准备阶段后, 不会播放, 当调用 play 时才会开始播放;
  - `duration`: 音乐时长, 单位毫秒; 当播放器加载资源成功后刷新;
  - `currentTime`: 当前时间, 单位毫秒;
  - `playableDuration`: 缓冲时长, 单位毫秒;
  - `error`: 错误, 播放失败时有值;
  - `volume`: 设置音量, 取值范围 \[0-1\];
  - `speed`: 设置播放速度, 取值范围 \[0.25, 4.0\];
- 状态监听
  - `playWhenReadyChange`: playWhenReady 改变时回调; 触发改变的原因可能有以下几个场景:
    - 主动调用 play 或 pause;
    - 音频流失去焦点或恢复焦点时;
    - 耳机插拔;
    - 播放结束;
  - `durationChange`: 播放时长发生改变时回调;
  - `currentTimeChange`: 当前时间发生改变时回调;
  - `playableDurationChange`: 缓冲时长发生改变时回调;
  - `errorChange`: 播放出错时回调或调用 stop 被设置为 undefined 时回调;

#### 音频实时编码及封装器

FFAudioWriter 该类用于接收原始 PCM 数据, 会将其编码为指定的目标格式(如 AAC、MP3、WAV), 并封装到目标文件.

- 初始化
  ```typescript
    // 原始数据的 PCM 音频参数
    const audioStreamInfo: audio.AudioStreamInfo = {
      samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_48000,
      channels: audio.AudioChannel.CHANNEL_2,
      sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
      encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
    };
  
    // 保存路径; 编码封装格式为 mp3;
    // FFAudioWriter 会根据目标文件名自动选择合适的封装格式, 所以目标文件名必须包含正确的文件后缀(如 `.mp4`、`.aac`、`.wav`), 以便自动推测编码封装格式.
    const outputPath = getContext(this).filesDir + '/output.mp3'; 
  
    // 创建 FFAudioWriter
    this.mAudioWriter = new FFAudioWriter(outputPath);
    // 同步方式初始化, 也可以调用异步方式初始化.
    try {
      this.mAudioWriter.prepareSync(audioStreamInfo); 
    }
    catch (error) {
      this.stopCapture(error);
      return;
    }
  ```

- 实时编码、封装
  ```typescript
    // 这里以音频录制的回调举例, 当有新的 PCM 数据时, 实时传递给 FFAudioWriter 即可;
    audioCapturer.on('readData', async (buffer) => {
      // 同步方式写入数据
      // 传入的 PCM 数据将被编码为目标格式的数据并被封装写入到目标文件中.
      try {
        this.mAudioWriter?.writeSync(buffer);
      }
      catch (error) {
        this.stopCapture(error);
      }
    
      // Promise 方式写入数据
      // await this.mAudioWriter?.write(buffer).catch((error: Error) => {
      //   this.stopCapture(error);
      // });
    
      // 异步方式写入数据
      // this.mAudioWriter?.write(buffer, (error) => {
      //   if ( error ) {
      //     this.stopCapture(error);
      //   }
      // });
    });
  ```

- 关闭
  ```typescript
      // 没有更多数据时, 请调用关闭以写入文件尾部并释放相关资源。
      this.mAudioWriter?.closeSync();
      this.mAudioWriter = undefined;
  ```

#### 注意事项

该库包含了模拟器 x86_64 架构, 请在打包发布时排除该架构以减少包体积, 如下在工程级目录下`build-profile.json5`中, 找到相应的产品配置, 添加排除项;

```json
{
  "app": {
    "products": [
      {
        "name": "xxx",
        "nativeLib": {
          "filter": {
            "excludes": [
              "**/x86_64/*.so"
            ]
          }
        }
      }
    ]
  }
}
```