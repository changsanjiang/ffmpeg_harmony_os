import { FFAudioPlayer, FFPlayWhenReadyChangeReason } from '@sj/ffmpeg';
import { FileMgr } from '@sj/filemgr';
import { MCMediaCache } from '@sj/mediacache';

interface AudioItem {
  readonly name: string;
  readonly url: string;
}

@Entry
@Component
struct AudioPlayerDemo {
  private mAudios: AudioItem[] = [
    {
      name: "world",
      url: getContext(this).filesDir + '/world.mp3'
    },
    {
      name: "power",
      url: getContext(this).filesDir + '/power.mp3'
    },
    {
      name: "mp4",
      url: getContext(this).filesDir + '/input.mp4'
    },
    {
      name: "HUAWEI Mate 70 Pro+",
      url: "https://consumer.huawei.com/content/dam/huawei-cbg-site/cn/mkt/pdp/phones/mate70-pro-plus/video/design/design-intro-popup.mp4"
    }
  ];

  private mIndex = -1;
  private mAudioPlayer?: FFAudioPlayer = undefined;

  private mDuration = 0;
  @State
  private mAudioName: string = "";
  @State
  private mCurrentTimeStr: string = "--:--";
  @State
  private mDurationTimeStr: string = "--:--";
  @State
  private mSliderProgress: number = 0;
  private mSliderState = SliderChangeMode.End;
  @State
  private mPlaybackStateStr: string = "点击播放";

  async aboutToAppear(): Promise<void> {
    // 把 rawfile 文件复制到 app 目录下
    FileMgr.copyRawFileToPath(getContext(this), 'world.mp3', this.mAudios[0].url, { checkExistence: true, returnIfExists: true });
    FileMgr.copyRawFileToPath(getContext(this), 'power.mp3', this.mAudios[1].url, { checkExistence: true, returnIfExists: true });
    FileMgr.copyRawFileToPath(getContext(this), 'input.mp4', this.mAudios[2].url, { checkExistence: true, returnIfExists: true });

    const proxyUrl = await MCMediaCache.proxy("https://consumer.huawei.com/content/dam/huawei-cbg-site/cn/mkt/pdp/phones/mate70-pro-plus/video/design/design-intro-popup.mp4");
    this.mAudios.push({
      name: "proxyUrl HUAWEI Mate 70 Pro+",
      url: proxyUrl
    })
  }

  aboutToDisappear(): void {
    if ( this.mAudioPlayer ) {
      this.mAudioPlayer.stop();
    }
  }

  build() {
    RelativeContainer() {
      Column({ space: 20 }) {

        // 播放控制
        Row({ space: 32 }) {
          Text('上一首')
            .height(32)
            .fontSize(18)
            .fontWeight(FontWeight.Medium)
            .onClick(() => {
              console.log('播放上一首');
              this.playPrevious();
            })

          Text(this.mPlaybackStateStr)
            .id('播放或暂停')
            .height(32)
            .fontSize(18)
            .fontWeight(FontWeight.Medium)
            .onClick(() => {
              console.log('播放或暂停');
              this.playOrPause();
            })

          Text('下一首')
            .height(32)
            .fontSize(18)
            .fontWeight(FontWeight.Medium)
            .onClick(() => {
              console.log('播放下一首');
              this.playNext();
            })
        }

        // 显示进度
        Flex({ direction: FlexDirection.Row, justifyContent: FlexAlign.Center, alignItems: ItemAlign.Center }) {
          Text(this.mCurrentTimeStr)
            .id('当前时间')
            .fontSize(14)
            .textAlign(TextAlign.Center)
            .width(60)

          Slider({
            value: this.mSliderProgress,
            min: 0,
            max: 100
          }).flexGrow(1).onChange((value: number, mode: SliderChangeMode) => {
            if ( this.mSliderState != mode ) {
              console.log(`AAAA: mSliderStateChange: ${mode}`);
              this.mSliderState = mode;
              if ( this.mDuration > 0 && this.mAudioPlayer && mode === SliderChangeMode.End ) {
                const time = value / 100 * this.mDuration;
                console.log(`AAAA: seek ${time}`);
                this.mAudioPlayer.seek(time);
              }
            }
          })

          Text(this.mDurationTimeStr)
            .id('音乐时长')
            .fontSize(14)
            .textAlign(TextAlign.Center)
            .width(60)
        }
        .width('100%')
        .padding({ left: 24, right: 24})

        Text(this.mAudioName)
          .id('音乐名称')
          .fontSize(14)
          .textAlign(TextAlign.Center)
          .alignRules({
            center: { anchor: '__container__', align: VerticalAlign.Center },
            middle: { anchor: '__container__', align: HorizontalAlign.Center }
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

  private playPrevious() {
    if ( this.mIndex === -1 ) {
      this.playAt(0);
    }
    else if ( this.mIndex > 0 ) {
      this.playAt(this.mIndex - 1);
    }
    else {
      this.playAt(this.mAudios.length - 1);
    }
  }

  private playNext() {
    if ( this.mIndex === -1 ) {
      this.playAt(0);
    }
    else {
      this.playAt((this.mIndex + 1) % this.mAudios.length);
    }
  }

  private playOrPause() {
    if ( this.mIndex === -1 ) {
      this.playAt(0);
    }
    else {
      let player = this.getPlayer();
      player.playWhenReady ? player.pause() : player.play();
    }
  }

  private playAt(index: number) {
    this.mIndex = index;

    let audio = this.mAudios[index];
    let player = this.getPlayer();
    player.stop();
    player.url = audio.url;
    player.play();

    this.mAudioName = audio.name;
  }

  private getPlayer(): FFAudioPlayer {
    let player = this.mAudioPlayer;
    if ( player ) {
      return player;
    }
    player = new FFAudioPlayer();
    player.on('playWhenReadyChange', (playWhenReady, reason) => {
      // 音乐播放完毕后播放下一首
      if ( reason === FFPlayWhenReadyChangeReason.PLAYBACK_ENDED ) {
        this.playNext();
      }

      this.mPlaybackStateStr = playWhenReady ? "点击暂停" : "点击播放";
    });
    player.on('durationChange', (durationMs) => {
      this.mDuration = durationMs;
      this.mDurationTimeStr = this.formatTime(durationMs, durationMs);
      this.mCurrentTimeStr = this.formatTime(0, durationMs);
    });
    player.on('currentTimeChange', (currentTimeMs) => {
      this.mCurrentTimeStr = this.formatTime(currentTimeMs, this.mDuration);
      if ( this.mSliderState === SliderChangeMode.End ) {
        this.mSliderProgress = this.mDuration > 0 ? (currentTimeMs / this.mDuration) * 100 : 0;
      }
    });
    player.on('playableDurationChange', (playableDuration) => {
      console.log(`bufferTime: ${playableDuration}`);
    });
    player.on('errorChange', (error) => {
      if ( error ) {
        console.log(`error: ${error?.message}`);
        this.mAudioPlayer?.stop();
      }
    });

    this.mAudioPlayer = player;
    return player;
  }

  private formatTime(timeMs: number, durationMs: number): string {
    if ( durationMs === 0 ) {
      return "--:--";
    }

    const timeSecs = Math.round(timeMs / 1000);
    const totalDurationSecs = Math.round(durationMs / 1000);

    const MIN_SECS = 60;
    const HOUR_SECS = 60 * MIN_SECS;

    const hours = Math.floor(timeSecs / HOUR_SECS);
    const minutes = Math.floor((timeSecs % HOUR_SECS) / MIN_SECS);
    const seconds = timeSecs % MIN_SECS;

    const formattedHours = String(hours).padStart(2, '0');
    const formattedMinutes = String(minutes).padStart(2, '0');
    const formattedSeconds = String(seconds).padStart(2, '0');

    return totalDurationSecs < HOUR_SECS
      ? `${formattedMinutes}:${formattedSeconds}`
      : `${formattedHours}:${formattedMinutes}:${formattedSeconds}`;
  }
}