#include "tokyo_cabinet_iterator.h"
#include <string.h>

using namespace meguro;
/*
 * Open the Tokyo Cabinet file with a few settings
 */
void 
TokyoCabinetIterator::initialize(const MeguroEnvironment* env, const char* uri) throw (IteratorInitException)
{
  tcdb_ = tcadbnew();
  char path[4096];

  snprintf(path,4096,"%s#mode=ref#opts=l",uri);
  if(!tcadbopen(tcdb_,path)) {
    throw IteratorInitException();
  }

  if(!tcadbiterinit(tcdb_)) {
    throw IteratorInitException();
  }

  uint64_t size = tcadbrnum(tcdb_);

  progress_ = new Progress("Mapping",size, env->verbose_progress);
}


/*
 * Pull back the next key value pair If we don't find a vbuf, let's just make
 * it an "" to ease the javascript.  Will need to deal with NULL values inside
 * the Tokyo Cabinet in the future.
 */
KeyValuePair*
TokyoCabinetIterator::next() throw (IteratorCompleteException)
{
  char* kbuf = (char*) tcadbiternext2(tcdb_);
  if (kbuf) {
    char* vbuf = tcadbget2(tcdb_,kbuf);
    if (!vbuf)
      vbuf = strdup("");
    KeyValuePair* kvp = new KeyValuePair(kbuf,vbuf);
    if (progress_)
      progress_->tick();
    return kvp;
  } else {
    if (progress_)
      progress_->done();
    throw IteratorCompleteException();
  }
}

/*
 * Close the tokyo cabinet file
 */
TokyoCabinetIterator::~TokyoCabinetIterator() {
  if (!tcadbclose(tcdb_)) {
    fprintf(stderr,"Iterator Closing Error\n");
  }
  if (progress_)
    delete progress_;
  tcadbdel(tcdb_);
}
