#include <string.h>
#include <tcutil.h>
#include <tchdb.h>
#include <tcbdb.h>
#include <vector>

#include "tc_iterator.h"
#include "base64.h"

using namespace std;

void mg_tch_iterator(const char* filename,void(* fn_ptr)(char* key, char* value, void* traveler), void* traveler)
{
  int ecode;
  TCHDB* hdb = tchdbnew();
  if(!tchdbtune(hdb,-1,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  /*
  if (!tchdbsetxmsiz(hdb,TC_MEMORY_SIZE)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "setxmsiz error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }
  */

  if(!tchdbopen(hdb, filename, HDBOREADER)){
    ecode = tchdbecode(hdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  uint64_t num_records = tchdbrnum(hdb);

  if (!tchdbiterinit(hdb)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "iterator error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  int key_len;
  int val_len;
  char* key;
  uint64_t count = 1;

  fprintf(stdout,"\rMapping: %9d / %d", (int) count, (int) num_records);
  fflush(stdout);

  while((key = (char*) tchdbiternext(hdb,&key_len))) {
    char* value = (char*) tchdbget(hdb,key,key_len,&val_len);
    if (value) {
      fn_ptr(key,value,traveler);
      free(value);
    }
    free(key);

    fprintf(stdout,"\rMapping: %9d / %d", (int) count, (int) num_records);
    fflush(stdout);
    count++;
  }

  fprintf(stdout,"\n");
  fflush(stdout);

  tchdbdel(hdb);
}

void mg_tcb_iterator_with_grouping(const char* filename, void(* fn_ptr)(char* key, vector<char*> values, void* traveler), void* traveler)
{
  int ecode;
  TCBDB* map_out_db = tcbdbnew();
  if(!tcbdbtune(map_out_db,-1,-1,-1,-1,-1,BDBTLARGE)) {
    ecode = tcbdbecode(map_out_db);
    fprintf(stderr, "tune error: %s\n", tcbdberrmsg(ecode));
    exit(-1);
  }

  if (!tcbdbopen(map_out_db,filename, BDBOREADER|BDBONOLCK)) {
    ecode = tcbdbecode(map_out_db);
    fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
    exit(-1);
  }

  uint64_t num_records = tcbdbrnum(map_out_db);
  uint64_t count = 1;
  fprintf(stdout,"\rReducing: %9d / %d", (int) count, (int) num_records);
  fflush(stdout);

  BDBCUR* cur;
  char* cur_key = NULL;
  char* key, *val;
  int region_size = 0;
  cur = tcbdbcurnew(map_out_db);
  tcbdbcurfirst(cur);
  vector<char*> values;
  cur_key = (char*) tcbdbcurkey3(cur,&region_size);

  while((key = (char*) tcbdbcurkey3(cur,&region_size)) != NULL){
    val= (char*) tcbdbcurval3(cur,&region_size);
    if(val){
      if (strcmp(cur_key,key)) {
        fn_ptr(cur_key,values,traveler);
        cur_key = key;
        values.clear();
      }
      values.push_back(val);
    }
    tcbdbcurnext(cur);
    fprintf(stdout,"\rReducing: %9d / %d", (int) count, (int) num_records);
    fflush(stdout);
    count++;
  }

  tcbdbcurdel(cur);

  if (!tcbdbclose(map_out_db)) {
    ecode = tcbdbecode(map_out_db);
    fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
  }

  tcbdbdel(map_out_db);
}

/*
 * This function iterates through a tch database, but converts the value into a
 * set of values using comma delineation.
 */
void mg_base64_tch_iterator_with_grouping(const char* filename, void(* fn_ptr)(char* key, vector<char*> values, void* traveler), void* traveler)
{
  int ecode;
  TCHDB* hdb = tchdbnew();
  if(!tchdbtune(hdb,-1,-1,-1,HDBTLARGE)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "tune error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  if(!tchdbopen(hdb, filename, HDBOREADER)){
    ecode = tchdbecode(hdb);
    fprintf(stderr, "open error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  uint64_t num_records = tchdbrnum(hdb);

  if (!tchdbiterinit(hdb)) {
    ecode = tchdbecode(hdb);
    fprintf(stderr, "iterator error: %s\n", tchdberrmsg(ecode));
    exit(-1);
  }

  int key_len;
  int val_len;
  char* key;
  uint64_t count = 1;

  fprintf(stdout,"\rReducing: %9d / %d", (int) count, (int) num_records);
  fflush(stdout);

  while((key = (char*) tchdbiternext(hdb,&key_len))) {
    char* value = (char*) tchdbget(hdb,key,key_len,&val_len);
    if (value) {
      vector<char*> values;
      int len = strlen(value);
      char* start = value;
      int start_index = 0;
      char* elem = NULL;
      size_t elem_len = 0;
      for(int i=0; i < len; i++) {
        if (value[i] == ',') {
          bool ok = base64_decode_alloc(start,i - start_index,&elem,&elem_len);
          if (!ok) {
            fprintf(stderr,"Invalid Base64 Element\n");
          }
          if (!elem) {
            fprintf(stderr,"Ran out of memory\n");
          }
          elem[elem_len] = 0x0;
          start_index = i + 1;
          start = value + i + 1;
          values.push_back(elem);
        }
      }
      fn_ptr(key,values,traveler);
      for(vector<char*>::const_iterator ii = values.begin(); ii != values.end(); ii++) {
        free(*ii);
      }
      free(value);
    }
    free(key);

    fprintf(stdout,"\rReducing: %9d / %d", (int) count, (int) num_records);
    fflush(stdout);
    count++;
  }

  fprintf(stdout,"\n");
  fflush(stdout);

  tchdbdel(hdb);
}
