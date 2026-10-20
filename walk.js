const Promise = require('bluebird');
const fs = require('fs-extra');
const path = require('path');

function walk(target,
              callback,
              options) {
  const opt = options || {};
  let allFileNames;

  return fs.readdir(target)
    .catch(err => (err.code === 'ENOENT')
      ? Promise.resolve([])
      : Promise.reject(err))
    .then((fileNames) => {
      allFileNames = fileNames;
      // bigint stats keep inode numbers above 2^53 exact for idStr
      const statOptions = { bigint: opt.details === true };
      return Promise.map(fileNames, (statPath) =>
                         Promise.resolve(fs.lstat(path.join(target, statPath), statOptions)).reflect(), { concurrency: 50 });
    }).then((res) => {
      // use the stats results to generate a list of paths of the directories
      // in the searched directory
      const subDirs = res
        .map((stat, idx) => stat.isFulfilled() && stat.value().isDirectory() ? path.join(target, allFileNames[idx]) : undefined)
        .filter(stat => stat !== undefined);
      callback(res.map((stat, idx) => stat.isFulfilled() ? {
	filePath: path.join(target, allFileNames[idx]),
	isDirectory: stat.value().isDirectory(),
	size: Number(stat.value().size),
	mtime: stat.value().mtime,
	...(opt.details ? {
	  id: Number(stat.value().ino),
	  idStr: stat.value().ino.toString(),
	  linkCount: Number(stat.value().nlink),
	} : {}),
      } : undefined).filter(iter => iter !== undefined));
      return Promise.mapSeries(subDirs, (subDir) =>
        walk(subDir, callback, options))
        .then(() => undefined);
    });
}

module.exports = {
  default: walk,
};

