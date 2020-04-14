declare module "turbowalk" {
  export interface IEntry {
    // full path to the file
    filePath: string;
    // whether this is a directory
    isDirectory: boolean;
    // whether this is a reparse point (symbolic link or junction point)
    isReparsePoint: boolean
    // size in bytes
    size: number;
    // last modification time (as seconds since the unix epoch)
    mtime: number;
    // if the terminators option was set, this indicates whether an entry is such a terminator
    isTerminator?: boolean;
    // unique id of the file (could have collisions due to the limited range of the number type)
    id?: number;
    // stringified file id (should be unique)
    idStr?: string;
    // number of (hard-)links to the data
    linkCount?: number;
  }

  export interface IWalkOptions {
    // add a fake entry to the output list for each directory at the point where its
    // done. This can be useful to simplify parsing the output (default: false)
    terminators?: boolean;
    // add linkCount and id attributes to the output. This makes the walk slower (default: false)
    details?: boolean;
    // minimum number of entries per call to the progress callback (except for the last
    // invocation of course). Higher numbers should increase performance but also memory usage
    // and responsiveness for all kinds of progress indicators (default: 1024)
    threshold?: number;
    // recurse into subdirectories (default: true)
    recurse?: boolean;
    // ignore files with the "hidden" attribute (default: true)
    skipHidden?: boolean;
    // don't recurse into links (junctions), otherwise we may end in an endless loop (default: true)
    // Note: Before 2.0.0 the behavior of this flag wasn't what's documented here. This previously
    //   left out all links from the result, both directories (junctions) and files (symbolic links).
    //   Now it simply doesn't recurse into junction points but still lists them in the output
    skipLinks?: boolean;
    // skip past directories that aren't accessible without producing an error (default: true)
    skipInaccessible?: boolean;
  }

  function turbowalk(basePath: string, progress: (entries: IEntry[]) => void,
                     options?: IWalkOptions): Bluebird<void>;

  export default turbowalk;
}
