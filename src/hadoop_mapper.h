#ifndef __MEGURO_HADOOP_MAPPER_H__
#define __MEGURO_HADOOP_MAPPER_H__

#include "mapper.h"

/*
 * The Hadoop Mapper does not use Tokyo Cabinet to store emissions.  It outputs
 * them in the format key\tvalue to stdout.
 */
namespace meguro {
  
  class HadoopMapper : public meguro::Mapper {
    public:
      HadoopMapper(const MeguroEnvironment* env);
      virtual ~HadoopMapper();

      void emit(const string& key, const string& value);
      void emit_noop(const string& key, const string& value);
      
      /* In the Hadoop Mapper, set does the same thing as emit behind the scenes */
      void set(const string& key, const string& value);

      void begin() {};
      void end() {};
  };
}

#endif
