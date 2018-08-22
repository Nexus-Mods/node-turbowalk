#pragma once

#include <string>
#include <vector>
#include <functional>

#include "optional.h"

static uint32_t FILE_ATTRIBUTE_TERMINATOR = 0x80000000;

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
