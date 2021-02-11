let bluebird;
try {
  bluebird = require('bluebird');
  Promise = bluebird;
} catch (err) { }

let walk;

if (process.platform === 'win32') {
  const path = require('path');
  const winapi = require('winapi-bindings');

  const isDriveLetter = (input) =>
    (input.length === 2) && input.endsWith(':');

  walk = (walkPath, progress, options, setCancel) => {
    if (walkPath === undefined) {
      throw new Error('expected at least one parameter');
    }
    const stackErr = new Error();
    return new Promise((resolve, reject) => {
      let cancelled = false;
      if (setCancel !== undefined) {
        setCancel(() => { cancelled = true; });
      }

      const basePath = isDriveLetter(walkPath) ? walkPath : path.normalize(walkPath);
      const onProgress = (entries) => {
        if (progress !== undefined) {
          progress(entries);
        }
        return !cancelled;
      };
      const onFinish = (err) => {
        if (err !== null) {
          err.stack = [].concat(err.stack.split('\n')[0], stackErr.stack.split('\n').slice(1)).join('\n');
          reject(err);
        } else {
          resolve();
        }
      };
      winapi.WalkDir(basePath, onProgress, options || {}, onFinish);
    });
  };
} else {
  walk = (target, callback, options) =>
    Promise.resolve(require('./walk').default(target, callback, options));
}

module.exports = {
  __esModule: true,
  default: walk,
}
