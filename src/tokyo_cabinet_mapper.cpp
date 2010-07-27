#include "tokyo_cabinet_mapper.h"

#include <tcutil.h>
#include <tcbdb.h>

#include "mgutil.h"
#include "shadow_key_map.h"
#include "uthash.h"

using namespace std;
using namespace meguro;

TokyoCabinetMapper::TokyoCabinetMapper(const MeguroEnvironment* env) : Mapper(env)
{
  map_out_path_= env->map_out_path;
}

/*
 * When Meguro.emit is called from the JS, we look for the key inside a hash
 * table.  If it exists then the key count is incremented and a new key is
 * created for use in the tokyo cabinet.  For example, if the key 'hello' is
 * emitted, the first time it is stored as 'hello', but the second time it is
 * stored as 'hello|m1'
 */
void 
TokyoCabinetMapper::emit(const string& key, const string& value)
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
TokyoCabinetMapper::emit_noop(const string& key, const string& value)
{
  emit_count_++;
  emit_size_estimate_ += key.size() + value.size();
}
    
void 
TokyoCabinetMapper::set(const string& key, const string& value)
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

void 
TokyoCabinetMapper::begin()
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
}


void 
TokyoCabinetMapper::end()
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
