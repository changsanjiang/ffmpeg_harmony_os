export const setPrintHandler: (printHandler?: (msg: string) => void) => void;

export const prepare: (executionId: number) => void;
export const execute: (
  executionId: number,
  commands: string[],
  logCallback?: (level: number, msg: string) => void,
  progressCallback?: (msg: string) => void, // progressCallback for ffmpeg; 注意: 这个是 ffmpeg 进度消息的回调, 只有在执行 ffmpeg 命令时才会回调;
  outputCallback?: (msg: string) => void, // outputCallback for ffprobe; 注意: 这个是 ffprobe 输出消息的回调, 只有在执行 ffprobe 命令时才会回调;
) => number;
export const cancel: (executionId: number) => void;