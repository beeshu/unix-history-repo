// RUN: %clang_cc1 -mconstructor-aliases -std=c++11 -fexceptions -emit-llvm %s -o - -triple=i386-pc-win32 | FileCheck %s -check-prefix=X86
// RUN: %clang_cc1 -mconstructor-aliases -std=c++11 -fexceptions -emit-llvm %s -o - -triple=x86_64-pc-win32 | FileCheck %s -check-prefix=X64

struct A {
  A(int a);
  A(const A &o);
  ~A();
  int a;
};

void foo(A a, A b, A c) {
}

// Order of destruction should be left to right.
//
// X86-LABEL: define void @"\01?foo@@YAXUA@@00@Z"
// X86:          ([[argmem_ty:<{ %struct.A, %struct.A, %struct.A }>]]* inalloca)
// X86: %[[a:[^ ]*]] = getelementptr inbounds [[argmem_ty]]* %0, i32 0, i32 0
// X86: %[[b:[^ ]*]] = getelementptr inbounds [[argmem_ty]]* %0, i32 0, i32 1
// X86: %[[c:[^ ]*]] = getelementptr inbounds [[argmem_ty]]* %0, i32 0, i32 2
// X86: call x86_thiscallcc void @"\01??1A@@QAE@XZ"(%struct.A* %[[a]])
// X86: call x86_thiscallcc void @"\01??1A@@QAE@XZ"(%struct.A* %[[b]])
// X86: call x86_thiscallcc void @"\01??1A@@QAE@XZ"(%struct.A* %[[c]])
// X86: ret void

// X64-LABEL: define void @"\01?foo@@YAXUA@@00@Z"
// X64:         (%struct.A* %[[a:[^,]*]], %struct.A* %[[b:[^,]*]], %struct.A* %[[c:[^)]*]])
// X64: call void @"\01??1A@@QEAA@XZ"(%struct.A* %[[a]])
// X64: call void @"\01??1A@@QEAA@XZ"(%struct.A* %[[b]])
// X64: call void @"\01??1A@@QEAA@XZ"(%struct.A* %[[c]])
// X64: ret void


void call_foo() {
  foo(A(1), A(2), A(3));
}

// Order of evaluation should be right to left, and we should clean up the right
// things as we unwind.
//
// X86-LABEL: define void @"\01?call_foo@@YAXXZ"()
// X86: call i8* @llvm.stacksave()
// X86: %[[argmem:[^ ]*]] = alloca inalloca [[argmem_ty]]
// X86: %[[arg3:[^ ]*]] = getelementptr inbounds [[argmem_ty]]* %[[argmem]], i32 0, i32 2
// X86: invoke x86_thiscallcc %struct.A* @"\01??0A@@QAE@H@Z"(%struct.A* %[[arg3]], i32 3)
// X86: %[[arg2:[^ ]*]] = getelementptr inbounds [[argmem_ty]]* %[[argmem]], i32 0, i32 1
// X86: invoke x86_thiscallcc %struct.A* @"\01??0A@@QAE@H@Z"(%struct.A* %[[arg2]], i32 2)
// X86: %[[arg1:[^ ]*]] = getelementptr inbounds [[argmem_ty]]* %[[argmem]], i32 0, i32 0
// X86: invoke x86_thiscallcc %struct.A* @"\01??0A@@QAE@H@Z"(%struct.A* %[[arg1]], i32 1)
// X86: invoke void @"\01?foo@@YAXUA@@00@Z"([[argmem_ty]]* inalloca %[[argmem]])
// X86: call void @llvm.stackrestore
// X86: ret void
//
//   lpad2:
// X86: call x86_thiscallcc void @"\01??1A@@QAE@XZ"(%struct.A* %[[arg2]])
// X86: br label
//
//   ehcleanup:
// X86: call x86_thiscallcc void @"\01??1A@@QAE@XZ"(%struct.A* %[[arg3]])

// X64-LABEL: define void @"\01?call_foo@@YAXXZ"()
// X64: call %struct.A* @"\01??0A@@QEAA@H@Z"(%struct.A* %[[arg3:[^,]*]], i32 3)
// X64: invoke %struct.A* @"\01??0A@@QEAA@H@Z"(%struct.A* %[[arg2:[^,]*]], i32 2)
// X64: invoke %struct.A* @"\01??0A@@QEAA@H@Z"(%struct.A* %[[arg1:[^,]*]], i32 1)
// X64: call void @"\01?foo@@YAXUA@@00@Z"
// X64:       (%struct.A* %[[arg1]], %struct.A* %[[arg2]], %struct.A* %[[arg3]])
// X64: ret void
//
//   lpad2:
// X64: call void @"\01??1A@@QEAA@XZ"(%struct.A* %[[arg2]])
// X64: br label
//
//   ehcleanup:
// X64: call void @"\01??1A@@QEAA@XZ"(%struct.A* %[[arg3]])
