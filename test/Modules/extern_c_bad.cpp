// RUN: rm -rf %t
// RUN: %clang_cc1 -verify -fmodules -x c++ -emit-module -fmodules-cache-path=%t -fmodule-name=c_library_bad %S/Inputs/module.map
