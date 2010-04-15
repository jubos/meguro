#include "dictionary.h"
#include "stdlib.h"
#include "progress.h"
#include <tcadb.h>

using namespace meguro;

Dictionary::Dictionary() 
{
}

void
Dictionary::load(const MeguroEnvironment* env,const char* filename) 
{
  TCADB* tcdb = tcadbnew();
  char path[4096];
  root_ = NULL;

  snprintf(path,4096,"%s#mode=ref#opts=l", (char*) filename);
  if(!tcadbopen(tcdb,path)) {
    throw DictionaryException();
  }

  if(!tcadbiterinit(tcdb)) {
    throw DictionaryException();
  }

  uint64_t rnum = tcadbrnum(tcdb);

  Progress* progress = new Progress("Loading Dictionary",rnum, env->verbose_progress);

  char* kbuf = NULL;
  while((kbuf = tcadbiternext2(tcdb)) != NULL) {
    progress->tick();
    char* vbuf = tcadbget2(tcdb,kbuf);
    if (!vbuf)
      vbuf = strdup("");
    dict_elem_t* elem = (dict_elem_t*) malloc(sizeof(dict_elem_t));
    elem->key = kbuf;
    elem->value = vbuf;
    HASH_ADD_KEYPTR(hh,root_,elem->key,strlen(kbuf),elem);
  }

  progress->done();
  delete progress;

  if (!tcadbclose(tcdb)) {
  }

  tcadbdel(tcdb);
}

const char*
Dictionary::get(const char* key) 
{
  dict_elem_t* elem = NULL;
  HASH_FIND_STR(root_,key,elem);
  if (elem)
    return elem->value;
  else
    return NULL;
}


/*
 * Destructor needs to clear out the hash
 */
Dictionary::~Dictionary() 
{
  dict_elem_t* cur;
  while(root_) {
    cur = root_;
    HASH_DEL(root_,cur);
    free(cur->key);
    free(cur->value);
    free(cur);
  }
}

