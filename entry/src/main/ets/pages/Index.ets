import { FFAbortController, FFmpeg, FFProgressMessageParser } from '@sj/ffmpeg';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';
  @State show: boolean = false

  build() {
    RelativeContainer() {
      Text(this.message)
        .id('HelloWorld')
        .fontSize(50)
        .fontWeight(FontWeight.Bold)
        .alignRules({
          center: { anchor: '__container__', align: VerticalAlign.Center },
          middle: { anchor: '__container__', align: HorizontalAlign.Center }
        })
        .onClick(() => {
          console.info("onClick");
          for (let index = 0; index < 1 ; index++) {
            // let commands = ["ffmpeg", "--help"];
            // let commands = ["ffprobe", "--help"];
            let cacheDir = getContext(this).cacheDir;
            let outputPath = cacheDir + `/output_${index}.mp4`;
            let commands = ["ffmpeg", "-i", "https://dh2.v.netease.com/2017/cg/fxtpty.mp4", outputPath, "-y", "-v", "info"];
            let abortController = new FFAbortController(); // 创建 abortController, 在需要时终止脚本执行;
            setTimeout(() => abortController.abort(), 4000); // 模拟取消; 这里模拟取消, 延迟4s后取消操作;

            FFmpeg.execute(commands, {
              logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
              progressCallback: (message: string) => console.log(`[progress]${JSON.stringify(FFProgressMessageParser.parse(message))}`),
              signal: abortController.signal //  传入 abortController.signal, 内部会监听取消信号;
            }).then(() => {
              console.info(`Execution succeeded`);
            }).catch((error: Error) => {
              console.error(`Execution failed with error: ${error.message}`);
            });
          }
        })
    }
    .height('100%')
    .width('100%')
  }
}