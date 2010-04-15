#ifndef __MEGURO_V8_JS_HANDLE__
#define __MEGURO_V8_JS_HANDLE__ 

#include <string>
#include <v8.h>

#include "js_handle.h"

using namespace std;
using namespace v8;

namespace meguro {
  class Mapper;

  class V8JSHandle : public JSHandle {
    public:
      V8JSHandle(Mapper* mapper, const string& js_file);

      string map(const string& key, const string& value);
      string reduce(const string& key, const string& value);

      virtual ~V8JSHandle() {}

    protected:
      Persistent<Function> init_fn_;
      Persistent<Function> map_fn_;
      Persistent<Function> reduce_fn_;

      Persistent<Context> context_;
      Handle<ObjectTemplate> global_;
  };
}

#endif
