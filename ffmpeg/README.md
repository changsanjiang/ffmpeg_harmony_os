# FFmpeg for HarmonyOS

目前移植了 fftools/ffmpeg, 可以在App中执行 ffmpeg 相关的脚本命令;

#### 安装

请在需要依赖的模块找到 oh-package.json5 文件, 新增如下依赖, 执行同步后等待安装完成;
```json
{
  "dependencies": {
    "@sj/ffmpeg": "^1.0.1"
  }
}
```

#### 使用示例:

- 与在终端使用类似, 通过拼接 ffmpeg 命令执行脚本:
    ```ts
    import { FFProgressMessageParser, FFmpeg } from '@sj/ffmpeg';
            
    let commands = ["ffmpeg", "-i", inputPath, outputPath, "-y", "-v", "debug"];            
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
    
    let commands = ["ffmpeg", "-i", inputPath, outputPath, "-y", "-v", "debug"];
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

#### 设置日志等级

- 日志等级为全局配置, 可选值如下:
  - quiet：禁用所有日志输出，不显示任何信息。
  - panic：仅显示 panic 级别的错误，通常是严重的内部错误，导致程序无法继续执行。
  - fatal：显示 fatal 错误，表示严重错误，导致程序退出。
  - error：显示错误信息，程序可能会继续执行，但需要处理错误。
  - warning：显示警告信息，表示潜在问题，但不会影响程序继续执行。
  - info：显示一般信息，包含较为简洁的程序输出。
  - verbose：显示详细信息，通常包含更多的调试信息。
  - debug：显示最详细的调试信息，用于开发和调试过程。
  - trace：显示最详细的跟踪信息，记录每个细节和操作，通常用于极其详细的调试和分析。

- 通过`setLogLevel`设置日志等级:
  ```ts
  import { FFmpeg } from '@sj/ffmpeg';
  
  FFmpeg.setLogLevel('info');
  ``` 

- 通过`getLogLevel`获取当前日志等级, 默认值 'info':
  ```ts
  import { FFmpeg } from '@sj/ffmpeg';
  
  let level = FFmpeg.getLogLevel('info');
  ```