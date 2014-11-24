// Test this without pch.
// RUN: %clang_cc1 %s -include %s -verify -fsyntax-only

// Test with pch.
// RUN: %clang_cc1 %s -emit-pch -o %t
// RUN: %clang_cc1 %s -emit-llvm -include-pch %t -o - | FileCheck %s

// expected-no-diagnostics

#ifndef HEADER
#define HEADER
#pragma clang optimize off

#else

int a;

void f() {
  a = 12345;
}

// Check that the function is decorated with optnone

// CHECK-DAG: @f() [[ATTRF:#[0-9]+]]
// CHECK-DAG: attributes [[ATTRF]] = { {{.*}}noinline{{.*}}optnone{{.*}} }

#endif
