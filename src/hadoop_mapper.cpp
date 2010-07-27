#include "hadoop_mapper.h"
#include <stdio.h>

using namespace meguro;

HadoopMapper::HadoopMapper(const MeguroEnvironment* env) : Mapper(env) {}

void 
HadoopMapper::emit(const string& key, const string& value)
{
  emit_count_++;
  printf("%s\t%s\n",key.c_str(),value.c_str());
}

void 
HadoopMapper::emit_noop(const string& key, const string& value)
{
  emit(key,value);
}

void 
HadoopMapper::set(const string& key, const string& value)
{
  emit(key,value);
}
