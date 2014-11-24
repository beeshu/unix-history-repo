// RUN: %clang_cc1 -triple %itanium_abi_triple -emit-llvm -o - %s | FileCheck %s
namespace std {
  class type_info;
}

struct X { };

void f() {
  // CHECK: @_ZTS1X = linkonce_odr constant
  // CHECK: @_ZTI1X = linkonce_odr constant 
  (void)typeid(X&);
}
