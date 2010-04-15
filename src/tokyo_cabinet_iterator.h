#ifndef __MEGURO_TOKYO_CABINET_ITERATOR_H__
#define __MEGURO_TOKYO_CABINET_ITERATOR_H__

#include <string>
#include <tcadb.h>
#include "iterator.h"
#include "progress.h"

using namespace std;

namespace meguro {
  class TokyoCabinetIterator : public Iterator {
    public:
      void initialize(const MeguroEnvironment* environment, const char* path) throw (IteratorInitException);
      KeyValuePair* next() throw (IteratorCompleteException);

      ~TokyoCabinetIterator();

    protected:
      TCADB* tcdb_;
      Progress* progress_;
  };
}


#endif
