#ifndef __ABSTRACT_X_BOSSMinimalExample_1_0_HPP__
#define __ABSTRACT_X_BOSSMinimalExample_1_0_HPP__

#include "forward_decls_abstract_classes.hpp"
#include "identification.hpp"

namespace CAT_3(BACKENDNAME,_,SAFE_VERSION)
{

  namespace nspace1
  {

    namespace nspace2
    {

      class Abstract_X
      {
        public:

            virtual int& i_ref_GAMBIT() =0;

        public:
            virtual void pointerAssign_GAMBIT(Abstract_X*) =0;
            virtual Abstract_X* pointerCopy_GAMBIT() =0;
            virtual ~Abstract_X() {};
      };

    }

  }

}

#include "backend_undefs.hpp"

#endif /* __ABSTRACT_X_BOSSMinimalExample_1_0_HPP__ */
