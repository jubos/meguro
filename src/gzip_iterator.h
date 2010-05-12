#ifndef __MEGURO_GZIP_ITERATOR_H__
#define __MEGURO_GZIP_ITERATOR_H__

#include "iterator.h"
#include "progress.h"

#include <sys/stat.h>
#include <string>
#include <stdio.h>
#include <zlib.h>

using namespace std;

namespace meguro {
  class GzipIterator : public Iterator {
    public:
      static const int IO_BUFFER_SIZE = 8192;
      void initialize(const MeguroEnvironment* environment, const char* uri) throw (IteratorInitException);
      KeyValuePair* next() throw (IteratorCompleteException);

      ~GzipIterator();

    protected:
      struct stat64 file_stats_;
      gzFile file_;
      Progress* progress_;
  };

}


#endif
