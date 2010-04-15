#ifndef __MEGURO_DICTIONARY_H__
#define __MEGURO_DICTIONARY_H__

#include "uthash.h"
#include <exception>
#include "environment.h"

using namespace std;

namespace meguro {
  struct dict_elem_t {
    char* key;
    char* value;
    UT_hash_handle hh;
  };

  class DictionaryException : public exception {};

  class Dictionary {
    public:
      Dictionary();

      void load(const MeguroEnvironment* env, const char* filename);
      const char* get(const char* key);

      virtual ~Dictionary();

    protected:
      dict_elem_t* root_;
  };
}

#endif
