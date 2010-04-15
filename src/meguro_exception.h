#ifndef __MEGURO_EXCEPTION_H__
#define __MEGURO_EXCEPTION_H__

#include <exception>

using namespace std;

namespace meguro {
  class MeguroException : exception {
    public:
      MeguroException(const char* what) {
        what_ = what;
      }

      const char* what() {
        return what_;
      }
    protected:
      const char* what_;
  };
}


#endif
