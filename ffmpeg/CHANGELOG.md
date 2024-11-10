#### [v1.0.2] 2024/11
- 日志等级改为全局配置, 可选值为`'quiet' | 'panic' | 'fatal' | 'error' | 'warning' | 'info' | 'verbose' | 'debug' | 'trace';`;
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