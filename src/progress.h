#ifndef __MEGURO_PROGRESS_H__
#define __MEGURO_PROGRESS_H__

#include <stdint.h>

namespace meguro {
  class Progress {
    public:
      /* 
       * Pass a size of 0 to denote absolute progress rather than a percentage
       * on data sources where you don't know how big it will be 
       */
      Progress(const char* title, uint64_t size, bool verbose = false);
      void tick(uint64_t unit_size = 1);
      /*
       * Use absolute if we don't know how big the incoming data is
       */
      void absolute(uint64_t increment);
      void done();

      virtual ~Progress();

    protected:
      char* title_;
      bool verbose_;
      bool absolute_;
      uint64_t size_;
      uint64_t current_;
      uint32_t current_percentage_;
      uint32_t current_sample_;
      uint32_t sample_rate_;
  };

}

#endif
