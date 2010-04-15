#ifndef __MEGURO_LINE_ITERATOR_H__
#define __MEGURO_LINE_ITERATOR_H__

#include "iterator.h"
#include "progress.h"

#include <sys/stat.h>
#include <string>
#include <stdio.h>

using namespace std;

namespace meguro {
  class LineIterator : public Iterator {
    public:
      static const int IO_BUFFER_SIZE = 4096;
      void initialize(const MeguroEnvironment* environment, const char* uri) throw (IteratorInitException);
      KeyValuePair* next() throw (IteratorCompleteException);

      ~LineIterator();

    protected:
      struct stat64 file_stats_;
      FILE* file_;
      Progress* progress_;
  };

}


#endif
