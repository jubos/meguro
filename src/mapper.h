#ifndef __MEGURO_MAPPER_H__
#define __MEGURO_MAPPER_H__

#include <tcutil.h>
#include <tcbdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <exception>
#include "js_handle.h"
#include "thread_safe_queue.h"
#include "environment.h"
#include "iterator.h"
#include <pthread.h>

#include "uthash.h"

/*
 * The mapper gets called for each entry in the Tokyo Cabinet file
 */

namespace meguro {
  class MapperException : std::exception {
    public:
      MapperException(const char* what) {
        what_ = what;
      }
      const char* what() {
        return what_;
      }
    protected:
      const char* what_;
  };

  class Mapper {
    public:
      Mapper(const MeguroEnvironment* env);
      virtual ~Mapper();

      void begin();
      void end();

      KeyValuePair* next();
      void emit(const string& key, const string& value);
      void emit_noop(const string& key, const string& value);

      void set(const string& key, const string& value);

      bool reduce_;
      JSHandle* js_;
      const char* map_out_path_;
      TCHDB* map_out_db_;
      int map_count_;
      ThreadSafeQueue<KeyValuePair*>* tsq_;
      Iterator* iterator_;
    protected:
      uint64_t emit_count_;
      uint64_t set_count_;
      uint64_t emit_size_estimate_;
      uint32_t input_path_index_;
      pthread_mutex_t emit_mutex_;
      Progress* progress_;
      const MeguroEnvironment* env_;
  };
}

#endif
