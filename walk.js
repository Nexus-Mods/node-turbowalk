const Promise = require('bluebird');
const fs = require('fs-extra-promise');

function walk(target,
              callback,
              options) {
  const opt = options || {};
  let allFileNames;

  return fs.readdirAsync(target)
    .catch(err => (err.code === 'ENOENT')
      ? Promise.resolve([])
      : Promise.reject(err))
    .then((fileNames) => {
      allFileNames = fileNames;
      return Promise.map(fileNames, (statPath) =>
                         fs.lstatAsync(path.join(target, statPath)).reflect(), { concurrency: 50 });
    }).then((res) => {
      // use the stats results to generate a list of paths of the directories
      // in the searched directory
      const subDirs = res
        .map((stat, idx) => stat.isFulfilled() && stat.value().isDirectory() ? path.join(target, allFileNames[idx]) : undefined)
        .filter(stat => stat !== undefined);
      callback(res.map((stat, idx) => stat.isFulfilled() ? {
	filePath: path.join(target, allFileNames[idx]),
	isDirectory: stat.value().isDirectory(),
	size: stat.value().size,
	mtime: stat.value().mtime,
      } : undefined).filter(iter => iter !== undefined));
      return Promise.mapSeries(subDirs, (subDir) =>
        walk(subDir, callback, options));
    });
}

module.exports = {
  default: walk,
};

