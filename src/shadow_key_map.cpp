#include "shadow_key_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tchdb.h>
#include "progress.h"

using namespace meguro;

// There could be a bug here if we have insane numbers of emits!
// 1024 should be more than enough
#define MAX_SUFFIX_LEN 1024
#define SUFFIX_SEPARATOR "|m"

ShadowKeyMap::ShadowKeyMap(const MeguroEnvironment* env)
{
  env_ = env;
  map_root_ = NULL;
}

void 
ShadowKeyMap::load(const char* filename) throw(ShadowKeyMapException)
{
  int ecode = 0;
  TCHDB* tchdb = tchdbnew();
  if(!tchdbopen(tchdb,filename,HDBOREADER | HDBONOLCK)) {
    ecode = tchdbecode(tchdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    throw ShadowKeyMapException("Cannot open mapper output for load in shadow key map");
  }

  if(!tchdbiterinit(tchdb)) {
    ecode = tchdbecode(tchdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    throw ShadowKeyMapException("Cannot instantiate iterator");
  }

  uint64_t size = tchdbrnum(tchdb);

  Progress* progress = new Progress("Loading Existing Mapper Output", size, env_->verbose_progress);

  char* key = NULL;
  int suffix_len = strlen(SUFFIX_SEPARATOR);
  while((key = tchdbiternext2(tchdb)) != NULL) {
    shadow_key_count_t* kv = NULL;
    char* suffix = NULL;
    uint32_t new_count = 0;
    if ((suffix = strstr(key,SUFFIX_SEPARATOR)) != NULL) {
      new_count = (uint32_t) atoi(suffix + suffix_len);
      key[suffix - key] = 0x0;
    }
    HASH_FIND_STR(map_root_,key, kv);
    if (kv) {
      if (new_count > kv->count) {
        kv->count = new_count;
        free(key);
      }
    } else {
      kv = (shadow_key_count_t*) malloc(sizeof(shadow_key_count_t));
      kv->key = key;
      kv->count = new_count;
      HASH_ADD_KEYPTR(hh,map_root_,kv->key, strlen(kv->key), kv);
    }
    progress->tick();
  }

  progress->done();
  delete progress;

  if (!tchdbclose(tchdb)) {
    fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
  }

  tchdbdel(tchdb);
}

void 
ShadowKeyMap::increment(const char* key)
{
  shadow_key_count_t* kv = NULL;
  HASH_FIND_STR(map_root_,key,kv);
  if (kv) {
    kv->count++;
  } else {
    kv = (shadow_key_count_t*) malloc(sizeof(shadow_key_count_t));
    kv->key = strdup(key);
    kv->count = 0;
    HASH_ADD_KEYPTR(hh,map_root_,kv->key, strlen(kv->key), kv);
  }
}

/*
 * Mainly for debugging, but let's print it out
 */
void 
ShadowKeyMap::print()
{
  shadow_key_count_t* elem = NULL;
  for(elem = map_root_; elem != NULL; elem=(shadow_key_count_t*) elem->hh.next) {
    printf("%s:%u\n", elem->key, elem->count);
  }
}

uint32_t 
ShadowKeyMap::key_count(const char* key)
{
  shadow_key_count_t* kv = NULL;
  HASH_FIND_STR(map_root_,key,kv);
  if (kv) {
    return kv->count;
  } else {
    return 0;
  }
}

char* 
ShadowKeyMap::increment_and_return_shadow_key(const char* key)
{
  shadow_key_count_t* kv = NULL;
  HASH_FIND_STR(map_root_,key,kv);
  if (kv) {
    kv->count++;
  } else {
    kv = (shadow_key_count_t*) malloc(sizeof(shadow_key_count_t));
    kv->key = strdup(key);
    kv->count = 0;
    HASH_ADD_KEYPTR(hh,map_root_,kv->key, strlen(kv->key), kv);
  }
  return generate_shadow_key(key,kv->count);
}

char* 
ShadowKeyMap::current_shadow_key(const char* key)
{
  shadow_key_count_t* kv = NULL;
  HASH_FIND_STR(map_root_,key,kv);
  uint32_t count = 0;
  if (kv) {
    count = kv->count;
  }
  return generate_shadow_key(key,count);
}

char* 
ShadowKeyMap::generate_shadow_key(const char* key, uint32_t count)
{
  if (count == 0)
    return strdup(key);

  int root_len = strlen(key);
  int suffix_len = 1;
  if (count > 9)
    suffix_len = ((int)log10(count)) + 1;

  int shadow_key_len = root_len + strlen(SUFFIX_SEPARATOR) + suffix_len + 1;
  char* shadow_key = (char*) malloc(shadow_key_len);
  snprintf(shadow_key,shadow_key_len,"%s%s%d",key,SUFFIX_SEPARATOR,count);
  return shadow_key;
}

uint64_t 
ShadowKeyMap::size()
{
  return HASH_COUNT(map_root_);
}

/*
 * Destructor needs to destroy all the elements in the hash
 */
ShadowKeyMap::~ShadowKeyMap() 
{
  shadow_key_count_t* elem = NULL;
  while(map_root_) {
    elem = map_root_;
    HASH_DEL(map_root_,elem);
    free(elem->key);
    free(elem);
  }
}
