#ifndef __MEGURO_TOKYO_CABINET_HASH_ITERATOR_H__
#define __MEGURO_TOKYO_CABINET_HASH_ITERATOR_H__

#include <string>
#include <tchdb.h>
#include "iterator.h"
#include "progress.h"

using namespace std;

namespace meguro {
  class TokyoCabinetHashIterator : public Iterator {
    public:
      void initialize(const MeguroEnvironment* environment, const char* path) throw (IteratorInitException);
      KeyValuePair* next() throw (IteratorCompleteException);

      ~TokyoCabinetHashIterator();

    protected:
      TCHDB* tchdb_;
      Progress* progress_;
  };
}


#endif
