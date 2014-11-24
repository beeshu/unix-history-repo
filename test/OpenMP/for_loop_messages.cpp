// RUN: %clang_cc1 -fsyntax-only -fopenmp=libiomp5 -x c++ -std=c++11 -fexceptions -fcxx-exceptions -verify %s

class S {
  int a;
  S() : a(0) {}

public:
  S(int v) : a(v) {}
  S(const S &s) : a(s.a) {}
};

static int sii;
#pragma omp threadprivate(sii) // expected-note {{defined as threadprivate or thread local}}

int test_iteration_spaces() {
  const int N = 100;
  float a[N], b[N], c[N];
  int ii, jj, kk;
  float fii;
  double dii;
#pragma omp parallel
#pragma omp for
  for (int i = 0; i < 10; i += 1) {
    c[i] = a[i] + b[i];
  }
#pragma omp parallel
#pragma omp for
  for (char i = 0; i < 10; i++) {
    c[i] = a[i] + b[i];
  }
#pragma omp parallel
#pragma omp for
  for (char i = 0; i < 10; i += '\1') {
    c[i] = a[i] + b[i];
  }
#pragma omp parallel
#pragma omp for
  for (long long i = 0; i < 10; i++) {
    c[i] = a[i] + b[i];
  }
#pragma omp parallel
// expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'double'}}
#pragma omp for
  for (long long i = 0; i < 10; i += 1.5) {
    c[i] = a[i] + b[i];
  }
#pragma omp parallel
#pragma omp for
  for (long long i = 0; i < 'z'; i += 1u) {
    c[i] = a[i] + b[i];
  }
#pragma omp parallel
// expected-error@+2 {{variable must be of integer or random access iterator type}}
#pragma omp for
  for (float fi = 0; fi < 10.0; fi++) {
    c[(int)fi] = a[(int)fi] + b[(int)fi];
  }
#pragma omp parallel
// expected-error@+2 {{variable must be of integer or random access iterator type}}
#pragma omp for
  for (double fi = 0; fi < 10.0; fi++) {
    c[(int)fi] = a[(int)fi] + b[(int)fi];
  }
#pragma omp parallel
// expected-error@+2 {{variable must be of integer or random access iterator type}}
#pragma omp for
  for (int &ref = ii; ref < 10; ref++) {
  }
#pragma omp parallel
// expected-error@+2 {{initialization clause of OpenMP for loop must be of the form 'var = init' or 'T var = init'}}
#pragma omp for
  for (int i; i < 10; i++)
    c[i] = a[i];

#pragma omp parallel
// expected-error@+2 {{initialization clause of OpenMP for loop must be of the form 'var = init' or 'T var = init'}}
#pragma omp for
  for (int i = 0, j = 0; i < 10; ++i)
    c[i] = a[i];

#pragma omp parallel
// expected-error@+2 {{initialization clause of OpenMP for loop must be of the form 'var = init' or 'T var = init'}}
#pragma omp for
  for (; ii < 10; ++ii)
    c[ii] = a[ii];

#pragma omp parallel
// expected-warning@+3 {{expression result unused}}
// expected-error@+2 {{initialization clause of OpenMP for loop must be of the form 'var = init' or 'T var = init'}}
#pragma omp for
  for (ii + 1; ii < 10; ++ii)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+2 {{initialization clause of OpenMP for loop must be of the form 'var = init' or 'T var = init'}}
#pragma omp for
  for (c[ii] = 0; ii < 10; ++ii)
    c[ii] = a[ii];

#pragma omp parallel
// Ok to skip parenthesises.
#pragma omp for
  for (((ii)) = 0; ii < 10; ++ii)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+2 {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'i'}}
#pragma omp for
  for (int i = 0; i; i++)
    c[i] = a[i];

#pragma omp parallel
// expected-error@+3 {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'i'}}
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'i'}}
#pragma omp for
  for (int i = 0; jj < kk; ii++)
    c[i] = a[i];

#pragma omp parallel
// expected-error@+2 {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'i'}}
#pragma omp for
  for (int i = 0; !!i; i++)
    c[i] = a[i];

#pragma omp parallel
// expected-error@+2 {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'i'}}
#pragma omp for
  for (int i = 0; i != 1; i++)
    c[i] = a[i];

#pragma omp parallel
// expected-error@+2 {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'i'}}
#pragma omp for
  for (int i = 0;; i++)
    c[i] = a[i];

#pragma omp parallel
// Ok.
#pragma omp for
  for (int i = 11; i > 10; i--)
    c[i] = a[i];

#pragma omp parallel
// Ok.
#pragma omp for
  for (int i = 0; i < 10; ++i)
    c[i] = a[i];

#pragma omp parallel
// Ok.
#pragma omp for
  for (ii = 0; ii < 10; ++ii)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'ii'}}
#pragma omp for
  for (ii = 0; ii < 10; ++jj)
    c[ii] = a[jj];

#pragma omp parallel
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'ii'}}
#pragma omp for
  for (ii = 0; ii < 10; ++++ii)
    c[ii] = a[ii];

#pragma omp parallel
// Ok but undefined behavior (in general, cannot check that incr
// is really loop-invariant).
#pragma omp for
  for (ii = 0; ii < 10; ii = ii + ii)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'float'}}
#pragma omp for
  for (ii = 0; ii < 10; ii = ii + 1.0f)
    c[ii] = a[ii];

#pragma omp parallel
// Ok - step was converted to integer type.
#pragma omp for
  for (ii = 0; ii < 10; ii = ii + (int)1.1f)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'ii'}}
#pragma omp for
  for (ii = 0; ii < 10; jj = ii + 2)
    c[ii] = a[ii];

#pragma omp parallel
// expected-warning@+3 {{relational comparison result unused}}
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'ii'}}
#pragma omp for
  for (ii = 0; ii<10; jj> kk + 2)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'ii'}}
#pragma omp for
  for (ii = 0; ii < 10;)
    c[ii] = a[ii];

#pragma omp parallel
// expected-warning@+3 {{expression result unused}}
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'ii'}}
#pragma omp for
  for (ii = 0; ii < 10; !ii)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'ii'}}
#pragma omp for
  for (ii = 0; ii < 10; ii ? ++ii : ++jj)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'ii'}}
#pragma omp for
  for (ii = 0; ii < 10; ii = ii < 10)
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'ii' to increase on each iteration of OpenMP for loop}}
#pragma omp for
  for (ii = 0; ii < 10; ii = ii + 0)
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'ii' to increase on each iteration of OpenMP for loop}}
#pragma omp for
  for (ii = 0; ii < 10; ii = ii + (int)(0.8 - 0.45))
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'ii' to increase on each iteration of OpenMP for loop}}
#pragma omp for
  for (ii = 0; (ii) < 10; ii -= 25)
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'ii' to increase on each iteration of OpenMP for loop}}
#pragma omp for
  for (ii = 0; (ii < 10); ii -= 0)
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be negative due to this condition}}
// expected-error@+2 {{increment expression must cause 'ii' to decrease on each iteration of OpenMP for loop}}
#pragma omp for
  for (ii = 0; ii > 10; (ii += 0))
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'ii' to increase on each iteration of OpenMP for loop}}
#pragma omp for
  for (ii = 0; ii < 10; (ii) = (1 - 1) + (ii))
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be negative due to this condition}}
// expected-error@+2 {{increment expression must cause 'ii' to decrease on each iteration of OpenMP for loop}}
#pragma omp for
  for ((ii = 0); ii > 10; (ii -= 0))
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'ii' to increase on each iteration of OpenMP for loop}}
#pragma omp for
  for (ii = 0; (ii < 10); (ii -= 0))
    c[ii] = a[ii];

#pragma omp parallel
// expected-note@+2  {{defined as firstprivate}}
// expected-error@+2 {{loop iteration variable in the associated loop of 'omp for' directive may not be firstprivate, predetermined as private}}
#pragma omp for firstprivate(ii)
  for (ii = 0; ii < 10; ii++)
    c[ii] = a[ii];

#pragma omp parallel
// expected-error@+3 {{unexpected OpenMP clause 'linear' in directive '#pragma omp for'}}
// expected-note@+2  {{defined as linear}}
// expected-error@+2 {{loop iteration variable in the associated loop of 'omp for' directive may not be linear, predetermined as private}}
#pragma omp for linear(ii)
  for (ii = 0; ii < 10; ii++)
    c[ii] = a[ii];

#pragma omp parallel
#pragma omp for private(ii)
  for (ii = 0; ii < 10; ii++)
    c[ii] = a[ii];

#pragma omp parallel
#pragma omp for lastprivate(ii)
  for (ii = 0; ii < 10; ii++)
    c[ii] = a[ii];

#pragma omp parallel
  {
// expected-error@+2 {{loop iteration variable in the associated loop of 'omp for' directive may not be threadprivate or thread local, predetermined as private}}
#pragma omp for
    for (sii = 0; sii < 10; sii += 1)
      c[sii] = a[sii];
  }

#pragma omp parallel
// expected-error@+2 {{statement after '#pragma omp for' must be a for loop}}
#pragma omp for
  for (auto &item : a) {
    item = item + 1;
  }

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'i' to increase on each iteration of OpenMP for loop}}
#pragma omp for
  for (unsigned i = 9; i < 10; i--) {
    c[i] = a[i] + b[i];
  }

  int(*lb)[4] = nullptr;
#pragma omp parallel
#pragma omp for
  for (int(*p)[4] = lb; p < lb + 8; ++p) {
  }

#pragma omp parallel
// expected-warning@+2 {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
#pragma omp for
  for (int a{0}; a < 10; ++a) {
  }

  return 0;
}

// Iterators allowed in openmp for-loops.
namespace std {
struct random_access_iterator_tag {};
template <class Iter>
struct iterator_traits {
  typedef typename Iter::difference_type difference_type;
  typedef typename Iter::iterator_category iterator_category;
};
template <class Iter>
typename iterator_traits<Iter>::difference_type
distance(Iter first, Iter last) { return first - last; }
}
class Iter0 {
public:
  Iter0() {}
  Iter0(const Iter0 &) {}
  Iter0 operator++() { return *this; }
  Iter0 operator--() { return *this; }
  bool operator<(Iter0 a) { return true; }
};
int operator-(Iter0 a, Iter0 b) { return 0; }
class Iter1 {
public:
  Iter1(float f = 0.0f, double d = 0.0) {}
  Iter1(const Iter1 &) {}
  Iter1 operator++() { return *this; }
  Iter1 operator--() { return *this; }
  bool operator<(Iter1 a) { return true; }
  bool operator>=(Iter1 a) { return false; }
};
class GoodIter {
public:
  GoodIter() {}
  GoodIter(const GoodIter &) {}
  GoodIter(int fst, int snd) {}
  GoodIter &operator=(const GoodIter &that) { return *this; }
  GoodIter &operator=(const Iter0 &that) { return *this; }
  GoodIter &operator+=(int x) { return *this; }
  explicit GoodIter(void *) {}
  GoodIter operator++() { return *this; }
  GoodIter operator--() { return *this; }
  bool operator!() { return true; }
  bool operator<(GoodIter a) { return true; }
  bool operator<=(GoodIter a) { return true; }
  bool operator>=(GoodIter a) { return false; }
  typedef int difference_type;
  typedef std::random_access_iterator_tag iterator_category;
};
int operator-(GoodIter a, GoodIter b) { return 0; }
GoodIter operator-(GoodIter a) { return a; }
GoodIter operator-(GoodIter a, int v) { return GoodIter(); }
GoodIter operator+(GoodIter a, int v) { return GoodIter(); }
GoodIter operator-(int v, GoodIter a) { return GoodIter(); }
GoodIter operator+(int v, GoodIter a) { return GoodIter(); }

int test_with_random_access_iterator() {
  GoodIter begin, end;
  Iter0 begin0, end0;
#pragma omp parallel
#pragma omp for
  for (GoodIter I = begin; I < end; ++I)
    ++I;
#pragma omp parallel
// expected-error@+2 {{variable must be of integer or random access iterator type}}
#pragma omp for
  for (GoodIter &I = begin; I < end; ++I)
    ++I;
#pragma omp parallel
#pragma omp for
  for (GoodIter I = begin; I >= end; --I)
    ++I;
#pragma omp parallel
// expected-warning@+2 {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
#pragma omp for
  for (GoodIter I(begin); I < end; ++I)
    ++I;
#pragma omp parallel
// expected-warning@+2 {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
#pragma omp for
  for (GoodIter I(nullptr); I < end; ++I)
    ++I;
#pragma omp parallel
// expected-warning@+2 {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
#pragma omp for
  for (GoodIter I(0); I < end; ++I)
    ++I;
#pragma omp parallel
// expected-warning@+2 {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
#pragma omp for
  for (GoodIter I(1, 2); I < end; ++I)
    ++I;
#pragma omp parallel
#pragma omp for
  for (begin = GoodIter(0); begin < end; ++begin)
    ++begin;
#pragma omp parallel
#pragma omp for
  for (begin = begin0; begin < end; ++begin)
    ++begin;
#pragma omp parallel
// expected-error@+2 {{initialization clause of OpenMP for loop must be of the form 'var = init' or 'T var = init'}}
#pragma omp for
  for (++begin; begin < end; ++begin)
    ++begin;
#pragma omp parallel
#pragma omp for
  for (begin = end; begin < end; ++begin)
    ++begin;
#pragma omp parallel
// expected-error@+2 {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'I'}}
#pragma omp for
  for (GoodIter I = begin; I - I; ++I)
    ++I;
#pragma omp parallel
// expected-error@+2 {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'I'}}
#pragma omp for
  for (GoodIter I = begin; begin < end; ++I)
    ++I;
#pragma omp parallel
// expected-error@+2 {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'I'}}
#pragma omp for
  for (GoodIter I = begin; !I; ++I)
    ++I;
#pragma omp parallel
// expected-note@+3 {{loop step is expected to be negative due to this condition}}
// expected-error@+2 {{increment expression must cause 'I' to decrease on each iteration of OpenMP for loop}}
#pragma omp for
  for (GoodIter I = begin; I >= end; I = I + 1)
    ++I;
#pragma omp parallel
#pragma omp for
  for (GoodIter I = begin; I >= end; I = I - 1)
    ++I;
#pragma omp parallel
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'I'}}
#pragma omp for
  for (GoodIter I = begin; I >= end; I = -I)
    ++I;
#pragma omp parallel
// expected-note@+3 {{loop step is expected to be negative due to this condition}}
// expected-error@+2 {{increment expression must cause 'I' to decrease on each iteration of OpenMP for loop}}
#pragma omp for
  for (GoodIter I = begin; I >= end; I = 2 + I)
    ++I;
#pragma omp parallel
// expected-error@+2 {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'I'}}
#pragma omp for
  for (GoodIter I = begin; I >= end; I = 2 - I)
    ++I;
#pragma omp parallel
#pragma omp for
  for (Iter0 I = begin0; I < end0; ++I)
    ++I;
#pragma omp parallel
// Initializer is constructor without params.
// expected-warning@+2 {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
#pragma omp for
  for (Iter0 I; I < end0; ++I)
    ++I;
  Iter1 begin1, end1;
#pragma omp parallel
#pragma omp for
  for (Iter1 I = begin1; I < end1; ++I)
    ++I;
#pragma omp parallel
// expected-note@+3 {{loop step is expected to be negative due to this condition}}
// expected-error@+2 {{increment expression must cause 'I' to decrease on each iteration of OpenMP for loop}}
#pragma omp for
  for (Iter1 I = begin1; I >= end1; ++I)
    ++I;
#pragma omp parallel
// Initializer is constructor with all default params.
// expected-warning@+2 {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
#pragma omp for
  for (Iter1 I; I < end1; ++I) {
  }
  return 0;
}

template <typename IT, int ST>
class TC {
public:
  int dotest_lt(IT begin, IT end) {
#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'I' to increase on each iteration of OpenMP for loop}}
#pragma omp for
    for (IT I = begin; I < end; I = I + ST) {
      ++I;
    }
#pragma omp parallel
// expected-note@+3 {{loop step is expected to be positive due to this condition}}
// expected-error@+2 {{increment expression must cause 'I' to increase on each iteration of OpenMP for loop}}
#pragma omp for
    for (IT I = begin; I <= end; I += ST) {
      ++I;
    }
#pragma omp parallel
#pragma omp for
    for (IT I = begin; I < end; ++I) {
      ++I;
    }
  }

  static IT step() {
    return IT(ST);
  }
};
template <typename IT, int ST = 0>
int dotest_gt(IT begin, IT end) {
#pragma omp parallel
// expected-note@+3 2 {{loop step is expected to be negative due to this condition}}
// expected-error@+2 2 {{increment expression must cause 'I' to decrease on each iteration of OpenMP for loop}}
#pragma omp for
  for (IT I = begin; I >= end; I = I + ST) {
    ++I;
  }
#pragma omp parallel
// expected-note@+3 2 {{loop step is expected to be negative due to this condition}}
// expected-error@+2 2 {{increment expression must cause 'I' to decrease on each iteration of OpenMP for loop}}
#pragma omp for
  for (IT I = begin; I >= end; I += ST) {
    ++I;
  }

#pragma omp parallel
// expected-note@+3 {{loop step is expected to be negative due to this condition}}
// expected-error@+2 {{increment expression must cause 'I' to decrease on each iteration of OpenMP for loop}}
#pragma omp for
  for (IT I = begin; I >= end; ++I) {
    ++I;
  }

#pragma omp parallel
#pragma omp for
  for (IT I = begin; I < end; I += TC<int, ST>::step()) {
    ++I;
  }
}

void test_with_template() {
  GoodIter begin, end;
  TC<GoodIter, 100> t1;
  TC<GoodIter, -100> t2;
  t1.dotest_lt(begin, end);
  t2.dotest_lt(begin, end);         // expected-note {{in instantiation of member function 'TC<GoodIter, -100>::dotest_lt' requested here}}
  dotest_gt(begin, end);            // expected-note {{in instantiation of function template specialization 'dotest_gt<GoodIter, 0>' requested here}}
  dotest_gt<unsigned, -10>(0, 100); // expected-note {{in instantiation of function template specialization 'dotest_gt<unsigned int, -10>' requested here}}
}

void test_loop_break() {
  const int N = 100;
  float a[N], b[N], c[N];
#pragma omp parallel
#pragma omp for
  for (int i = 0; i < 10; i++) {
    c[i] = a[i] + b[i];
    for (int j = 0; j < 10; ++j) {
      if (a[i] > b[j])
        break; // OK in nested loop
    }
    switch (i) {
    case 1:
      b[i]++;
      break;
    default:
      break;
    }
    if (c[i] > 10)
      break; // expected-error {{'break' statement cannot be used in OpenMP for loop}}

    if (c[i] > 11)
      break; // expected-error {{'break' statement cannot be used in OpenMP for loop}}
  }

#pragma omp parallel
#pragma omp for
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      c[i] = a[i] + b[i];
      if (c[i] > 10) {
        if (c[i] < 20) {
          break; // OK
        }
      }
    }
  }
}

void test_loop_eh() {
  const int N = 100;
  float a[N], b[N], c[N];
#pragma omp parallel
#pragma omp for
  for (int i = 0; i < 10; i++) {
    c[i] = a[i] + b[i];
    try {
      for (int j = 0; j < 10; ++j) {
        if (a[i] > b[j])
          throw a[i];
      }
      throw a[i];
    } catch (float f) {
      if (f > 0.1)
        throw a[i];
      return; // expected-error {{cannot return from OpenMP region}}
    }
    switch (i) {
    case 1:
      b[i]++;
      break;
    default:
      break;
    }
    for (int j = 0; j < 10; j++) {
      if (c[i] > 10)
        throw c[i];
    }
  }
  if (c[9] > 10)
    throw c[9]; // OK

#pragma omp parallel
#pragma omp for
  for (int i = 0; i < 10; ++i) {
    struct S {
      void g() { throw 0; }
    };
  }
}

void test_loop_firstprivate_lastprivate() {
  S s(4);
#pragma omp parallel
#pragma omp for lastprivate(s) firstprivate(s)
  for (int i = 0; i < 16; ++i)
    ;
}

void test_ordered() {
#pragma omp parallel
#pragma omp for ordered ordered // expected-error {{directive '#pragma omp for' cannot contain more than one 'ordered' clause}}
  for (int i = 0; i < 16; ++i)
    ;
}

void test_nowait() {
#pragma omp parallel
#pragma omp for nowait nowait // expected-error {{directive '#pragma omp for' cannot contain more than one 'nowait' clause}}
  for (int i = 0; i < 16; ++i)
    ;
}
