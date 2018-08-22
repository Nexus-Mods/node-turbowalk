if (process.platform === 'win32') {
  let path = require('path');

  const turbowalk = require('./build/Release/turbowalk').default;
  // const turbowalk = require('./build/Debug/turbowalk').default;
  const bluebird = require('bluebird');

  module.exports = {
    default: (walkPath, progress, options) => new bluebird((resolve, reject, onCancel) => {
      cancelled = false;
      // onCancel will be available if bluebird is configured to support cancelation
      // but will be undefined otherwise
      if (onCancel !== undefined) {
        onCancel(() => { cancelled = true; });
      }
      turbowalk(walkPath, (entries) => {
        if (progress !== undefined) {
          progress(entries);
        }
        return !cancelled;
      }, (err) =>
        err !== null ? reject(err) : resolve(err)
      , options || {});
    }),
  };
} else {
  module.exports = {
    default: require('./walk').default,
  };
}
