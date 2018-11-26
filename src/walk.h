#pragma once

#include <string>
#include <vector>
#include <functional>

#include "optional.h"

static uint32_t FILE_ATTRIBUTE_TERMINATOR = 0x80000000;

class ApiError : public std::exception {
public:
  ApiError(uint32_t code, const std::string &func, const std::string &path)
    : std::exception()
    , mCode(code)
    , mFunc(func)
    , mPath(path)
    {
    }

  uint32_t code() const {
    return mCode;
  }

  const char *func() const {
    return mFunc.c_str();
  }

  const char *path() const {
    return mPath.c_str();
  }

private:
  uint32_t mCode;
  std::string mFunc;
  std::string mPath;
};

struct Entry {
  std::wstring filePath;
  uint32_t attributes;
  uint64_t size;
  uint32_t mtime;
  Optional<uint32_t> linkCount;
  Optional<uint64_t> id;
  Optional<std::string> idStr;
};

struct WalkOptions {
  Optional<uint32_t> threshold;
  Optional<bool> terminators;
  Optional<bool> details;
  Optional<bool> recurse;
  Optional<bool> skipHidden;
  Optional<bool> skipLinks;
};

void walk(const std::wstring &basePath,
          std::function<bool(const std::vector<Entry> &results)> cb,
          const WalkOptions &options);
