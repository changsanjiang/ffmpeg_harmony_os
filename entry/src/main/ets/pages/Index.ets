import { FFAbortController, FFmpeg } from '@sj/ffmpeg';

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
            // let cacheDir = getContext(this).cacheDir;
            // let inputPath = cacheDir + "/output.mp3";
            // let outputPath = cacheDir + `/output_${index}.aac`;
            // let commands = ["ffmpeg", "-i", inputPath, outputPath, "-y", "-v", "debug"];

            let cacheDir = getContext(this).cacheDir;
            let outputPath = cacheDir + `/output_${index}.mp4`;
            let commands = ["ffmpeg", "-i", "https://dh2.v.netease.com/2017/cg/fxtpty.mp4", outputPath, "-y"];

            let abortController = new FFAbortController();
            setTimeout(() => abortController.abort(), 4000);

            FFmpeg.execute(commands, {
              logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
              progressCallback: (message: string) => console.log(`[progress]${message}`),
              signal: abortController.signal
            }).then(() => {
              console.info("FFmpeg execution succeeded.");
            }).catch((error: Error) => {
              console.error(`FFmpeg execution failed with error: ${error.message}`);
            });
          }
        })
    }
    .height('100%')
    .width('100%')
  }
}