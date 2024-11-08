# FFmpeg for HarmonyOS 

目前移植了 fftools/ffmpeg, 可以在App中执行 ffmpeg 相关的脚本命令;

#### 使用示例:

- 与在终端使用类似, 通过拼接 ffmpeg 命令执行脚本:
    ```ts
    import { FFmpeg } from '@app/ffmpeg';
            
    let commands = ["ffmpeg", "-i", inputPath, outputPath, "-y", "-v", "debug"];            
    FFmpeg.execute(commands, (logLevel: number, logMessage: string) => {
      console.log(`${logMessage}`);
    }).then(() => {
      console.info("FFmpeg execution succeeded.");
    }).catch((error: Error) => {
      console.error(`FFmpeg execution failed with error: ${error.message}`);
    });
    ```
- 取消操作: 
    ```ts
    import { FFAbortController, FFmpeg } from '@app/ffmpeg';
    let abortController = new FFAbortController(); // 创建 abortController, 在需要时终止脚本执行; 
    setTimeout(() => abortController.abort(), 4000); // 模拟取消; 这里模拟取消, 延迟4s后取消操作; 
    
    FFmpeg.execute(commands, (logLevel: number, logMessage: string) => {
      console.log(`${logMessage}`);
    }, abortController.signal).then(() => { //  传入 abortController.signal, 内部会监听取消信号;
      console.info("FFmpeg execution succeeded.");
    }).catch((error: Error) => {
      console.error(`FFmpeg execution failed with error: ${error.message}`);
    });
    ```
