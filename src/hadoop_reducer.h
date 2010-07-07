#ifndef __MEGURO_HADOOP_REDUCER_H__
#define __MEGURO_HADOOP_REDUCER_H__

#include "reducer.h"

namespace meguro {
  class HadoopReducer : public Reducer {
    void save(const string& key, const string& value);
  };
}

#endif
