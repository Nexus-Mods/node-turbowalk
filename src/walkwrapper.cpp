#include <nan.h>
#include <iostream>
#include <map>
#include <atomic>

#include "walk.h"
#include "string_cast.h"

using namespace Nan;
using namespace v8;

template <typename T>
T cast(const v8::Local<v8::Value> &input);

template <>
bool cast(const v8::Local<v8::Value> &input) {
  return input->BooleanValue();
}

template <>
int cast(const v8::Local<v8::Value> &input) {
  return input->Int32Value();
}

static std::wstring strerror(DWORD errorno) {
  wchar_t *errmsg = nullptr;

  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorno,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&errmsg, 0, nullptr);

  if (errmsg) {
    for (int i = (wcslen(errmsg) - 1);
         (i >= 0) && ((errmsg[i] == '\n') || (errmsg[i] == '\r'));
         --i) {
      errmsg[i] = '\0';
    }

    return errmsg;
  }
  else {
    return L"Unknown error";
  }
}

inline Local<Value> WinApiException(
  DWORD lastError
  , const char *func = nullptr
  , const char* path = nullptr) {

  std::wstring errStr = strerror(lastError);
  std::string err = toMB(errStr.c_str(), CodePage::UTF8, errStr.size());
  return node::WinapiErrnoException(Isolate::GetCurrent(), lastError, func, err.c_str(), path);
}

Local<String> operator "" _n(const char *input, size_t) {
  return Nan::New(input).ToLocalChecked();
}

std::wstring toWC(const Local<Value> &input) {
  if (input->IsNullOrUndefined()) {
    return std::wstring();
  }
  String::Utf8Value temp(input);
  return toWC(*temp, CodePage::UTF8, temp.length());
}

v8::Local<v8::Object> convert(const Entry &input) {
  v8::Local<v8::Object> result = Nan::New<v8::Object>();
  result->Set("filePath"_n,
    Nan::New(toMB(input.filePath.c_str(), CodePage::UTF8, input.filePath.size())).ToLocalChecked());
  result->Set("isDirectory"_n, Nan::New((input.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0));
  result->Set("size"_n, Nan::New(static_cast<double>(input.size)));
  result->Set("mtime"_n, Nan::New(input.mtime));
  result->Set("isTerminator"_n, Nan::New((input.attributes & FILE_ATTRIBUTE_TERMINATOR) != 0));

  if (input.linkCount.isSet()) {
    result->Set("linkCount"_n, Nan::New(*input.linkCount));
  }
  if (input.id.isSet()) {
    result->Set("id"_n, Nan::New(static_cast<double>(*input.id)));
    result->Set("idStr"_n, Nan::New(*input.idStr).ToLocalChecked());
  }

  return result;
}

v8::Local<v8::Array> convert(const Entry *input, size_t count) {
  v8::Local<v8::Array> result = Nan::New<v8::Array>();
  for (size_t i = 0; i < count; ++i) {
    result->Set(i, convert(input[i]));
  }
  return result;
}

class WalkWorker : public AsyncProgressQueueWorker<Entry> {
 public:
  WalkWorker(Callback *callback, Callback *progress, const std::wstring &basePath, const WalkOptions &options)
    : AsyncProgressQueueWorker<Entry>(callback)
    , mProgress(progress)
    , mBasePath(basePath)
    , mOptions(options)
    , mErrorCode(0)
    , mErrorFunc()
    , mErrorPath()
  {}

  ~WalkWorker() {
    delete mProgress;
  }

  virtual void Execute(const AsyncProgressQueueWorker<Entry>::ExecutionProgress &progress) override {
    mCancelled = false;
    try {
      walk(mBasePath, [&progress, this](const std::vector<Entry> &entries) -> bool {
        progress.Send(&entries[0], entries.size());
        return !mCancelled;
      }, mOptions);
    } catch (const ApiError &e) {
      SetErrorMessage("API Error");
      mErrorCode = e.code();
      mErrorFunc = e.func();
      mErrorPath = e.path();
    } catch (const std::exception &e) {
      SetErrorMessage(e.what());
    }
  }

  virtual void HandleProgressCallback(const Entry *data, size_t size) override {
    Nan::HandleScope scope;

    v8::Local<v8::Value> argv[] = {
        convert(data, size).As<v8::Value>()
    };
    v8::MaybeLocal<v8::Value> res = Nan::Call(*mProgress, 1, argv);
    if (!cast<bool>(res.ToLocalChecked())) {
      mCancelled = true;
    }
  }

  virtual void HandleOKCallback () override {
    Nan::HandleScope scope;

    Local<Value> argv[] = {
        Null()
    };

    Nan::Call(*callback, 1, argv);
  }

  virtual void HandleErrorCallback() override {
    Nan::HandleScope scope;

    Local<Value> argv[] = {
      mErrorCode != 0
        ? WinApiException(mErrorCode, mErrorFunc.c_str(), mErrorPath.c_str())
        : Exception::Error(New<v8::String>(ErrorMessage()).ToLocalChecked())
    };
    Nan::Call(*callback, 1, argv);
  }

 private:
   Callback *mProgress;
   std::wstring mBasePath;
   bool mCancelled;
   WalkOptions mOptions;
   uint32_t mErrorCode;
   std::string mErrorFunc;
   std::string mErrorPath;
};


template <typename T>
T get(const v8::Local<v8::Object> &obj, const char *key, const T &def) {
  v8::Local<v8::String> keyLoc = Nan::New(key).ToLocalChecked();
  return obj->Has(keyLoc) ? cast<T>(obj->Get(keyLoc)) : def;
}

NAN_METHOD(walku8) {
  if (info.Length() < 3) {
    Nan::ThrowTypeError("Expected 3 or 4 arguments");
    return;
  }

  String::Utf8Value basePath(info[0]->ToString());
  Callback *progress = new Callback(To<v8::Function>(info[1]).ToLocalChecked());
  Callback *callback = new Callback(To<v8::Function>(info[2]).ToLocalChecked());

  WalkOptions options;
  if (info.Length() > 3) {
    v8::Local<v8::Object> optionsIn = To<v8::Object>(info[3]).ToLocalChecked();
    options.details = get(optionsIn, "details", false);
    options.terminators = get(optionsIn, "terminators", false);
    options.threshold = get(optionsIn, "threshold", 1024);
    options.recurse = get(optionsIn, "recurse", true);
    options.skipHidden = get(optionsIn, "skipHidden", true);
  }

  std::wstring walkPath = toWC(*basePath, CodePage::UTF8, strlen(*basePath));
  AsyncQueueWorker(new WalkWorker(callback, progress, walkPath, options));
}

NAN_MODULE_INIT(Init) {
  Nan::Set(target, New<v8::String>("default").ToLocalChecked(),
    GetFunction(New<v8::FunctionTemplate>(walku8)).ToLocalChecked());
}

NODE_MODULE(turbowalk, Init)
