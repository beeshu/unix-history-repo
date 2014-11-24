// RUN: %clang_cc1 -std=c++11 -emit-llvm %s -o - -triple x86_64-linux-gnu | FileCheck %s

int f();
int g();

// CHECK: @a = thread_local global i32 0
thread_local int a = f();
extern thread_local int b;
// CHECK: @c = global i32 0
int c = b;
// CHECK: @_ZL1d = internal thread_local global i32 0
static thread_local int d = g();

struct U { static thread_local int m; };
// CHECK: @_ZN1U1mE = thread_local global i32 0
thread_local int U::m = f();

template<typename T> struct V { static thread_local int m; };
template<typename T> thread_local int V<T>::m = g();

// CHECK: @e = global i32 0
int e = V<int>::m;

// CHECK: @_ZN1VIiE1mE = linkonce_odr thread_local global i32 0

// CHECK: @_ZZ1fvE1n = internal thread_local global i32 0

// CHECK: @_ZGVZ1fvE1n = internal thread_local global i8 0

// CHECK: @_ZZ8tls_dtorvE1s = internal thread_local global
// CHECK: @_ZGVZ8tls_dtorvE1s = internal thread_local global i8 0

// CHECK: @_ZZ8tls_dtorvE1t = internal thread_local global
// CHECK: @_ZGVZ8tls_dtorvE1t = internal thread_local global i8 0

// CHECK: @_ZZ8tls_dtorvE1u = internal thread_local global
// CHECK: @_ZGVZ8tls_dtorvE1u = internal thread_local global i8 0
// CHECK: @_ZGRZ8tls_dtorvE1u_ = internal thread_local global

// CHECK: @_ZGVN1VIiE1mE = linkonce_odr thread_local global i64 0

// CHECK: @__tls_guard = internal thread_local global i8 0

// CHECK: @llvm.global_ctors = appending global {{.*}} @[[GLOBAL_INIT:[^ ]*]]

// CHECK: @_ZTH1a = alias void ()* @__tls_init
// CHECK: @_ZTHL1d = alias internal void ()* @__tls_init
// CHECK: @_ZTHN1U1mE = alias void ()* @__tls_init
// CHECK: @_ZTHN1VIiE1mE = alias linkonce_odr void ()* @__tls_init


// Individual variable initialization functions:

// CHECK: define {{.*}} @[[A_INIT:.*]]()
// CHECK: call i32 @_Z1fv()
// CHECK-NEXT: store i32 {{.*}}, i32* @a, align 4

// CHECK-LABEL: define i32 @_Z1fv()
int f() {
  // CHECK: %[[GUARD:.*]] = load i8* @_ZGVZ1fvE1n, align 1
  // CHECK: %[[NEED_INIT:.*]] = icmp eq i8 %[[GUARD]], 0
  // CHECK: br i1 %[[NEED_INIT]]

  // CHECK: %[[CALL:.*]] = call i32 @_Z1gv()
  // CHECK: store i32 %[[CALL]], i32* @_ZZ1fvE1n, align 4
  // CHECK: store i8 1, i8* @_ZGVZ1fvE1n
  // CHECK: br label
  static thread_local int n = g();

  // CHECK: load i32* @_ZZ1fvE1n, align 4
  return n;
}

// CHECK: define {{.*}} @[[C_INIT:.*]]()
// CHECK: call i32* @_ZTW1b()
// CHECK-NEXT: load i32* %{{.*}}, align 4
// CHECK-NEXT: store i32 %{{.*}}, i32* @c, align 4

// CHECK-LABEL: define weak_odr hidden i32* @_ZTW1b()
// CHECK: br i1 icmp ne (void ()* @_ZTH1b, void ()* null),
// not null:
// CHECK: call void @_ZTH1b()
// CHECK: br label
// finally:
// CHECK: ret i32* @b

// CHECK: define {{.*}} @[[D_INIT:.*]]()
// CHECK: call i32 @_Z1gv()
// CHECK-NEXT: store i32 %{{.*}}, i32* @_ZL1d, align 4

// CHECK: define {{.*}} @[[U_M_INIT:.*]]()
// CHECK: call i32 @_Z1fv()
// CHECK-NEXT: store i32 %{{.*}}, i32* @_ZN1U1mE, align 4

// CHECK: define {{.*}} @[[E_INIT:.*]]()
// CHECK: call i32* @_ZTWN1VIiE1mE()
// CHECK-NEXT: load i32* %{{.*}}, align 4
// CHECK-NEXT: store i32 %{{.*}}, i32* @e, align 4

// CHECK-LABEL: define weak_odr hidden i32* @_ZTWN1VIiE1mE()
// CHECK: call void @_ZTHN1VIiE1mE()
// CHECK: ret i32* @_ZN1VIiE1mE


struct S { S(); ~S(); };
struct T { ~T(); };

// CHECK-LABEL: define void @_Z8tls_dtorv()
void tls_dtor() {
  // CHECK: load i8* @_ZGVZ8tls_dtorvE1s
  // CHECK: call void @_ZN1SC1Ev(%struct.S* @_ZZ8tls_dtorvE1s)
  // CHECK: call i32 @__cxa_thread_atexit({{.*}}@_ZN1SD1Ev {{.*}} @_ZZ8tls_dtorvE1s{{.*}} @__dso_handle
  // CHECK: store i8 1, i8* @_ZGVZ8tls_dtorvE1s
  static thread_local S s;

  // CHECK: load i8* @_ZGVZ8tls_dtorvE1t
  // CHECK-NOT: _ZN1T
  // CHECK: call i32 @__cxa_thread_atexit({{.*}}@_ZN1TD1Ev {{.*}}@_ZZ8tls_dtorvE1t{{.*}} @__dso_handle
  // CHECK: store i8 1, i8* @_ZGVZ8tls_dtorvE1t
  static thread_local T t;

  // CHECK: load i8* @_ZGVZ8tls_dtorvE1u
  // CHECK: call void @_ZN1SC1Ev(%struct.S* @_ZGRZ8tls_dtorvE1u_)
  // CHECK: call i32 @__cxa_thread_atexit({{.*}}@_ZN1SD1Ev {{.*}} @_ZGRZ8tls_dtorvE1u_{{.*}} @__dso_handle
  // CHECK: store i8 1, i8* @_ZGVZ8tls_dtorvE1u
  static thread_local const S &u = S();
}

// CHECK: declare i32 @__cxa_thread_atexit(void (i8*)*, i8*, i8*)

// CHECK: define {{.*}} @_Z7PR15991v(
int PR15991() {
  thread_local int n;
  auto l = [] { return n; };
  return l();
}

struct PR19254 {
  static thread_local int n;
  int f();
};
// CHECK: define {{.*}} @_ZN7PR192541fEv(
int PR19254::f() {
  // CHECK: call void @_ZTHN7PR192541nE(
  return this->n;
}

namespace {
thread_local int anon_i{1};
}
void set_anon_i() {
  anon_i = 2;
}
// CHECK-LABEL: define internal i32* @_ZTWN12_GLOBAL__N_16anon_iE()

// CHECK: define {{.*}} @[[V_M_INIT:.*]]()
// CHECK: load i8* bitcast (i64* @_ZGVN1VIiE1mE to i8*)
// CHECK: %[[V_M_INITIALIZED:.*]] = icmp eq i8 %{{.*}}, 0
// CHECK: br i1 %[[V_M_INITIALIZED]],
// need init:
// CHECK: call i32 @_Z1gv()
// CHECK: store i32 %{{.*}}, i32* @_ZN1VIiE1mE, align 4
// CHECK: store i64 1, i64* @_ZGVN1VIiE1mE
// CHECK: br label

// CHECK: define {{.*}}@[[GLOBAL_INIT:.*]]()
// CHECK: call void @[[C_INIT]]()
// CHECK: call void @[[E_INIT]]()


// CHECK: define {{.*}}@__tls_init()
// CHECK: load i8* @__tls_guard
// CHECK: %[[NEED_TLS_INIT:.*]] = icmp eq i8 %{{.*}}, 0
// CHECK: store i8 1, i8* @__tls_guard
// CHECK: br i1 %[[NEED_TLS_INIT]],
// init:
// CHECK: call void @[[A_INIT]]()
// CHECK: call void @[[D_INIT]]()
// CHECK: call void @[[U_M_INIT]]()
// CHECK: call void @[[V_M_INIT]]()


// CHECK: define weak_odr hidden i32* @_ZTW1a() {
// CHECK:   call void @_ZTH1a()
// CHECK:   ret i32* @a
// CHECK: }


// CHECK: declare extern_weak void @_ZTH1b()


// CHECK-LABEL: define internal i32* @_ZTWL1d()
// CHECK: call void @_ZTHL1d()
// CHECK: ret i32* @_ZL1d

// CHECK-LABEL: define weak_odr hidden i32* @_ZTWN1U1mE()
// CHECK: call void @_ZTHN1U1mE()
// CHECK: ret i32* @_ZN1U1mE
