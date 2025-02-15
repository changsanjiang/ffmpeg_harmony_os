#### [v1.1.0] 2025/02
- 新增音乐播放器, 使用 ffmpeg 解码后通过 AudioRenderer 进行播放;
- 修复内存泄露的问题;

#### [v1.0.9] 2025/01
- 开启`libx265`, 用于支持`x265`编码;

#### [v1.0.8] 2024/12
- 修复`cannot find record '&@sj/ffmpeg/xxx'`;

#### [v1.0.7] 2024/12
- 开启`libfreetype`, 用于支持`drawtext`文字水印;

#### [v1.0.6] 2024/11
- 增加`x86_64`架构;

#### [v1.0.5] 2024/11
- 为`ffmpeg_hw.c`中的 static 变量添加 _Thread_local;
- 添加gpl声明;

#### [v1.0.4] 2024/11
- 增加 lame, libaom, libogg,libvorbis, opus, x264 等编码器;
- 由于开启了gpl选项, 所以许可证修改为 GPL-3.0;

#### [v1.0.3] 2024/11
- 移植 fftools/ffprobe; 可以执行 ffprobe 相关的脚本命令了;
  - FFmpeg.Options 新增`outputCallback`, 用于 ffprobe 输出消息时的回调, 只有在执行 ffprobe 命令时才会回调;
  - 通过执行`FFmpeg.execute(["ffprobe", "--help"], ...)`获取帮助信息;

#### [v1.0.2] 2024/11
- 新增 progressCallback, 回调值结构如下:
  - frame=61
  - fps=0.00
  - stream_0_0_q=1.6
  - bitrate=   0.1kbits/s
  - total_size=44
  - out_time_us=2688000
  - out_time_ms=2688000
  - out_time=00:00:02.688000
  - dup_frames=0
  - drop_frames=0
  - speed=4.98x
  - progress=continue

#### [v1.0.1] 2024/11
- 修改混淆规则, 使导出类保持类名不变;

#### [v1.0.0] 2024/11
- 移植 fftools/ffmpeg;