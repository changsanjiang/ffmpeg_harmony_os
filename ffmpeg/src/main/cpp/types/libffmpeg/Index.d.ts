export const setPrintHandler: (printHandler?: (msg: string) => void) => void;
export const setLogLevel: (name: string) => void;
export const getLogLevel: () => string;

export const prepare: (executionId: number) => void;
export const execute: (
  executionId: number,
  commands: string[],
  logCallback?: (level: number, msg: string) => void,
  progressCallback?: (msg: string) => void,
) => number;
export const cancel: (executionId: number) => void;