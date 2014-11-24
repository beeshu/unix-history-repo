// RUN: %clang_cc1 -analyze -analyzer-checker=core,alpha.core -analyzer-store=region -verify %s
//
// Just exercise the analyzer on code that has at one point caused issues
// (i.e., no assertions or crashes).


static void f1(const char *x, char *y) {
  while (*x != 0) {
    *y++ = *x++;
  }
}

// This following case checks that we properly handle typedefs when getting
// the RvalueType of an ElementRegion.
typedef struct F12_struct {} F12_typedef;
typedef void* void_typedef;
void_typedef f2_helper();
static void f2(void *buf) {
  F12_typedef* x;
  x = f2_helper();
  memcpy((&x[1]), (buf), 1); // expected-warning{{implicitly declaring library function 'memcpy' with type 'void *(void *, const void *}} \
  // expected-note{{include the header <string.h> or explicitly provide a declaration for 'memcpy'}}
}
