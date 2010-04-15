#include <iostream>
#include <tcbdb.h>

#include "base64.h"
#include "reducer.h"
#include "tracemonkey_js_handle.h"
#include "mapper.h"
#include "types.h"
#include "progress.h"
#include "shadow_key_map.h"

using namespace std;
using namespace meguro;

// -------------------------------------------
// -- P u b l i c  I m p l e m e n t a t i o n
// -------------------------------------------

/*
 * Open the JS file and store it in the js_ handle
 * Initialize the intermediate tokyo cabinet file for mapping output
 */

Reducer::Reducer(const MeguroEnvironment* env)
{
  env_ = env;
  progress_ = NULL;
  tsq_ = new ThreadSafeQueue<KeyValueListPair*>(100);
}

void 
Reducer::begin()
{
  int ecode;
  TCHDB* map_out_db;
  map_out_db = tchdbnew();
  if(!tchdbtune(map_out_db,-1,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(map_out_db);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
  }

  if (!tchdbopen(map_out_db,env_->map_out_path, HDBOREADER | HDBONOLCK)) {
    ecode = tchdbecode(map_out_db);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
  }

  open_reducer_out();

  shadow_key_count_t* elem = NULL;
  ShadowKeyMap* shad = env_->shadow_key_map;
  shadow_key_count_t* shadow_root = env_->shadow_key_map->map_root();
  uint64_t num_shadow_keys = HASH_COUNT(shadow_root);
  progress_ = new Progress("Reducing",num_shadow_keys, env_->verbose_progress);

  for(elem = shadow_root; elem != NULL; elem=(shadow_key_count_t*) elem->hh.next) {
    uint32_t key_count = 0;
    KeyValueListPair*  pair = new KeyValueListPair();
    pair->key = strdup(elem->key);
    for(key_count=0; key_count <= elem->count; key_count++) {
      char* shadow_key = shad->generate_shadow_key(elem->key,key_count);
      char* value = NULL;
      if ((value = tchdbget2(map_out_db, shadow_key)) != NULL) {
        pair->value_list.push_back(value);
      }
      free(shadow_key);
    }
    progress_->tick();
    tsq_->enqueue(pair);
  }

  if (!tchdbclose(map_out_db)) {
    ecode = tchdbecode(map_out_db);
    fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
  }

  tchdbdel(map_out_db);

  progress_->done();
  tsq_->done(true);
}

void
Reducer::end()
{
  close_reducer_out();
}

KeyValueListPair* 
Reducer::next() 
{
  return tsq_->dequeue();
}

/*
 * Take the string and value and dump it into the output tokyo cabinet.  This
 * will overwrite the existing value.
 */
void 
Reducer::save(const string& key, const string& value)
{
  if (!tchdbput2(reduce_out_db_,key.c_str(),value.c_str()))
    fprintf(stderr,"Error Saving\n");
}

Reducer::~Reducer() 
{
  delete tsq_;
  delete progress_;
}

void 
Reducer::open_reducer_out()
{
  int ecode;
  reduce_out_db_= tchdbnew();
  ShadowKeyMap* shad = env_->shadow_key_map;
  uint64_t num_buckets = shad->size();
  if(!tchdbtune(reduce_out_db_,num_buckets,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(reduce_out_db_);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    throw ReducerException("Reducer Tuning Error");
    exit(-1);
  }

  if (!tchdbsetxmsiz(reduce_out_db_,env_->map_mem_size)) {
    ecode = tchdbecode(reduce_out_db_);
    fprintf(stderr, "mmap error: %s\n", tchdberrmsg(ecode));
    throw ReducerException("Reducer mmap error");
  }

  if (!tchdbsetmutex(reduce_out_db_)) {
    ecode = tchdbecode(reduce_out_db_);
    fprintf(stderr, "mutext error on reduce output db: %s\n", tchdberrmsg(ecode));
    throw ReducerException("Reducer tokyo cabinet mutex error");
  }

  if(!tchdbopen(reduce_out_db_,env_->reduce_out_path, HDBOWRITER|HDBOCREAT|HDBOTRUNC)){
    ecode = tchdbecode(reduce_out_db_);
    fprintf(stderr, "Reducer open error: %s\n", tchdberrmsg(ecode));
    throw ReducerException("Reducer tokyo cabinet open error");
  }
}

void 
Reducer::close_reducer_out()
{
  int ecode;
  if (!tchdbclose(reduce_out_db_)) {
    ecode = tchdbecode(reduce_out_db_);
    fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
    throw ReducerException("Reducer tokyo cabinet close error");
  }
  tchdbdel(reduce_out_db_);
}
