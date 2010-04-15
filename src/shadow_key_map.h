#ifndef __MG_SHADOW_KEY_MAP_H__
#define __MG_SHADOW_KEY_MAP_H__

#include "uthash.h"
#include "environment.h"
#include "meguro_exception.h"
#include <pthread.h>

namespace meguro {
  struct shadow_key_count_t {
    char* key;
    uint32_t count;
    UT_hash_handle hh;
  };

  class ShadowKeyMapException : public MeguroException {
    public:
      ShadowKeyMapException(const char* what) : MeguroException(what) {}
  };

  class ShadowKeyMap {
    public:
      ShadowKeyMap(const MeguroEnvironment* env);
      ~ShadowKeyMap();
      /*
       * Load up a shadow key map from a previous map output file by analyzing
       * the keys in the file
       */
      void load(const char* filename) throw(ShadowKeyMapException);
      
      /*
       * Increment a count associated with a key
       */
      void increment(const char* key);

      /*
       * Get the current key count for a particular key
       */
      uint32_t key_count(const char* key);

      char* increment_and_return_shadow_key(const char* key);

      char* generate_shadow_key(const char* key, uint32_t count);

      /*
       * Get the current shadow key for a particular root key
       */
      char* current_shadow_key(const char* key);

      void print();

      uint64_t size();

      /*
       * Breaks encapsulation but allows for easy iteration from other classes
       */
      shadow_key_count_t* map_root() { return map_root_; }

    protected:
      const MeguroEnvironment* env_;
      shadow_key_count_t* map_root_;
      pthread_mutex_t emit_mutex_;

  };
}


#endif
