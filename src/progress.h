#ifndef __MEGURO_PROGRESS_H__
#define __MEGURO_PROGRESS_H__

#include <stdint.h>

namespace meguro {
  class Progress {
    public:
      Progress(const char* title, uint64_t size, bool verbose = false);
      void tick(uint64_t unit_size = 1);
      void done();

      virtual ~Progress();

    protected:
      char* title_;
      bool verbose_;
      uint64_t size_;
      uint64_t current_;
      uint32_t current_percentage_;
  };

}

#endif
