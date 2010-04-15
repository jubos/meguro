#ifndef __MEGURO_TYPES_H__
#define __MEGURO_TYPES_H__

#include <vector>

using namespace std;


namespace meguro {
  struct KeyValuePair {
    public:
      char* key;
      char* value;

      KeyValuePair(char* key,char* value) {
        this->key = key;
        this->value = value;
      }
      
      ~KeyValuePair() {
        free(key);
        free(value);
      }
  };

  typedef vector<char*> ValueList;

  struct KeyValueListPair {
    public:
      char* key;
      vector<char*> value_list;

      ~KeyValueListPair() {
        free(key);
        for(ValueList::const_iterator ii = value_list.begin(), end = value_list.end(); ii != end; ii++) {
          char* value = *ii;
          free(value);
        }
        value_list.clear();
      }
  };
}

#endif
