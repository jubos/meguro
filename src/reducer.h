#ifndef __MEGURO_REDUCER_H__
#define __MEGURO_REDUCER_H__

#include <tcutil.h>
#include <tchdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "types.h"
#include "js_handle.h"
#include "thread_safe_queue.h"
#include "meguro_exception.h"

namespace meguro {

  class ReducerException : public MeguroException {
    public:
      ReducerException(const char* what) : MeguroException(what) {}
  };

  class Reducer {
    public:
      Reducer(const MeguroEnvironment* env);
      void begin();
      void end();

      KeyValueListPair* next();
      void save(const string& key, const string& value);
      virtual ~Reducer();

      JSHandle* js_;
      const char* reduce_out_path_;
      TCHDB* reduce_out_db_;
      Progress* progress_;
    protected:
      ThreadSafeQueue<KeyValueListPair*>* tsq_;
      const MeguroEnvironment* env_;
      void open_reducer_out();
      void close_reducer_out();
  };
}

#endif
