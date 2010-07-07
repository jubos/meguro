#include <string.h>
#include <iostream>
#include <math.h>

#include "mapper.h"
#include "iterator.h"
#include "line_iterator.h"
#include "tokyo_cabinet_iterator.h"
#include "gzip_iterator.h"
#include "tokyo_cabinet_hash_iterator.h"
#include "stdin_iterator.h"
#include "base64.h"
#include "mgutil.h"
#include "shadow_key_map.h"

using namespace std;
using namespace meguro;

// There could be a bug here if we have insane numbers of emits!
// 1024 should be more than enough
//#define MAX_SUFFIX_LEN 1024
//#define SUFFIX_SEPARATOR "|m"

static Iterator* pick_an_iterator(const MeguroEnvironment* env, const char* path);

/*
 * Open the JS file and store it in the js_ handle
 * Initialize the intermediate tokyo cabinet file for mapping output
 * If
 */
Mapper::Mapper(const MeguroEnvironment* env)
{
  emit_count_ = 0;
  set_count_ = 0;
  emit_size_estimate_ = 0;
  env_ = env;
  reduce_ = env->reduce_out_path != NULL;
  map_count_ = 0;
  map_out_path_= env->map_out_path;

  tsq_ = new ThreadSafeQueue<KeyValuePair*>(1000);

  if (pthread_mutex_init(&emit_mutex_,NULL) < 0) {
    throw MapperException("Mutex Init Error");
  }

  input_path_index_ = 0;
}

Mapper::~Mapper()
{
  delete tsq_;
}

/*
 * Iterate through the tokyo cabinet input file and put key/value pairs into
 * the thread safe queue.
 */
void
Mapper::begin()
{
  int ecode;
  map_out_db_ = tchdbnew();
  uint8_t tc_opts = HDBTLARGE;
  if (env_->map_bzip2)
    tc_opts |= HDBTBZIP;
  if(!tchdbtune(map_out_db_,env_->number_of_buckets,-1,-1,tc_opts)) {
    ecode = tchdbecode(map_out_db_);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if (!tchdbsetxmsiz(map_out_db_,env_->map_mem_size)) {
    ecode = tchdbecode(map_out_db_);
    fprintf(stderr, "mmap error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if (!tchdbsetmutex(map_out_db_)) {
    ecode = tchdbecode(map_out_db_);
    fprintf(stderr, "mutext error on map output db: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if (!tchdbopen(map_out_db_,map_out_path_, HDBOWRITER|HDBOCREAT|HDBOTRUNC)) {
    ecode = tchdbecode(map_out_db_);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  while(input_path_index_ < env_->input_paths.size()) {
    const char* path = env_->input_paths[input_path_index_];
    try {
      iterator_ = pick_an_iterator(env_,path);
      iterator_->initialize(env_, path);
      for(;;) {
        KeyValuePair* kvp = iterator_->next();
        if (env_->key_pattern) {
          if (!strcmp(kvp->key,env_->key_pattern))
            tsq_->enqueue(kvp);
          else
            delete kvp;
        } else {
          tsq_->enqueue(kvp);
        }
      }
    } catch (IteratorInitException& e) {
      fprintf(stderr, "Iterator Initialization Error on %s\n", path);
    } catch (IteratorCompleteException& ce) {
      delete iterator_;
    }
    input_path_index_++;
  }
  tsq_->done(true);
}

/*
 * Close out the Tokyo Cabinet mapper output file
 */
void 
Mapper::end()
{
  int ecode;
  if (!tchdbclose(map_out_db_)) {
    ecode = tchdbecode(map_out_db_);
    fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
  }
  tchdbdel(map_out_db_);
  char* number_emits = human_readable_number(emit_count_);
  char* number_sets = human_readable_number(set_count_);
  printf("Mapper Complete: %s Emits %s Sets\n", number_emits, number_sets);
  free(number_emits);
  free(number_sets);
  if (env_->optimize_bucket_count) {
    char* file_size_estimate = human_readable_filesize(emit_size_estimate_);
    printf("Estimated Map File Size: %s\n",file_size_estimate);
    free(file_size_estimate);
  }
}

KeyValuePair*
Mapper::next()
{
  return tsq_->dequeue();
}

/*
 * When Meguro.emit is called from the JS, we look for the key inside a hash
 * table.  If it exists then the key count is incremented and a new key is
 * created for use in the tokyo cabinet.  For example, if the key 'hello' is
 * emitted, the first time it is stored as 'hello', but the second time it is
 * stored as 'hello|m1'
 */
void 
Mapper::emit(const string& key, const string& value)
{
  int ecode;
  const char* key_cstr = key.c_str();
  const char* value_cstr = value.c_str();

  emit_count_++;

  if (pthread_mutex_lock(&emit_mutex_) < 0) {
    throw MapperException("Mutex Locking Issue");
  }

  ShadowKeyMap* shad = env_->shadow_key_map;
  char* shadow_key = shad->increment_and_return_shadow_key(key_cstr);

  if (pthread_mutex_unlock(&emit_mutex_) < 0) {
    throw MapperException("Mutex Unlocking Issue");
  }

  if (shadow_key) {
    if(!tchdbput(map_out_db_,shadow_key,strlen(shadow_key), value_cstr, strlen(value_cstr))) {
      ecode = tchdbecode(map_out_db_);
      fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
      throw MapperException("Could not tchdbput");
    }
    free(shadow_key);
  } else 
    throw MapperException("Out of Memory");
}

void 
Mapper::set(const string& key, const string& value)
{
  int ecode;
  const char* key_cstr = key.c_str();
  const char* value_cstr = value.c_str();

  if (pthread_mutex_lock(&emit_mutex_) < 0) {
    throw MapperException("Mutex Locking Issue");
  }

  set_count_++;

  if (pthread_mutex_unlock(&emit_mutex_) < 0) {
    throw MapperException("Mutex Unlocking Issue");
  }

  if(!tchdbput(map_out_db_,key_cstr,strlen(key_cstr), value_cstr, strlen(value_cstr))) {
    ecode = tchdbecode(map_out_db_);
    fprintf(stderr, "put error: %s\n", tchdberrmsg(ecode));
    throw MapperException("Could not tchdbput");
  }
}

/*
 * The emit_noop is used for optimizing the bucket count and memory size
 */
void
Mapper::emit_noop(const string& key, const string& value)
{
  emit_count_++;
  emit_size_estimate_ += key.size() + value.size();
}

//------------------------------------------------
//-- S t a t i c    I m p l e m e n t a t i o n --
//------------------------------------------------
//

/*
 * Guess that it is a tokyo cabinet file based on the naming schema
 */
static Iterator* pick_an_iterator(const MeguroEnvironment* env, const char* filename)
{
  if (env->use_stdin)
    return new StdinIterator();

  char* ext = strrchr((char*) filename,'.');
  if (ext) {
    if (!strcasecmp(ext, ".tch"))
      return new TokyoCabinetHashIterator();
    else if (!strcasecmp(ext,".tcb") || !strcasecmp(ext,".tcf") || !strcasecmp(ext,".tct"))
      return new TokyoCabinetIterator();
    else if(!strcasecmp(ext,".gz") || !strcasecmp(ext,".gzip"))
      return new GzipIterator();
    else
      return new LineIterator();
  } else 
    return new LineIterator();
}
