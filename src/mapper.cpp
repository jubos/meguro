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
 * Iterate through the input iterator and put key/value pairs into the thread
 * safe queue that the executable will pull off and give to each mapper JS
 * thread.
 */
void
Mapper::iterate()
{
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

KeyValuePair*
Mapper::next()
{
  return tsq_->dequeue();
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
