import { FFAudioPlayer, FFAudioWriter } from '@sj/ffmpeg';
import { abilityAccessCtrl, common } from '@kit.AbilityKit';
import { audio } from '@kit.AudioKit';
import { checkPermissionGrant, reqPermissionsFromUser } from './PermissionUtils';

@Entry
@Component
struct AudioWriterDemo {
  @State mStateStr: string = '实时转码-点击开始录制声音';
  @State mWroteBytesStr: string = '0';

  mAudioCapturer?: audio.AudioCapturer = undefined;
  mOutputPath = getContext(this).filesDir + '/audio_captured_output.mp3'; // 保存路径; 编码封装格式为 mp3;
  /**
   * 根据目标文件名自动选择合适的封装格式，将 PCM 音频数据编码后写入到目标文件。
   *
   * @note **注意:** 目标文件名必须包含正确的文件后缀（如 `.mp4`、`.aac`、`.wav`），以便自动推测编码封装格式。
   */
  mAudioWriter?: FFAudioWriter = undefined;

  // 录制完之后可以播放
  @State mPlayButtonVisibility: Visibility = Visibility.Hidden;
  mAudioPlayer?: FFAudioPlayer = undefined;

  aboutToDisappear(): void {
    if ( this.mAudioPlayer ) {
      this.mAudioPlayer.stop();
    }
  }

  build() {
    RelativeContainer() {
      Column({ space: 20 }) {
        Text(this.mStateStr)
          .id('实时转码')
          .fontSize(20)
          .fontWeight(FontWeight.Bold)
          .onClick(() => {
            console.info("onClick");
            this.startOrStopCapture();
          })

        Text(this.mWroteBytesStr)
          .id('已写入的字节数')
          .fontSize(20)
          .fontWeight(FontWeight.Bold)

        Text('播放')
          .id('播放按钮')
          .fontSize(20)
          .fontWeight(FontWeight.Bold)
          .visibility(this.mPlayButtonVisibility)
          .onClick(() => {
            console.info("onClick play");
            this.play();
          })

      }
      .alignRules({
        center: { anchor: '__container__', align: VerticalAlign.Center },
        middle: { anchor: '__container__', align: HorizontalAlign.Center }
      })
    }
    .height('100%')
    .width('100%')
  }

  async startOrStopCapture() {
    if ( this.mAudioCapturer ) {
      // 停止录制
      this.stopCapture();
      return;
    }

    // 音频录制权限申请
    if ( !this.checkPermissions() ) {
      return;
    }

    // 开始录制
    this.startCapture();
  }

  async checkPermissions() {
    const status = await checkPermissionGrant('ohos.permission.MICROPHONE');
    if ( status !== abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED ) {
      this.mStateStr = "请授予音频录制权限后重试";
      reqPermissionsFromUser(['ohos.permission.MICROPHONE'], getContext(this) as common.UIAbilityContext);
      return false;
    }
    return true;
  }

  async startCapture() {
    this.mPlayButtonVisibility = Visibility.Hidden;

    // 设置音频参数
    const audioStreamInfo: audio.AudioStreamInfo = {
      samplingRate: audio.AudioSamplingRate.SAMPLE_RATE_48000,
      channels: audio.AudioChannel.CHANNEL_2,
      sampleFormat: audio.AudioSampleFormat.SAMPLE_FORMAT_S16LE,
      encodingType: audio.AudioEncodingType.ENCODING_TYPE_RAW
    };

    // 创建 mAudioWriter
    this.mAudioWriter = new FFAudioWriter(this.mOutputPath);
    try {
      this.mAudioWriter.prepareSync(audioStreamInfo); // 初始化
    }
    catch (error) {
      this.stopCapture(error);
      return;
    }

    // 创建 mAudioCapturer
    let audioCapturerInfo: audio.AudioCapturerInfo = {
      source: audio.SourceType.SOURCE_TYPE_MIC,
      capturerFlags: 0
    };

    let audioCapturerOptions: audio.AudioCapturerOptions = {
      streamInfo: audioStreamInfo,
      capturerInfo: audioCapturerInfo
    };

    const audioCapturer = await audio.createAudioCapturer(audioCapturerOptions);
    let wroteBytes = 0;
    audioCapturer.on('readData', async (buffer) => {
      // 同步方式写入数据
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

      // 展示写入的数据字节数
      wroteBytes += buffer.byteLength;
      this.mWroteBytesStr = `${wroteBytes}`;
    });

    // 开始录制
    await audioCapturer.start();

    this.mAudioCapturer = audioCapturer;
    this.mStateStr = "已开始录制, 再次点击结束录制";
  }

  async stopCapture(error?: Error) {
    console.log(`停止录制: error: ${error?.message}`);

    // 停止录制
    await this.mAudioCapturer?.stop();
    await this.mAudioCapturer?.release();
    this.mAudioCapturer = undefined;

    // 关闭写入
    this.mAudioWriter?.closeSync();
    this.mAudioWriter = undefined;

    this.mStateStr = "已停止录制";
    // 如果没出错，显示播放按钮
    if ( !error ) this.mPlayButtonVisibility = Visibility.Visible;
  }

  // 播放
  play() {
    if ( !this.mAudioPlayer ) {
      this.mAudioPlayer = new FFAudioPlayer();
      this.mAudioPlayer.on("errorChange", (error) => {
        if ( error ) {
          console.log(`播放失败: error: ${error.message}`);
        }
      });
    }
    this.mAudioPlayer.stop();
    this.mAudioPlayer.url = this.mOutputPath;
    this.mAudioPlayer.play();
  }
}