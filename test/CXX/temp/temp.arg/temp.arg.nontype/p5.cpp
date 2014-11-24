// RUN: %clang_cc1 -fsyntax-only -verify %s

// C++0x [temp.arg.nontype] p5:
//   The following conversions are performed on each expression used as
//   a non-type template-argument. If a non-type template-argument cannot be
//   converted to the type of the corresponding template-parameter then the
//   program is ill-formed.
//     -- for a non-type template-parameter of integral or enumeration type,
//        integral promotions (4.5) and integral conversions (4.7) are applied.
namespace integral_parameters {
  template<short s> struct X0 { };
  X0<17> x0i;
  X0<'a'> x0c;
  template<char c> struct X1 { };
  X1<100l> x1l;
}

//     -- for a non-type template-parameter of type pointer to object,
//        qualification conversions (4.4) and the array-to-pointer conversion
//        (4.2) are applied; if the template-argument is of type
//        std::nullptr_t, the null pointer conversion (4.10) is applied.
namespace pointer_to_object_parameters {
  // PR6226
  struct Str {
    Str(const char *);
  };

  template<const char *s>
  struct A {
    Str get() { return s; }
  };

  char hello[6] = "Hello";
  extern const char world[6];
  const char world[6] = "world";
  void test() {
    (void)A<hello>().get();
    (void)A<world>().get();
  }

  class X {
  public:
    X();
    X(int, int);
    operator int() const;
  };
  
  template<X const *Ptr> struct A2; // expected-note{{template parameter is declared here}}
  
  X *X_ptr;
  X an_X;
  X array_of_Xs[10];
  A2<X_ptr> *a12; // expected-error{{must have its address taken}}
  A2<array_of_Xs> *a13;
  A2<&an_X> *a13_2;
  A2<(&an_X)> *a13_3; // expected-warning{{address non-type template argument cannot be surrounded by parentheses}}

  // PR6244
  struct X1 {} X1v;
  template <X1*> struct X2 { };
  template <X1* Value> struct X3 : X2<Value> { };
  struct X4 : X3<&X1v> { };

  // PR6563
  int *bar;
  template <int *> struct zed {}; // expected-note 2{{template parameter is declared here}}
  void g(zed<bar>*); // expected-error{{must have its address taken}}

  int baz;
  void g2(zed<baz>*); // expected-error{{must have its address taken}}

  void g3(zed<&baz>*); // okay
}

//     -- For a non-type template-parameter of type reference to object, no
//        conversions apply. The type referred to by the reference may be more
//        cv-qualified than the (otherwise identical) type of the
//        template-argument. The template-parameter is bound directly to the
//        template-argument, which shall be an lvalue.
namespace reference_parameters {
  template <int& N> struct S0 { }; // expected-note 3 {{template parameter is declared here}}
  template <const int& N> struct S1 { }; // expected-note 2 {{template parameter is declared here}}
  template <volatile int& N> struct S2 { }; // expected-note 2 {{template parameter is declared here}}
  template <const volatile int& N> struct S3 { };
  int i;
  extern const int ci;
  volatile int vi;
  extern const volatile int cvi;
  void test() {
    S0<i> s0;
    S0<ci> s0c; // expected-error{{reference binding of non-type template parameter of type 'int &' to template argument of type 'const int' ignores qualifiers}}
    S0<vi> s0v; // expected-error{{reference binding of non-type template parameter of type 'int &' to template argument of type 'volatile int' ignores qualifiers}}
    S0<cvi> s0cv; // expected-error{{reference binding of non-type template parameter of type 'int &' to template argument of type 'const volatile int' ignores qualifiers}}

    S1<i> s1;
    S1<ci> s1c;
    S1<vi> s1v; // expected-error{{reference binding of non-type template parameter of type 'const int &' to template argument of type 'volatile int' ignores qualifiers}}
    S1<cvi> s1cv; // expected-error{{reference binding of non-type template parameter of type 'const int &' to template argument of type 'const volatile int' ignores qualifiers}}

    S2<i> s2;
    S2<ci> s2c; // expected-error{{reference binding of non-type template parameter of type 'volatile int &' to template argument of type 'const int' ignores qualifiers}}
    S2<vi> s2v;
    S2<cvi> s2cv; // expected-error{{reference binding of non-type template parameter of type 'volatile int &' to template argument of type 'const volatile int' ignores qualifiers}}

    S3<i> s3;
    S3<ci> s3c;
    S3<vi> s3v;
    S3<cvi> s3cv;
  }
  
  namespace PR6250 {
    template <typename T, const T &ref> void inc() {
      ref++; // expected-error{{read-only variable is not assignable}}
    }
  
    template<typename T, const T &ref> void bind() {
      T &ref2 = ref; // expected-error{{drops qualifiers}}
    }
    
    int counter;
    void test() {
      inc<int, counter>(); // expected-note{{instantiation of}}
      bind<int, counter>(); // expected-note{{instantiation of}}
    }
  }

  namespace PR6749 {
    template <int& i> struct foo {}; // expected-note{{template parameter is declared here}}
    int x, &y = x;
    foo<y> f; // expected-error{{is not an object}}
  }
}

//     -- For a non-type template-parameter of type pointer to function, the
//        function-to-pointer conversion (4.3) is applied; if the
//        template-argument is of type std::nullptr_t, the null pointer
//        conversion (4.10) is applied. If the template-argument represents
//        a set of overloaded functions (or a pointer to such), the matching
//        function is selected from the set (13.4).
namespace pointer_to_function {
  template<int (*)(int)> struct X0 { }; // expected-note 3{{template parameter is declared here}}
  int f(int);
  int f(float);
  int g(float);
  int (*funcptr)(int);
  void x0a(X0<f>);
  void x0b(X0<&f>);
  void x0c(X0<g>); // expected-error{{non-type template argument of type 'int (float)' cannot be converted to a value of type 'int (*)(int)'}}
  void x0d(X0<&g>); // expected-error{{non-type template argument of type 'int (*)(float)' cannot be converted to a value of type 'int (*)(int)'}}
  void x0e(X0<funcptr>); // expected-error{{must have its address taken}}
}

//     -- For a non-type template-parameter of type reference to function, no
//        conversions apply. If the template-argument represents a set of
//        overloaded functions, the matching function is selected from the set
//        (13.4).
namespace reference_to_function {
  template<int (&)(int)> struct X0 { }; // expected-note 4{{template parameter is declared here}}
  int f(int);
  int f(float);
  int g(float);
  int (*funcptr)(int);
  void x0a(X0<f>);
  void x0b(X0<&f>); // expected-error{{address taken in non-type template argument for template parameter of reference type 'int (&)(int)'}}
  void x0c(X0<g>); // expected-error{{non-type template parameter of reference type 'int (&)(int)' cannot bind to template argument of type 'int (float)'}}
  void x0d(X0<&g>); // expected-error{{address taken in non-type template argument for template parameter of reference type 'int (&)(int)'}}
  void x0e(X0<funcptr>); // expected-error{{non-type template parameter of reference type 'int (&)(int)' cannot bind to template argument of type 'int (*)(int)'}}
}
//     -- For a non-type template-parameter of type pointer to member function,
//        if the template-argument is of type std::nullptr_t, the null member
//        pointer conversion (4.11) is applied; otherwise, no conversions
//        apply. If the template-argument represents a set of overloaded member
//        functions, the matching member function is selected from the set
//        (13.4).
namespace pointer_to_member_function {
  struct X { };
  struct Y : X { 
    int f(int);
    int g(int);
    int g(float);
    float h(float);
  };

  template<int (Y::*)(int)> struct X0 {}; // expected-note{{template parameter is declared here}}
  X0<&Y::f> x0a;
  X0<&Y::g> x0b;
  X0<&Y::h> x0c; // expected-error-re{{non-type template argument of type 'float (pointer_to_member_function::Y::*)(float){{( __attribute__\(\(thiscall\)\))?}}' cannot be converted to a value of type 'int (pointer_to_member_function::Y::*)(int){{( __attribute__\(\(thiscall\)\))?}}'}}
}

//     -- For a non-type template-parameter of type pointer to data member,
//        qualification conversions (4.4) are applied; if the template-argument
//        is of type std::nullptr_t, the null member pointer conversion (4.11)
//        is applied.
namespace pointer_to_member_data {
  struct X { int x; };
  struct Y : X { int y; };

  template<int Y::*> struct X0 {}; // expected-note{{template parameter is declared here}}
  X0<&Y::y> x0a;
  X0<&Y::x> x0b;  // expected-error{{non-type template argument of type 'int pointer_to_member_data::X::*' cannot be converted to a value of type 'int pointer_to_member_data::Y::*'}}

  // Test qualification conversions
  template<const int Y::*> struct X1 {};
  X1<&Y::y> x1a;
}
