#include "stdin_iterator.h"
#include "environment.h"
#include "progress.h"

#include <string.h>

using namespace meguro;

void
StdinIterator::initialize(const MeguroEnvironment* env, const char* path) throw(IteratorInitException)
{
  progress_ = NULL;
}

KeyValuePair*
StdinIterator::next() throw(IteratorCompleteException)
{
  char buf[IO_BUFFER_SIZE];
  int full_len = 0;
  char* full_line = NULL;
  char* line = NULL;
  for(;;) {
    line = fgets(buf,IO_BUFFER_SIZE,stdin);
    if (!line && !full_line) {
      if (feof(stdin)) {
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

StdinIterator::~StdinIterator()
{
  if (progress_)
    delete progress_;
}
