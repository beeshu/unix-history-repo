// RUN: rm -rf %t 
// RUN: %clang_cc1 -fmodules -fmodules-cache-path=%t -I %S/Inputs/ModuleMapLocations/Module_ModuleMap -I %S/Inputs/ModuleMapLocations/Both -F %S/Inputs/ModuleMapLocations -x objective-c -fsyntax-only %s -verify

// regular
@import module_modulemap;
@import both;
// framework
@import Module_ModuleMap_F;
@import Module_ModuleMap_F.Private;
@import Both_F;
@import Inferred;

void test() {
  will_be_found1();
  wont_be_found1(); // expected-warning{{implicit declaration of function 'wont_be_found1' is invalid in C99}}
  will_be_found2();
  wont_be_found2(); // expected-warning{{implicit declaration of function 'wont_be_found2' is invalid in C99}}
}
