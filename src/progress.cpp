#include "progress.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

using namespace meguro;

static inline void output_human_readable_number(char* title, uint64_t number);

Progress::Progress(const char* title,uint64_t size, bool verbose) 
{
  verbose_ = verbose;
  title_ = strdup(title);
  size_ = size;
  absolute_ = (size == 0);
  current_ = 0;
  current_percentage_ = 0;
  current_sample_ = 0;
  sample_rate_ = 1000;
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
Progress::absolute(uint64_t size)
{
  current_ += size;
  if (verbose_) {
    output_human_readable_number(title_,current_);
  } else {
    current_sample_++;
    if (current_sample_ % sample_rate_ == 0) {
      output_human_readable_number(title_,current_);
    }
  }
  fflush(stderr);
}

void
Progress::done()
{
  if (absolute_) {
    output_human_readable_number(title_,current_);
    fprintf(stderr,"\n");
  }
  else
    fprintf(stderr,"\r%s: 100%%\n", title_);
  fflush(stderr);
}

Progress::~Progress()
{
  free(title_);
}

//------------------------------------------------
//-- S t a t i c    I m p l e m e n t a t i o n --
//------------------------------------------------
static void output_human_readable_number(char* title, uint64_t number) {
  if (number > 1024 * 1024) {
    fprintf(stderr,"\r%s: %7.2fM", title,(double) number / (1024 * 1024));
  } else if(number > 1024) {
    fprintf(stderr,"\r%s: %7.2fK", title,(double) number / 1024);
  } else 
    fprintf(stderr,"\r%s: %7llu", title,number);
}
