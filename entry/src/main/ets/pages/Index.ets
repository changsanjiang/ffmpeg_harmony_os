import { FFAbortController, FFmpeg, FFProgressMessageParser } from '@sj/ffmpeg';
import { fileIo } from '@kit.CoreFileKit';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';
  @State show: boolean = false

  private inputPath = getContext(this).filesDir + '/input.mp4';

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
            let inputPath = this.inputPath; // 输入文件的路径, 可以是本地路径也可以是远程路径;
            let outputPath = cacheDir + `/output_${index}.mp4`;
            let commands = ["ffmpeg", "-i", inputPath, outputPath, "-y", "-v", "info"];

            FFmpeg.execute(commands, {
              logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
              progressCallback: (message: string) => console.log(`[progress]${JSON.stringify(FFProgressMessageParser.parse(message))}`),
            }).then(() => {
              console.info(`Execution succeeded with outputPath: ${outputPath}`);
            }).catch((error: Error) => {
              console.error(`Execution failed with error: ${error.message}`);
            });
          }
        })
    }
    .height('100%')
    .width('100%')
  }

  aboutToAppear(): void {
    // 把示例mp4复制到文件目录下;
    fileIo.access(this.inputPath, fileIo.AccessModeType.EXIST).then((exists) => {
      if ( !exists ) {
        getContext(this).resourceManager.getRawFileContent('input.mp4', (_err, value) => {
          let myBuffer: ArrayBufferLike = value.buffer;
          let file = fileIo.openSync(this.inputPath, fileIo.OpenMode.READ_WRITE | fileIo.OpenMode.CREATE);
          fileIo.writeSync(file.fd, myBuffer);
          fileIo.closeSync(file);
        });
      }
    });
  }
}