#ifndef __MEGURO_JS_HANDLE__
#define __MEGURO_JS_HANDLE__ 

#include <string>
#include <vector>

#include "environment.h"
#include "meguro_exception.h"

using namespace std;

namespace meguro {
  class Mapper;
  class Reducer;

  class JSHandleException : public MeguroException {
    public:
      JSHandleException(const char* what) : MeguroException(what) {}
  };

  class JSHandle {
    public:
      JSHandle(const MeguroEnvironment* env) { env_ = env; }
      virtual bool has_reduce_fn() = 0;
      virtual bool has_map_fn() = 0;

      virtual void map(const string& key, const string& value) = 0;
      virtual void reduce(const string& key, const vector<char*>& values) = 0;
      virtual void log(const string& msg) = 0;
      virtual void mapper(Mapper* mapper) = 0;
      virtual void reducer(Reducer* reducer) = 0;

      Mapper* mapper() { return mapper_; }
      Reducer* reducer() { return reducer_; }
      const MeguroEnvironment* env() { return env_; }

      virtual ~JSHandle() {}

    protected:
      const MeguroEnvironment* env_;
      Mapper* mapper_;
      Reducer* reducer_;

  };
}

#endif
