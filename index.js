if (process.platform === 'win32') {
  let path = require('path');

  const turbowalk = require('./build/Release/turbowalk').default;
  // const turbowalk = require('./build/Debug/turbowalk').default;
  const bluebird = require('bluebird');

  const isDriveLetter = (input) =>
    (input.length === 2) && input.endsWith(':')

  module.exports = {
    default: (walkPath, progress, options) => {
      const stackErr = new Error();
      return new bluebird((resolve, reject, onCancel) => {
        cancelled = false;
        // onCancel will be available if bluebird is configured to support cancelation
        // but will be undefined otherwise
        if (onCancel !== undefined) {
          onCancel(() => { cancelled = true; });
        }
        turbowalk(isDriveLetter(walkPath) ? walkPath : path.normalize(walkPath), (entries) => {
          if (progress !== undefined) {
            progress(entries);
          }
          return !cancelled;
        }, (err) => {
          if (err !== null) {
            err.stack = [].concat(err.stack.split('\n')[0], stackErr.stack.split('\n').slice(1)).join('\n');
            const code = {
              2: 'ENOTFOUND',
              3: 'ENOTFOUND',
              23: 'EIO',
              32: 'EBUSY',
              55: 'ENOTCONN',
              1392: 'EIO',
            }[err.errno];
            if (code !== undefined) {
              err.code = code;
            }
            reject(err);
          } else {
            resolve();
          }
        }
          , options || {});
      });
    },
  };
} else {
  module.exports = {
    default: require('./walk').default,
  };
}
