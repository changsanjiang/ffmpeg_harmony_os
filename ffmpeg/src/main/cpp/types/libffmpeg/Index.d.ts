export const setPrintHandler: (printHandler?: (msg: string) => void) => void;
export const prepare: (executionId: number) => void;
export const execute: (executionId: number, commands: string[], logCallback?: (level: number, msg: string) => void, printHandler?: (msg: string) => void) => number;
export const cancel: (executionId: number) => void;