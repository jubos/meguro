#include "gzip_iterator.h"
#include "environment.h"
#include "progress.h"
#include <zlib.h>

#include <string.h>

using namespace meguro;

void
GzipIterator::initialize(const MeguroEnvironment* env, const char* path) throw(IteratorInitException)
{
  if (stat64(path, &file_stats_) != 0) {
    throw IteratorInitException();
  }

  file_ = gzopen(path,"r");
  if (!file_)
    throw IteratorInitException();

  int title_len = strlen("Mapping ") + strlen(path) + 1;
  char* title = (char*) malloc(title_len);
  snprintf(title,title_len,"%s%s", "Mapping ", path);
  progress_ = new Progress(title,0, env->verbose_progress);
  free(title);
}

KeyValuePair*
GzipIterator::next() throw(IteratorCompleteException)
{
  char buf[IO_BUFFER_SIZE];
  int full_len = 0;
  char* full_line = NULL;
  char* line = NULL;
  for(;;) {
    line = gzgets(file_,buf,IO_BUFFER_SIZE);
    if (!line && !full_line) {
      if (gzeof(file_)) {
        if (progress_) 
          progress_->done();
        throw IteratorCompleteException();
      } else {
        if (progress_) 
          progress_->done();
        throw IteratorCompleteException();
      }
    }

    if (!line)
      break;

    int len = strlen(line);
    if (progress_)
      progress_->absolute(len);
    int new_len = full_len + len;
    full_line = (char*) realloc(full_line,full_len + len);
    memcpy(full_line + full_len,line,len);
    full_len = new_len;
    if (line[len - 1] == '\n') {
      full_line[full_len -1] = '\0';
      break;
    }
  }

  KeyValuePair* kvp = new KeyValuePair(strdup(""),full_line);
  return kvp;
}

GzipIterator::~GzipIterator()
{
  gzclose(file_);
  if (progress_)
    delete progress_;
}
