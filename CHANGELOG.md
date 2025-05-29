#### [v1.2.4] 2025/05/29
- 修复多轨音频 seek 后可能不播放的问题;

#### [v1.2.3] 2025/05/22
- 修复`FFAudioWriter`可能丢帧的问题;
- 重构了部分实现类;

#### [v1.2.2] 2025/04/28
- 音乐播放器增加配置`streamUsage`及`deviceType`;
- 音频播放输出格式固定为`44100, 2, s16le`;

#### [v1.2.1] 2025/04/28
- 音乐播放器增加配置`streamUsage`及`deviceType`;

#### [v1.2.0] 2025/04
- 修复`abort()`失效的问题;

#### [v1.1.9] 2025/04
- 修复执行命令时异步回调丢失或顺序错乱的问题;

#### [v1.1.8] 2025/03
- 修复音乐播放器可能出现死锁的问题;

#### [v1.1.7] 2025/03
- 添加`FFAudioWriter`, 用于将 PCM 音频数据进行编码封装并写入到目标文件;
- 增加支持`subtitles`滤镜;

#### [v1.1.6] 2025/03
- 移除 decThread, 音乐播放改为实时解码;

#### [v1.1.5] 2025/03
- FFAudioPlayer 增加`httpOptions`, 用于配置 http 请求, 例如设置 headers、cookies 等;
- FFAudioPlayer 增加断网重试;

#### [v1.1.4] 2025/02
- 开启`libopencore-amrnb`和`libopencore-amrwb`, 用来编码和解码`AMR-NB`和`AMR-WB`音频;

#### [v1.1.3] 2025/02
- 开启`librubberband`, 用于支持`rubberband`滤镜;
- 音乐播放器新增`startTimePosition`, 用于指定播放初始的位置;

#### [v1.1.2] 2025/02
- 添加`-fPIC`编译选项;
- 基于`ffmpeg + AudioRenderer`封装音乐播放器;
- 修复执行脚步命令时内存泄露的问题;
- 修复空指针访问导致的崩溃;

#### [v1.1.1] 2025/02
- 添加`-fPIC`编译选项;
- 修复 FFAbortController 可能崩溃的问题;

#### [v1.1.0] 2025/02
- 基于`ffmpeg + AudioRenderer`封装音乐播放器;
- 修复执行脚步命令时内存泄露的问题;

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