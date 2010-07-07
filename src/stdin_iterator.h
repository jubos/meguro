#ifndef __MEGURO_STDIN_ITERATOR_H__
#define __MEGURO_STDIN_ITERATOR_H__

#include "iterator.h"
#include "progress.h"

#include <sys/stat.h>
#include <string>
#include <stdio.h>

using namespace std;

namespace meguro {
  class StdinIterator : public Iterator {
    public:
      static const int IO_BUFFER_SIZE = 8192;
      void initialize(const MeguroEnvironment* environment, const char* uri) throw (IteratorInitException);
      KeyValuePair* next() throw (IteratorCompleteException);

      ~StdinIterator();

    protected:
      Progress* progress_;
  };

}


#endif
