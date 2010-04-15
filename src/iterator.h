#ifndef __MEGURO_ITERATOR_H__
#define __MEGURO_ITERATOR_H__

#include <stdlib.h>
#include <exception>

#include "environment.h"
#include "types.h"

using namespace std;

namespace meguro {
  class IteratorInitException : public exception {};
  class IteratorCompleteException : public exception {};

  class Iterator {
    public:
      virtual void initialize(const MeguroEnvironment* environment, const char* uri) throw(IteratorInitException) = 0;
      virtual KeyValuePair* next() throw(IteratorCompleteException) = 0;
      virtual ~Iterator() {};
    protected:
      unsigned int count_;
  };
}

#endif
