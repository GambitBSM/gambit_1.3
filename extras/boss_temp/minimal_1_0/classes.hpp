#ifndef __CLASSES_HPP__
#define __CLASSES_HPP__

#include "abstracts.hpp"

namespace nspace1
{
  namespace nspace2
  {
    class X : public virtual Abstract_X 
    {
      public:
        
        int i;

        X() : i(0) {}

        X(int i_in) : i(i_in) {}

      public:
        Abstract_X* pointerCopy_GAMBIT() { return new X(*this); }
        void pointerAssign_GAMBIT(Abstract_X* in) { *this = *dynamic_cast<X*>(in); }

      public:
        int& i_ref_GAMBIT() { return i; }

    };
  }
}


namespace nspace3
{
  class Y : public virtual Abstract_Y
  {
    public:

        nspace1::nspace2::X x;

        Y() {}

        Y(nspace1::nspace2::X x_in) : x(x_in)
        {
            x = x_in;
        }

        nspace1::nspace2::X get_x()
        {
            return x;
        }

        void set_x(nspace1::nspace2::X& x_in)
        {
            x = x_in;
        }

    public:
        Abstract_Y* pointerCopy_GAMBIT() { return new Y(*this); }
        void pointerAssign_GAMBIT(Abstract_Y* in) { *this = *reinterpret_cast<Y*>(in); }

    public:
        nspace1::nspace2::Abstract_X& x_ref_GAMBIT() { return x; }


    public:
        nspace1::nspace2::Abstract_X* get_x_GAMBIT()
        {
            return new nspace1::nspace2::X(get_x());
        }

        void set_x_GAMBIT(nspace1::nspace2::Abstract_X& x_in)
        {
            set_x(reinterpret_cast< nspace1::nspace2::X& >(x_in));
        }

  };
}

#endif // __CLASSES_HPP__
