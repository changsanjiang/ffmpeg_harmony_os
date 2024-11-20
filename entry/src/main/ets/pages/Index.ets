import { FFAbortController, FFmpeg, FFProgressMessageParser } from '@sj/ffmpeg';
import { fileIo } from '@kit.CoreFileKit';
import { photoAccessHelper } from '@kit.MediaLibraryKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { FileMgr } from '@sj/filemgr';

@Entry
@Component
struct Index {
  @State message: string = '请点击';
  @State show: boolean = false


  build() {
    RelativeContainer() {
      Text(this.message)
        .id('HelloWorld')
        .fontSize(20)
        .fontWeight(FontWeight.Bold)
        .alignRules({
          center: { anchor: '__container__', align: VerticalAlign.Center },
          middle: { anchor: '__container__', align: HorizontalAlign.Center }
        })
        .onClick(() => {
          console.info("onClick");
          // this.test1();
          this.test2();
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

  private inputPath = getContext(this).filesDir + '/input.mp4';

  test1() {
    // let commands = ["ffmpeg", "--help"];
    // let commands = ["ffprobe", "--help"];
    let cacheDir = getContext(this).cacheDir;
    let inputPath = this.inputPath; // 输入文件的路径, 可以是本地路径也可以是远程路径;
    let outputPath = cacheDir + `/output_1.mp4`;
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

  test2() {
    const photoSelectOptions = new photoAccessHelper.PhotoSelectOptions();
    photoSelectOptions.MIMEType = photoAccessHelper.PhotoViewMIMETypes.IMAGE_TYPE; // 过滤选择媒体文件类型为IMAGE
    photoSelectOptions.maxSelectNumber = 1; // 选择媒体文件的最大数目
    let uris: Array<string> = [];
    const photoViewPicker = new photoAccessHelper.PhotoViewPicker();
    photoViewPicker.select(photoSelectOptions).then((photoSelectResult: photoAccessHelper.PhotoSelectResult) => {
      uris = photoSelectResult.photoUris;
      console.info('photoViewPicker.select to file succeed and uris are:' + uris);

      let cacheDir = getContext(this).filesDir;
      let inputPath = this.inputPath; // 输入文件的路径, 可以是本地路径也可以是远程路径;
      let outputPath = cacheDir + `/output_2.mp4`;
      let commands = ["ffmpeg", "-i", inputPath, "-i", FileMgr.getPathFromUri(uris[0]) as string, "-filter_complex", "overlay=0:0", outputPath, "-y", "-v", "info"];

      FFmpeg.execute(commands, {
        logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
        progressCallback: (message: string) => {
          this.message = message;
          console.log(`[progress]${JSON.stringify(FFProgressMessageParser.parse(message))}`);
        },
      }).then(() => {
        this.message = "已完成:" + outputPath;
        console.info(`Execution succeeded with outputPath: ${outputPath}`);
      }).catch((error: Error) => {
        console.error(`Execution failed with error: ${error.message}`);
      });

    }).catch((err: BusinessError) => {
      console.error(`Invoke photoViewPicker.select failed, code is ${err.code}, message is ${err.message}`);
    })
  }
}