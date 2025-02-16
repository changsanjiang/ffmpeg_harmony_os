import { FFmpeg, FFProgressMessageParser } from '@sj/ffmpeg';
import { photoAccessHelper } from '@kit.MediaLibraryKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { FileMgr } from '@sj/filemgr';
import { MCLogModule, MCMediaCache } from '@sj/mediacache';

@Entry
@Component
struct Index {
  @State message: string = '请点击';
  @State show: boolean = false

  private mp4Path = getContext(this).filesDir + '/input.mp4';
  private gifPath = getContext(this).filesDir + '/loading.gif';

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
          // this.test2();
          // this.test3();
          // this.test4();
          // this.test5();
          this.test6();
        })
    }
    .height('100%')
    .width('100%')
  }

  async aboutToAppear(): Promise<void> {

    await MCMediaCache.prepare();
    MCMediaCache.setLogEnabled(true);
    MCMediaCache.setLogWhiteModules([MCLogModule.MCHttpConnectionHandler, MCLogModule.MCHttpResponse])

    FileMgr.copyRawFileToPath(getContext(this), 'input.mp4', this.mp4Path, { checkExistence: true, returnIfExists: true });
    FileMgr.copyRawFileToPath(getContext(this), 'loading.gif', this.gifPath, { checkExistence: true, returnIfExists: true });
  }

  test1() {
    // let commands = ["ffmpeg", "--help"];
    // let commands = ["ffprobe", "--help"];
    let cacheDir = getContext(this).cacheDir;
    let inputPath = this.mp4Path; // 输入文件的路径, 可以是本地路径也可以是远程路径;
    let outputPath = cacheDir + `/output_1.mp4`;
    let commands = ["ffmpeg", "-i", inputPath, outputPath, "-y", "-v", "info"];

    console.log(`[Commands] ${commands.join(" ")}`);

    FFmpeg.execute(commands, {
      logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
      progressCallback: (message: string) => console.log(`[progress]${JSON.stringify(FFProgressMessageParser.parse(message))}`),
    }).then(() => {
      console.info(`Execution succeeded with outputPath: ${outputPath}`);
    }).catch((error: Error) => {
      console.error(`Execution failed with error: ${error.message}`);
    });
  }

  // 从相册选择一张图片，合并到视频中
  test2() {
    const photoSelectOptions = new photoAccessHelper.PhotoSelectOptions();
    photoSelectOptions.MIMEType = photoAccessHelper.PhotoViewMIMETypes.IMAGE_TYPE; // 过滤选择媒体文件类型为IMAGE
    photoSelectOptions.maxSelectNumber = 1; // 选择媒体文件的最大数目
    let uris: Array<string> = [];
    const photoViewPicker = new photoAccessHelper.PhotoViewPicker();
    photoViewPicker.select(photoSelectOptions).then((photoSelectResult: photoAccessHelper.PhotoSelectResult) => {
      uris = photoSelectResult.photoUris;
      console.info('photoViewPicker.select to file succeed and uris are:' + uris);

      let cacheDir = getContext(this).cacheDir;
      let inputPath = this.mp4Path; // 输入文件的路径, 可以是本地路径也可以是远程路径;
      let outputPath = cacheDir + `/output_2.mp4`;
      let commands = ["ffmpeg", "-i", inputPath, "-i", FileMgr.getPathFromUri(uris[0]) as string, "-filter_complex", "overlay=0:0", outputPath, "-y", "-v", "info"];

      console.log(`[Commands] ${commands.join(" ")}`);

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

  // 将gif合并到视频中
  test3() {
    let cacheDir = getContext(this).cacheDir;
    let mp4Path = this.mp4Path; // 输入文件的路径, 可以是本地路径也可以是远程路径;
    let gitPath = this.gifPath;
    let outputPath = cacheDir + `/output_3.mp4`;
    let commands = ["ffmpeg", "-i", mp4Path, "-ignore_loop", "0", "-i", gitPath, "-filter_complex", "overlay=0:0", "-shortest", outputPath, "-y", "-v", "info"];

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
  }

  // 绘制文字水印
  test4() {
    let cacheDir = getContext(this).cacheDir;
    let mp4Path = this.mp4Path; // 输入文件的路径, 可以是本地路径也可以是远程路径;
    let outputPath = cacheDir + `/output_4.mp4`;
    let commands = ["ffmpeg", "-i", mp4Path, "-vf", "drawtext=text='文字水印':fontfile=/system/fonts/HarmonyOS_Sans_SC.ttf:fontcolor=white:fontsize=24:x=10:y=10", "-codec:a", "copy", outputPath, "-y", "-v", "info"];
    console.log(`[Commands] ${commands.join(" ")}`);

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
  }

  test5() {
    let commands = ["ffprobe", "-v", "info", "-of", "json", "-show_entries", "stream=sample_rate,bit_rate,duration", "-i", this.mp4Path];
    let output = "";
    FFmpeg.execute(commands, {
      logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
      outputCallback: (msg: string) => {
        output += msg;
        console.log(`[output]${msg}`);
      }
    }).then(() => {
      console.info(`Execution succeeded with output: ${output}`);
      this.message = output;
    }).catch((error: Error) => {
      console.error(`Execution failed with error: ${error.message}`);
    });
  }

  test6() {
    // let commands = ["ffmpeg", "--help"];
    // let commands = ["ffprobe", "--help"];
    let cacheDir = getContext(this).cacheDir;
    let inputPath = this.mp4Path; // 输入文件的路径, 可以是本地路径也可以是远程路径;
    let outputPath = cacheDir + `/output_6.mp4`;
    let commands = ["ffmpeg", "-i", inputPath, "-c:a", "copy", "-c:v", "libx265", outputPath, "-y", "-v", "info"];

    console.log(`[Commands] ${commands.join(" ")}`);

    FFmpeg.execute(commands, {
      logCallback: (logLevel: number, logMessage: string) => console.log(`[${logLevel}]${logMessage}`),
      progressCallback: (message: string) => {
        this.message = message;
        console.log(`[progress]${JSON.stringify(FFProgressMessageParser.parse(message))}`);
      },
    }).then(() => {
      this.message = `Execution succeeded with outputPath: ${outputPath}`;
      console.info(this.message);
    }).catch((error: Error) => {
      console.error(`Execution failed with error: ${error.message}`);
    });
  }
}