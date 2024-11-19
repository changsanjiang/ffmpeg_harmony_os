import filemgr from 'libfilemgr.so';

namespace FileMgr {
  export function getPathFromUri(uri: string): string | undefined {
    return filemgr.getPathFromUri(uri);
  }
}

export default FileMgr;