#ifndef __MEGURO_ENVIRONMENT__
#define __MEGURO_ENVIRONMENT__

#include <stdint.h>
#include <vector>

using namespace std;

#define DEFAULT_BUCKET_NUM 1000000L
#define DEFAULT_VM_SIZE 100000000L

/*
 * This is the overall environment that we pass around between the
 * mapper/reducers and initializers, so that various settings can be in there.
 */
namespace meguro {
  class Mapper;
  class ShadowKeyMap;
  class Reducer;
  class Progress;
  class Dictionary;

  typedef enum {TOKYOCABINET_OUTPUT = 0, STDIO_OUTPUT} OutputType;

  struct MeguroEnvironment {
    vector<char*> input_paths;
    char* map_out_path;
    char* reduce_out_path;
    char* js_file;
    char* key_pattern;
    uint64_t map_mem_size;
    Mapper* mapper;
    Reducer* reducer;
    Dictionary* dictionary;
    ShadowKeyMap* shadow_key_map;
    bool do_map;
    bool do_reduce;
    bool verbose_progress;
    bool map_bzip2;
    bool optimize_bucket_count;
    bool just_reduce; 
    bool use_stdin;
    uint64_t number_of_buckets;
    uint32_t number_of_threads;
    uint64_t runtime_memory_size;
    uint64_t cap_amount;
    bool only_map;
    bool only_reduce;
    int output_type;

    MeguroEnvironment() {
      just_reduce = false;
      do_map = false;
      do_reduce = false;
      map_out_path = 0x0;
      reduce_out_path = 0x0;
      js_file = 0x0;
      mapper = 0x0;
      reducer = 0x0;
      dictionary = 0x0;
      key_pattern = 0x0;
      verbose_progress = false;
      map_bzip2 = false;
      optimize_bucket_count = false;
      number_of_buckets = DEFAULT_BUCKET_NUM;
      shadow_key_map = NULL;
      number_of_threads = 2;
      runtime_memory_size = DEFAULT_VM_SIZE;
      cap_amount = 0;
      use_stdin = false;
      only_map = false;
      only_reduce = false;
      output_type = TOKYOCABINET_OUTPUT;
    }
  };
}

#endif
