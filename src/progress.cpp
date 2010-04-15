#include "progress.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace meguro;

Progress::Progress(const char* title,uint64_t size, bool verbose) 
{
  verbose_ = verbose;
  title_ = strdup(title);
  size_ = size;
  current_ = 0;
  current_percentage_ = 0;
  if (verbose)
    fprintf(stderr,"\r%s: %3d%% (%12llu/%12llu)", title_,current_percentage_,1LL,(unsigned long long) size_);
  else
    fprintf(stderr,"\r%s: %3d%%", title_,current_percentage_);
  fflush(stderr);
}

void
Progress::tick(uint64_t size)
{
  current_ += size;
  uint32_t percentage = (uint32_t) (((double) current_ / size_) * 100);
  if (verbose_) {
    fprintf(stderr,"\r%s: %3d%% (%12llu/%12llu)", title_,percentage, (unsigned long long) current_, (unsigned long long) size_);
    fflush(stderr);
  } else {
    if (percentage > current_percentage_) {
      fprintf(stderr,"\r%s: %3d%%", title_,current_percentage_);
      fflush(stderr);
      current_percentage_ = percentage;
    }
  }

}

void
Progress::done()
{
  fprintf(stderr,"\r%s: 100%%\n", title_);
  fflush(stderr);
}

Progress::~Progress()
{
  free(title_);
}
