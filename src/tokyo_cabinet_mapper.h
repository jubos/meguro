#ifndef __MEGURO_TOKYO_CABINET_MAPPER_H_
#define __MEGURO_TOKYO_CABINET_MAPPER_H_

#include "mapper.h"

/*
 * The Tokyo Cabinet Mapper stores the map results in Tokyo Cabinet as the
 * intermediate store
 */
namespace meguro {
  
  class TokyoCabinetMapper: public meguro::Mapper {
    public:
      TokyoCabinetMapper(const MeguroEnvironment* env);
      virtual ~TokyoCabinetMapper();

      void emit(const string& key, const string& value);
      void emit_noop(const string& key, const string& value);
      
      /* In the Hadoop Mapper, set does the same thing as emit behind the scenes */
      void set(const string& key, const string& value);

      void begin();
      void end();
    protected:
      const char* map_out_path_;
      TCHDB* map_out_db_;
      int map_count_;

  };
}

#endif
