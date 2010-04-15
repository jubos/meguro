#include "tokyo_cabinet_hash_iterator.h"

using namespace meguro;

/*
 * Open the Tokyo Cabinet file with a few settings
 */
void 
TokyoCabinetHashIterator::initialize(const MeguroEnvironment* env, const char* path) throw (IteratorInitException)
{
  tchdb_ = tchdbnew();

  if(!tchdbopen(tchdb_,path,HDBOREADER | HDBONOLCK)) {
    throw IteratorInitException();
  }

  if(!tchdbiterinit(tchdb_)) {
    throw IteratorInitException();
  }

  uint64_t size = tchdbrnum(tchdb_);

  progress_ = new Progress("Mapping",size, env->verbose_progress);
}


/*
 * Pull back the next key value pair If we don't find a vbuf, let's just make
 * it an "" to ease the javascript.  Will need to deal with NULL values inside
 * the Tokyo Cabinet in the future.
 */
KeyValuePair*
TokyoCabinetHashIterator::next() throw (IteratorCompleteException)
{
  TCXSTR *key = tcxstrnew();
  TCXSTR *val = tcxstrnew();
  bool res = tchdbiternext3(tchdb_,key,val);
  if (res) {
    KeyValuePair* kvp = new KeyValuePair((char*)tcxstrptr(key),(char*) tcxstrptr(val));
    if (progress_)
      progress_->tick();
    free(key);
    free(val);
    return kvp;
  } else {
    tcxstrdel(key);
    tcxstrdel(val);
    if (progress_)
      progress_->done();
    throw IteratorCompleteException();
  }
}

/*
 * Close the tokyo cabinet file
 */
TokyoCabinetHashIterator::~TokyoCabinetHashIterator() {
  if (!tchdbclose(tchdb_)) {
    fprintf(stderr,"Iterator Closing Error\n");
  }
  if (progress_)
    delete progress_;
  tchdbdel(tchdb_);
}
