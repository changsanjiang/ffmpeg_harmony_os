import { ArkTSUtils } from "@kit.ArkTS";

export class FFAbortController {
  private mSignal = new FFAbortSignal_();

  get signal(): FFAbortSignal {
    return this.mSignal;
  }

  abort(reason?: Error): Promise<void> {
    return this.mSignal.abort(reason);
  }
}

export interface FFAbortSignal {
  readonly aborted: Promise<boolean>
  readonly reason: Promise<Error | undefined>
  addEventListener(event: 'aborted', callback: () => void): Promise<void>;
}

class FFAbortSignal_ implements FFAbortSignal {
  private mLock: ArkTSUtils.locks.AsyncLock = new ArkTSUtils.locks.AsyncLock();
  private mAborted = false
  private mReason: Error | undefined = undefined
  private mListeners = new Array<Function>();

  get aborted(): Promise<boolean> {
    return this.mLock.lockAsync(() => this.mAborted);
  }

  get reason(): Promise<Error | undefined>  {
    return this.mLock.lockAsync(() => this.mReason);
  }

  addEventListener(_: "aborted", callback: () => void): Promise<void> {
    return this.mLock.lockAsync(() => {
      if ( !this.mAborted ) this.mListeners.push(callback);
      else callback();
    });
  }

  abort(reason?: Error): Promise<void> {
    return this.mLock.lockAsync(() => {
      if ( !this.mAborted ) {
        this.mAborted = true;
        if ( reason ) {
          this.mReason = reason;
        }
        else {
          let abortErr = new Error('The operation was aborted.');
          abortErr.name = 'ABORT_ERR';
          this.mReason = abortErr;
        }
        this.mListeners.forEach((listener) => { listener(); });
        this.mListeners.length = 0;
      }
    });
  }
}