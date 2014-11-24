// RUN: %clang_cc1 -verify %s 

kernel __attribute__((vec_type_hint)) void kernel1() {} //expected-error{{'vec_type_hint' attribute takes one argument}}

kernel __attribute__((vec_type_hint(not_type))) void kernel2() {} //expected-error{{unknown type name 'not_type'}}

kernel __attribute__((vec_type_hint(void))) void kernel3() {} //expected-error{{invalid attribute argument 'void' - expecting a vector or vectorizable scalar type}}

kernel __attribute__((vec_type_hint(bool))) void kernel4() {} //expected-error{{invalid attribute argument 'bool' - expecting a vector or vectorizable scalar type}}

kernel __attribute__((vec_type_hint(int))) __attribute__((vec_type_hint(float))) void kernel5() {} //expected-warning{{attribute 'vec_type_hint' is already applied with different parameters}}

kernel __attribute__((work_group_size_hint(8,16,32,4))) void kernel6() {} //expected-error{{'work_group_size_hint' attribute requires exactly 3 arguments}}

kernel __attribute__((work_group_size_hint(1,2,3))) __attribute__((work_group_size_hint(3,2,1))) void kernel7() {}  //expected-warning{{attribute 'work_group_size_hint' is already applied with different parameters}}

__attribute__((reqd_work_group_size(8,16,32))) void kernel8(){} // expected-error {{attribute 'reqd_work_group_size' can only be applied to a kernel}}

__attribute__((work_group_size_hint(8,16,32))) void kernel9(){} // expected-error {{attribute 'work_group_size_hint' can only be applied to a kernel}}

__attribute__((vec_type_hint(char))) void kernel10(){} // expected-error {{attribute 'vec_type_hint' can only be applied to a kernel}}

constant int foo1 __attribute__((reqd_work_group_size(8,16,32))) = 0; // expected-error {{'reqd_work_group_size' attribute only applies to functions}}

constant int foo2 __attribute__((work_group_size_hint(8,16,32))) = 0; // expected-error {{'work_group_size_hint' attribute only applies to functions}}

constant int foo3 __attribute__((vec_type_hint(char))) = 0; // expected-error {{'vec_type_hint' attribute only applies to functions}}

void f_kernel_image2d_t( kernel image2d_t image ) { // expected-error {{'kernel' attribute only applies to functions}}
  int __kernel x; // expected-error {{'__kernel' attribute only applies to functions}}
  read_only int i; // expected-error {{'read_only' attribute only applies to parameters}}
  __write_only int j; // expected-error {{'__write_only' attribute only applies to parameters}}
}

kernel __attribute__((reqd_work_group_size(1,2,0))) void kernel11(){} // expected-error {{'reqd_work_group_size' attribute must be greater than 0}}
kernel __attribute__((reqd_work_group_size(1,0,2))) void kernel12(){} // expected-error {{'reqd_work_group_size' attribute must be greater than 0}}
kernel __attribute__((reqd_work_group_size(0,1,2))) void kernel13(){} // expected-error {{'reqd_work_group_size' attribute must be greater than 0}}
