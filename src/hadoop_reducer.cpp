#include "hadoop_reducer.h"

using namespace meguro;

void 
HadoopReducer::save(const string& key, const string& value)
{
  printf("%s\t%s\n",key.c_str(),value.c_str());
}
