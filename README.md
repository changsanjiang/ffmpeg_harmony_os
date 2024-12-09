# FFmpeg for HarmonyOS

目前移植了 fftools/ffmpeg, fftools/ffprobe, 可以在App中执行 ffmpeg 及 ffprobe 相关的脚本命令;

支持并发;

#### 安装
```shell
ohpm i @sj/ffmpeg
```

#### 在项目中引用

请在需要依赖的模块找到 oh-package.json5 文件, 新增如下依赖, 执行同步后等待安装完成;
```json
{
  "dependencies": {
    "@sj/ffmpeg": "^1.0.7"
  }
}
```

#### 执行 ffmpeg 命令:

- 与在终端使用类似, 通过拼接 ffmpeg 命令执行脚本:
    ```ts
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
    ```ts
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
    ```ts
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
    ```ts
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