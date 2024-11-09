export namespace FFCommandInspector {
  export function inspect(commands: string[]) {
    if (commands.includes('-v') || commands.includes('-loglevel')) {
      console.warn("[FFmpeg Warning]日志等级是全局配置的，建议不要手动设置 '-v' 选项。在并发执行多个 ffmpeg 命令时，日志等级的设置会影响其他任务的日志输出。");
    }
  }
}
