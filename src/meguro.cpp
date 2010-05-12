#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <tchdb.h>
#include <getopt.h>
#include <pthread.h>

#include "mapper.h"
#include "reducer.h"
#include "tracemonkey_js_handle.h"
#include "meguro_version.h"
#include "environment.h"
#include "dictionary.h"
#include "shadow_key_map.h"
#include "mgutil.h"

using namespace meguro;

//See https://bugzilla.mozilla.org/show_bug.cgi?id=551247 about why this exists
pthread_mutex_t tracemonkey_init_mutex;

static bool parse_javascript_file(MeguroEnvironment* env);
static bool check_write_permissions(const char* filename);
static void map_master(MeguroEnvironment* env);
static void reduce_master(MeguroEnvironment* env);
static void* threaded_map(void* data);
static void* threaded_reduce(void* data);
static void print_input_files(MeguroEnvironment* env);

int main(int argc, char **argv)
{
  MeguroEnvironment env = MeguroEnvironment();
  int verbose_progress = 0;
  bool opt_list = false;

  struct option long_options[] = {
    {"javascript",    required_argument, 0, 'j'},
    {"map-output",    required_argument, 0, 'm'},
    {"reduce-output", required_argument, 0, 'r'},
    {"mapper-mem",    required_argument, 0, 'a'},
    {"skip-map",      no_argument,       0, 'e'},
    {"bzip",          no_argument,       0, 'b'},
    {"bucket-number", required_argument, 0, 'n'},
    {"runtime-mem",   required_argument, 0, 'x'},
    {"threads",       required_argument, 0, 't'},
    {"optimize",      no_argument,       0, 'o'},
    {"dictionary",    required_argument, 0, 'd'},
    {"verbose",       no_argument,       &verbose_progress, 1},
    {"version",       no_argument,       0, 'v'},
    {"key",           required_argument, 0, 'k'},
    {"help",          no_argument,       0, 'h'},
    {"list",          no_argument,       0, 'l'},
    {"cap",           required_argument, 0, 'c'},
    {0,0,0,0}
  };


  int c = 0;
  const char* usage = "Usage: meguro [OPTION]... [FILE]...\n\
Run a javascript map reduce task on a file or set of files\n\
Example: meguro -j my.js input.tch\n\n\
Required Options:\n\
  -j : path to javascript map/reduce file\n\
  \n\
Output File Options\n\
  -m : path to tokyo cabinet map output file (default: map.out)\n\
  -r : path to tokyo cabinet reduce output file (default: reduce.out)\n\
  \n\
Tuning Options\n\
  -a/--mapper-mem    : mmap size for the mapper to use (ie. 100M, 2G) (default: 64M)\n\
  -b/--bzip          : bzip2 mapper output (useful for big output)\n\
  -n/--bucket-number : number of map buckets (default 10M)\n\
  -o/--optimize      : print the bucket count by doing map just iteration and no output\n\
  -t/--threads       : number of threads\n\
  -x/--runtime-mem   : javascript runtime memory size\n\
  \n\
Misc. Options\n\
  -c/--cap           : cap the reducer values at some arbitrary number\n\
  -d/--dictionary    : optional tokyo cabinet dictionary key/value store\n\
                     : accessible via Meguro.dictionary('key') inside the JS\n\
  -e/--skip-map      : just reduce, this means the input file is the output of another map\n\
  -h/--help          : show this message\n\
  -k/--key           : just run the map/reduce on the one key\n\
  --verbose          : show verbose progress\n\
  -v/--version       : show the version\n\
  \n\
Output File Options\n\
  -l/--list          : list a map or reduce output files contents in a key<TAB>value format\n\
\n\
Report bugs at http://github.com/jubos/meguro\n";

  char* dictionary_file = NULL;
  char* mmap_size = (char*) "64M";

  if (pthread_mutex_init(&tracemonkey_init_mutex,NULL) < 0) {
    fprintf(stderr,"Mutex Init Error");
    exit(-1);
  }

  int option_index = 0;
  while(1) {
    c = getopt_long(argc, argv, "a:bc:m:r:j:t:hvd:k:pon:ex:l", long_options, &option_index);

    if (c == -1)
      break;

    switch (c) {
      case 0:
        break;
      case 'b':
        env.map_bzip2 = true;
        break;
      case 'c':
        env.cap_amount = atoll(optarg);
        break;
      case 'e':
        env.just_reduce = true;
        break;
      case 'r':
        env.reduce_out_path = optarg;
        break;
      case 'm':
        env.map_out_path = optarg;
        break;
      case 'x':
        env.runtime_memory_size = size_from_string(optarg);
        break;
      case 'j':
        env.js_file = optarg;
        break;
      case 'n':
        env.number_of_buckets = number_from_string(optarg);
        break;
      case 't':
        env.number_of_threads = atoi(optarg);
        break;
      case 'd':
        dictionary_file = optarg;
        break;
      case 'k':
        env.key_pattern = optarg;
        break;
      case 'a':
        mmap_size = optarg;
        break;
      case 'o':
        env.optimize_bucket_count = true;
        break;
      case 'l':
        opt_list = true;
        break;
      case ':':
        printf("Usage error\n");
        exit(-2);
        break;
      case 'v':
        printf("Meguro %s\nCFLAGS:%s\n",MEGURO_VERSION,MEGURO_CFLAGS);
        exit(0);
        break;
      case 'h':
        printf("%s",usage);
        exit(-1);
        break;
      case '?':
        printf("%s",usage);
        exit(-2);
        break;
    }
  }
  
  env.verbose_progress = (verbose_progress == 1);

  for(int index = optind; index < argc; index++) {
    env.input_paths.push_back(argv[index]);
  }

  if (env.input_paths.size() == 0) {
    fprintf(stderr,"You must specify at least one input file\n");
    fprintf(stderr,usage);
    exit(-1);
  }

  if (opt_list) {
    print_input_files(&env);
    exit(0);
  }

  if (!env.js_file) {
    fprintf(stderr,"You must specify a js file\n");
    fprintf(stderr,usage);
    exit(-1);
  }

  if (!env.reduce_out_path)
    env.reduce_out_path = (char*) "reduce.out";

  if (!env.map_out_path) {
    env.map_out_path = (char*) "map.out";
  }

  env.map_mem_size = size_from_string(mmap_size);

  if (env.just_reduce)
    env.map_out_path = env.input_paths[0];

  if (!check_write_permissions(env.map_out_path)) {
    fprintf(stderr,"You do not have write permissions to map output file\n");
    exit(-1);
  }

  if (!check_write_permissions(env.reduce_out_path)) {
    fprintf(stderr,"You do not have write permissions to reduce output file\n");
    exit(-1);
  }

  if (!parse_javascript_file(&env)) {
    fprintf(stderr,"Error parsing your javascript file\n");
    exit(-1);
  }

  fprintf(stderr,"---------------Meguro------------------\n");
  if (env.js_file)
    fprintf(stderr,"Javascript: %s\n", env.js_file);
  if (env.do_map)
    fprintf(stderr,"Mapper output: %s\n",env.map_out_path);
  if (env.do_reduce)
    fprintf(stderr,"Reducer output: %s\n",env.reduce_out_path);
  if (env.map_bzip2)
    fprintf(stderr,"Bzip2: true\n");
  if (env.cap_amount)
    fprintf(stderr,"Cap:%llu\n", (unsigned long long) env.cap_amount);
  if (dictionary_file)
    fprintf(stderr,"Dictionary: %s\n",dictionary_file);
  fprintf(stderr,"Number of threads: %d\n",env.number_of_threads);
  char* runtime_memsize_str = human_readable_filesize(env.runtime_memory_size);
  if (runtime_memsize_str) {
    fprintf(stderr,"Javascript runtime memory size: %s\n",runtime_memsize_str);
    free(runtime_memsize_str);
  }
  char* bucket_num_str = human_readable_number(env.number_of_buckets);
  if (bucket_num_str) {
    fprintf(stderr,"Mapper buckets: %s\n",bucket_num_str);
    free(bucket_num_str);
  }
  char* map_mem_size_str = human_readable_filesize(env.map_mem_size);
  if (map_mem_size_str) {
    fprintf(stderr,"Mapper memory size: %s\n",map_mem_size_str);
    free(map_mem_size_str);
  }

  if (dictionary_file) {
    try {
      env.dictionary = new Dictionary();
      env.dictionary->load(&env,dictionary_file);
    } catch (DictionaryException& de) {
      fprintf(stderr, "Dictionary Load Exception\n");
      exit(-1);
    }
  }

  env.shadow_key_map = new ShadowKeyMap(&env);

  bool reduce = false;
  if (env.reduce_out_path)
    reduce = true;


  if (env.just_reduce) {
    if (env.do_reduce) {
      try {
        env.shadow_key_map->load(env.input_paths[0]);
      } catch (ShadowKeyMapException& skme) {
        printf("Error: %s\n", skme.what());
        exit(-1);
      }
    } else {
      fprintf(stderr,"You must specify a reduce function in your .js\n");
      exit(-1);
    }
  } else {
    if (env.do_map)
      map_master(&env);
    else {
      fprintf(stderr, "You must specify a map function in your .js\n");
      exit(-1);
    }
  }

  if (env.do_reduce) {
    reduce_master(&env);
  }

  if (env.dictionary)
    delete env.dictionary;

  delete env.shadow_key_map;

  return 0;
}

// -------------------------------------------
// -- S t a t i c  I m p l e m e n t a t i o n
// -------------------------------------------
static void* threaded_map(void* data)
{
  MeguroEnvironment* env = (MeguroEnvironment*) data;
  if (pthread_mutex_lock(&tracemonkey_init_mutex) < 0) {
    fprintf(stderr, "Tracemonkey Mutex Issue\n");
    exit(-1);
  }

  JSHandle* js = new TraceMonkeyJSHandle(env);

  if (pthread_mutex_unlock(&tracemonkey_init_mutex) < 0) {
    fprintf(stderr, "Tracemonkey Mutex Issue\n");
    exit(-1);
  }

  js->mapper(env->mapper);
  try {
    for(;;) {
      KeyValuePair* pair = env->mapper->next();
      try {
        js->map(pair->key,pair->value);
      } catch (JSHandleException& jse) {
        printf("%s\n", jse.what());
      }
      delete pair;
    }
  } catch (QueueEmptyException qe) {
  }
  delete js;
  return NULL;
}

static void* threaded_reduce(void* data)
{
  MeguroEnvironment* env = (MeguroEnvironment*) data;
  if (pthread_mutex_lock(&tracemonkey_init_mutex) < 0) {
    fprintf(stderr, "Tracemonkey Mutex Issue\n");
    exit(-1);
  }

  JSHandle* js = new TraceMonkeyJSHandle(env);

  if (pthread_mutex_unlock(&tracemonkey_init_mutex) < 0) {
    fprintf(stderr, "Tracemonkey Mutex Issue\n");
    exit(-1);
  }

  js->reducer(env->reducer);
  try {
    for(;;) {
      KeyValueListPair* pair = env->reducer->next();
      js->reduce(pair->key,pair->value_list);
      delete pair;
    }
  } catch (QueueEmptyException qe) {
  }
  delete js;
  return NULL;
}


/*
 * Setup all the threads to run the map tasks
 */
static void map_master(MeguroEnvironment* env)
{
  pthread_t tid[env->number_of_threads];
  int error;
  Mapper* mapper = new Mapper(env);
  env->mapper = mapper;

  for(unsigned int i=0; i< env->number_of_threads; i++) {
    error = pthread_create(&tid[i], NULL,threaded_map,env);
    if(0 != error)
      fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, error);
  }

  mapper->begin();

  for(unsigned int i=0; i< env->number_of_threads; i++) {
    error = pthread_join(tid[i], NULL);
  }

  mapper->end();
  delete mapper;
}

static void reduce_master(MeguroEnvironment* env)
{
  pthread_t tid[env->number_of_threads];
  int error;
  Reducer* reducer = new Reducer(env);
  env->reducer = reducer;

  for(unsigned int i=0; i< env->number_of_threads; i++) {
    error = pthread_create(&tid[i], NULL,threaded_reduce,env);
    if(0 != error)
      fprintf(stderr, "Couldn't run thread number %d, errno %d\n", i, error);
  }
  reducer->begin();
  for(unsigned int i=0; i< env->number_of_threads; i++) {
    error = pthread_join(tid[i], NULL);
  }
  reducer->end();
  delete reducer;
}

/*
 * Do an fopen on the output files to ensure that we have write permissions We
 * don't want to error out after running the map.
 */
static bool check_write_permissions(const char* filename)
{
  int filedes = open(filename, O_CREAT, S_IRWXU);
  if (filedes > 0) {
    close(filedes);
    unlink(filename);
    return true;
  } else {
    fprintf(stderr,"Error writing to %s\n", filename);
    return false;
  }
}

/*
 * This parses the javascript input file, ensures that it compiles correctly.
 * It also looks for a map and reduce function and sets the booleans
 * appropriately in the environment.
 */
static bool parse_javascript_file(MeguroEnvironment* env)
{
  try { 
    JSHandle* js = new TraceMonkeyJSHandle(env);
    if (js->has_map_fn()) 
      env->do_map = true;
    if (js->has_reduce_fn()) 
      env->do_reduce= true;
    return true;
  } catch (JSHandleException& e) {
    fprintf(stderr,"%s\n",e.what());
    return false;
  }
}

static int printdata(const char *ptr, int size, bool px){
  int len = 0;
  while(size-- > 0){
    if(px){
      if(len > 0) putchar(' ');
      len += printf("%02X", *(unsigned char *)ptr);
    } else {
      putchar(*ptr);
      len++;
    }
    ptr++;
  }
  return len;
}

/*
 * This function just uses the iterator to print out the key/values of an input
 * file This is useful in just letting the user see the output without diving
 * into tchmgr.
 */
static void print_input_files(MeguroEnvironment* env)
{
  for(vector<char*>::const_iterator ii = env->input_paths.begin(); ii != env->input_paths.end(); ii++) {
    int ecode;
    char* path = *ii;
    TCHDB* tchdb = tchdbnew();

    if(!tchdbopen(tchdb,path,HDBOREADER | HDBONOLCK)) {
      ecode = tchdbecode(tchdb);
      fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
      return;
    }

    if(!tchdbiterinit(tchdb)) {
      ecode = tchdbecode(tchdb);
      fprintf(stderr, "iterator error: %s\n", tchdberrmsg(ecode));
      return;
    }

    TCXSTR *key = tcxstrnew();
    TCXSTR *val = tcxstrnew();
    while(tchdbiternext3(tchdb, key, val)){
      printdata((const char*)tcxstrptr(key), tcxstrsize(key), false);
      putchar('\t');
      printdata((const char*) tcxstrptr(val), tcxstrsize(val), false);
      putchar('\n');
    }
    tcxstrdel(val);
    tcxstrdel(key);

    if (!tchdbclose(tchdb)) {
      ecode = tchdbecode(tchdb);
      fprintf(stderr, "close error: %s\n", tchdberrmsg(ecode));
    }
  }
}
