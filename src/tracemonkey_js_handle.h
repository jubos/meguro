#ifndef __MEGURO_TRACEMONKEY_JS_HANDLE__
#define __MEGURO_TRACEMONKEY_JS_HANDLE__

#include <jsapi.h>
#include "js_handle.h"
#include "environment.h"

namespace meguro {
  class Mapper;
  class Reducer;

  class TraceMonkeyJSHandle : public JSHandle {
    public:
      static bool tracemonkey_initialized;
      TraceMonkeyJSHandle(const MeguroEnvironment* env);

      bool has_reduce_fn();
      bool has_map_fn();

      void map(const string& key, const string& value);
      void reduce(const string& key, const vector<char*>& value);
      void log(const string& msg);
      void mapper(Mapper* mapper);
      void reducer(Reducer* reducer);

      ~TraceMonkeyJSHandle(); 

    protected:
      const MeguroEnvironment* env_;
      JSRuntime* rt_;
      JSContext* ctx_;
      JSObject* script_;
      JSObject* global_;
      JSObject* meguro_obj_;
  };
}


#endif
