// Test instrumentation of general constructs in C.

// RUN: %clang_cc1 -triple x86_64-apple-macosx10.9 -main-file-name c-general.c %s -o - -emit-llvm -fprofile-instr-generate | FileCheck -check-prefix=PGOGEN %s

// RUN: llvm-profdata merge %S/Inputs/c-general.proftext -o %t.profdata
// RUN: %clang_cc1 -triple x86_64-apple-macosx10.9 -main-file-name c-general.c %s -o - -emit-llvm -fprofile-instr-use=%t.profdata | FileCheck -check-prefix=PGOUSE %s

// PGOGEN: @[[SLC:__llvm_profile_counters_simple_loops]] = hidden global [4 x i64] zeroinitializer
// PGOGEN: @[[IFC:__llvm_profile_counters_conditionals]] = hidden global [11 x i64] zeroinitializer
// PGOGEN: @[[EEC:__llvm_profile_counters_early_exits]] = hidden global [9 x i64] zeroinitializer
// PGOGEN: @[[JMC:__llvm_profile_counters_jumps]] = hidden global [22 x i64] zeroinitializer
// PGOGEN: @[[SWC:__llvm_profile_counters_switches]] = hidden global [19 x i64] zeroinitializer
// PGOGEN: @[[BSC:__llvm_profile_counters_big_switch]] = hidden global [17 x i64] zeroinitializer
// PGOGEN: @[[BOC:__llvm_profile_counters_boolean_operators]] = hidden global [8 x i64] zeroinitializer
// PGOGEN: @[[BLC:__llvm_profile_counters_boolop_loops]] = hidden global [9 x i64] zeroinitializer
// PGOGEN: @[[COC:__llvm_profile_counters_conditional_operator]] = hidden global [3 x i64] zeroinitializer
// PGOGEN: @[[MAC:__llvm_profile_counters_main]] = hidden global [1 x i64] zeroinitializer
// PGOGEN: @[[STC:__llvm_profile_counters_static_func]] = internal global [2 x i64] zeroinitializer

// PGOGEN-LABEL: @simple_loops()
// PGOUSE-LABEL: @simple_loops()
// PGOGEN: store {{.*}} @[[SLC]], i64 0, i64 0
void simple_loops() {
  int i;
  // PGOGEN: store {{.*}} @[[SLC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[SL1:[0-9]+]]
  for (i = 0; i < 100; ++i) {
  }
  // PGOGEN: store {{.*}} @[[SLC]], i64 0, i64 2
  // PGOUSE: br {{.*}} !prof ![[SL2:[0-9]+]]
  while (i > 0)
    i--;
  // PGOGEN: store {{.*}} @[[SLC]], i64 0, i64 3
  // PGOUSE: br {{.*}} !prof ![[SL3:[0-9]+]]
  do {} while (i++ < 75);

  // PGOGEN-NOT: store {{.*}} @[[SLC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
}

// PGOGEN-LABEL: @conditionals()
// PGOUSE-LABEL: @conditionals()
// PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 0
void conditionals() {
  // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[IF1:[0-9]+]]
  for (int i = 0; i < 100; ++i) {
    // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 2
    // PGOUSE: br {{.*}} !prof ![[IF2:[0-9]+]]
    if (i % 2) {
      // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 3
      // PGOUSE: br {{.*}} !prof ![[IF3:[0-9]+]]
      if (i) {}
    // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 4
    // PGOUSE: br {{.*}} !prof ![[IF4:[0-9]+]]
    } else if (i % 3) {
      // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 5
      // PGOUSE: br {{.*}} !prof ![[IF5:[0-9]+]]
      if (i) {}
    } else {
      // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 6
      // PGOUSE: br {{.*}} !prof ![[IF6:[0-9]+]]
      if (i) {}
    }

    // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 8
    // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 7
    // PGOUSE: br {{.*}} !prof ![[IF7:[0-9]+]]
    if (1 && i) {}
    // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 10
    // PGOGEN: store {{.*}} @[[IFC]], i64 0, i64 9
    // PGOUSE: br {{.*}} !prof ![[IF8:[0-9]+]]
    if (0 || i) {}
  }

  // PGOGEN-NOT: store {{.*}} @[[IFC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
}

// PGOGEN-LABEL: @early_exits()
// PGOUSE-LABEL: @early_exits()
// PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 0
void early_exits() {
  int i = 0;

  // PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[EE1:[0-9]+]]
  if (i) {}

  // PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 2
  // PGOUSE: br {{.*}} !prof ![[EE2:[0-9]+]]
  while (i < 100) {
    i++;
    // PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 3
    // PGOUSE: br {{.*}} !prof ![[EE3:[0-9]+]]
    if (i > 50)
      break;
    // PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 4
    // PGOUSE: br {{.*}} !prof ![[EE4:[0-9]+]]
    if (i % 2)
      continue;
  }

  // PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 5
  // PGOUSE: br {{.*}} !prof ![[EE5:[0-9]+]]
  if (i) {}

  // PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 6
  do {
    // PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 7
    // PGOUSE: br {{.*}} !prof ![[EE6:[0-9]+]]
    if (i > 75)
      return;
    else
      i++;
  // PGOUSE: br {{.*}} !prof ![[EE7:[0-9]+]]
  } while (i < 100);

  // PGOGEN: store {{.*}} @[[EEC]], i64 0, i64 8
  // Never reached -> no weights
  if (i) {}

  // PGOGEN-NOT: store {{.*}} @[[EEC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
}

// PGOGEN-LABEL: @jumps()
// PGOUSE-LABEL: @jumps()
// PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 0
void jumps() {
  int i;

  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[JM1:[0-9]+]]
  for (i = 0; i < 2; ++i) {
    goto outofloop;
    // Never reached -> no weights
    if (i) {}
  }
// PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 3
outofloop:
  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 4
  // PGOUSE: br {{.*}} !prof ![[JM2:[0-9]+]]
  if (i) {}

  goto loop1;

  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 5
  // PGOUSE: br {{.*}} !prof ![[JM3:[0-9]+]]
  while (i) {
  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 6
  loop1:
    // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 7
    // PGOUSE: br {{.*}} !prof ![[JM4:[0-9]+]]
    if (i) {}
  }

  goto loop2;
// PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 8
first:
// PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 9
second:
// PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 10
third:
  i++;
  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 11
  // PGOUSE: br {{.*}} !prof ![[JM5:[0-9]+]]
  if (i < 3)
    goto loop2;

  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 12
  // PGOUSE: br {{.*}} !prof ![[JM6:[0-9]+]]
  while (i < 3) {
  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 13
  loop2:
    // PGOUSE: switch {{.*}} [
    // PGOUSE: ], !prof ![[JM7:[0-9]+]]
    switch (i) {
    // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 15
    case 0:
      goto first;
    // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 16
    case 1:
      goto second;
    // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 17
    case 2:
      goto third;
    }
    // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 14
  }

  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 18
  // PGOUSE: br {{.*}} !prof ![[JM8:[0-9]+]]
  for (i = 0; i < 10; ++i) {
    goto withinloop;
    // never reached -> no weights
    if (i) {}
  // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 20
  withinloop:
    // PGOGEN: store {{.*}} @[[JMC]], i64 0, i64 21
    // PGOUSE: br {{.*}} !prof ![[JM9:[0-9]+]]
    if (i) {}
  }

  // PGOGEN-NOT: store {{.*}} @[[JMC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
}

// PGOGEN-LABEL: @switches()
// PGOUSE-LABEL: @switches()
// PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 0
void switches() {
  static int weights[] = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5};

  // No cases -> no weights
  switch (weights[0]) {
  // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 2
  default:
    break;
  }
  // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 1

  // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 3
  // PGOUSE: br {{.*}} !prof ![[SW1:[0-9]+]]
  for (int i = 0, len = sizeof(weights) / sizeof(weights[0]); i < len; ++i) {
    // PGOUSE: switch {{.*}} [
    // PGOUSE: ], !prof ![[SW2:[0-9]+]]
    switch (i[weights]) {
    // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 5
    case 1:
      // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 6
      // PGOUSE: br {{.*}} !prof ![[SW3:[0-9]+]]
      if (i) {}
      // fallthrough
    // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 7
    case 2:
      // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 8
      // PGOUSE: br {{.*}} !prof ![[SW4:[0-9]+]]
      if (i) {}
      break;
    // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 9
    case 3:
      // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 10
      // PGOUSE: br {{.*}} !prof ![[SW5:[0-9]+]]
      if (i) {}
      continue;
    // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 11
    case 4:
      // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 12
      // PGOUSE: br {{.*}} !prof ![[SW6:[0-9]+]]
      if (i) {}
      // PGOUSE: switch {{.*}} [
      // PGOUSE: ], !prof ![[SW7:[0-9]+]]
      switch (i) {
      // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 14
      case 6 ... 9:
        // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 15
        // PGOUSE: br {{.*}} !prof ![[SW8:[0-9]+]]
        if (i) {}
        continue;
      }
      // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 13

    // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 16
    default:
      // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 17
      // PGOUSE: br {{.*}} !prof ![[SW9:[0-9]+]]
      if (i == len - 1)
        return;
    }
    // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 4
  }

  // PGOGEN: store {{.*}} @[[SWC]], i64 0, i64 18
  // Never reached -> no weights
  if (weights[0]) {}

  // PGOGEN-NOT: store {{.*}} @[[SWC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
}

// PGOGEN-LABEL: @big_switch()
// PGOUSE-LABEL: @big_switch()
// PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 0
void big_switch() {
  // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[BS1:[0-9]+]]
  for (int i = 0; i < 32; ++i) {
    // PGOUSE: switch {{.*}} [
    // PGOUSE: ], !prof ![[BS2:[0-9]+]]
    switch (1 << i) {
    // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 3
    case (1 << 0):
      // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 4
      // PGOUSE: br {{.*}} !prof ![[BS3:[0-9]+]]
      if (i) {}
      // fallthrough
    // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 5
    case (1 << 1):
      // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 6
      // PGOUSE: br {{.*}} !prof ![[BS4:[0-9]+]]
      if (i) {}
      break;
    // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 7
    case (1 << 2) ... (1 << 12):
      // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 8
      // PGOUSE: br {{.*}} !prof ![[BS5:[0-9]+]]
      if (i) {}
      break;
    // The branch for the large case range above appears after the case body
    // PGOUSE: br {{.*}} !prof ![[BS6:[0-9]+]]

    // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 9
    case (1 << 13):
      // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 10
      // PGOUSE: br {{.*}} !prof ![[BS7:[0-9]+]]
      if (i) {}
      break;
    // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 11
    case (1 << 14) ... (1 << 28):
      // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 12
      // PGOUSE: br {{.*}} !prof ![[BS8:[0-9]+]]
      if (i) {}
      break;
    // The branch for the large case range above appears after the case body
    // PGOUSE: br {{.*}} !prof ![[BS9:[0-9]+]]

    // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 13
    case (1 << 29) ... ((1 << 29) + 1):
      // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 14
      // PGOUSE: br {{.*}} !prof ![[BS10:[0-9]+]]
      if (i) {}
      break;
    // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 15
    default:
      // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 16
      // PGOUSE: br {{.*}} !prof ![[BS11:[0-9]+]]
      if (i) {}
      break;
    }
    // PGOGEN: store {{.*}} @[[BSC]], i64 0, i64 2
  }

  // PGOGEN-NOT: store {{.*}} @[[BSC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
  // PGOUSE: ret void
}

// PGOGEN-LABEL: @boolean_operators()
// PGOUSE-LABEL: @boolean_operators()
// PGOGEN: store {{.*}} @[[BOC]], i64 0, i64 0
void boolean_operators() {
  int v;
  // PGOGEN: store {{.*}} @[[BOC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[BO1:[0-9]+]]
  for (int i = 0; i < 100; ++i) {
    // PGOGEN: store {{.*}} @[[BOC]], i64 0, i64 2
    // PGOUSE: br {{.*}} !prof ![[BO2:[0-9]+]]
    v = i % 3 || i;

    // PGOGEN: store {{.*}} @[[BOC]], i64 0, i64 3
    // PGOUSE: br {{.*}} !prof ![[BO3:[0-9]+]]
    v = i % 3 && i;

    // PGOGEN: store {{.*}} @[[BOC]], i64 0, i64 5
    // PGOGEN: store {{.*}} @[[BOC]], i64 0, i64 4
    // PGOUSE: br {{.*}} !prof ![[BO4:[0-9]+]]
    // PGOUSE: br {{.*}} !prof ![[BO5:[0-9]+]]
    v = i % 3 || i % 2 || i;

    // PGOGEN: store {{.*}} @[[BOC]], i64 0, i64 7
    // PGOGEN: store {{.*}} @[[BOC]], i64 0, i64 6
    // PGOUSE: br {{.*}} !prof ![[BO6:[0-9]+]]
    // PGOUSE: br {{.*}} !prof ![[BO7:[0-9]+]]
    v = i % 2 && i % 3 && i;
  }

  // PGOGEN-NOT: store {{.*}} @[[BOC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
}

// PGOGEN-LABEL: @boolop_loops()
// PGOUSE-LABEL: @boolop_loops()
// PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 0
void boolop_loops() {
  int i = 100;

  // PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 2
  // PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[BL1:[0-9]+]]
  // PGOUSE: br {{.*}} !prof ![[BL2:[0-9]+]]
  while (i && i > 50)
    i--;

  // PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 4
  // PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 3
  // PGOUSE: br {{.*}} !prof ![[BL3:[0-9]+]]
  // PGOUSE: br {{.*}} !prof ![[BL4:[0-9]+]]
  while ((i % 2) || (i > 0))
    i--;

  // PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 6
  // PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 5
  // PGOUSE: br {{.*}} !prof ![[BL5:[0-9]+]]
  // PGOUSE: br {{.*}} !prof ![[BL6:[0-9]+]]
  for (i = 100; i && i > 50; --i);

  // PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 8
  // PGOGEN: store {{.*}} @[[BLC]], i64 0, i64 7
  // PGOUSE: br {{.*}} !prof ![[BL7:[0-9]+]]
  // PGOUSE: br {{.*}} !prof ![[BL8:[0-9]+]]
  for (; (i % 2) || (i > 0); --i);

  // PGOGEN-NOT: store {{.*}} @[[BLC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
}

// PGOGEN-LABEL: @conditional_operator()
// PGOUSE-LABEL: @conditional_operator()
// PGOGEN: store {{.*}} @[[COC]], i64 0, i64 0
void conditional_operator() {
  int i = 100;

  // PGOGEN: store {{.*}} @[[COC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[CO1:[0-9]+]]
  int j = i < 50 ? i : 1;

  // PGOGEN: store {{.*}} @[[COC]], i64 0, i64 2
  // PGOUSE: br {{.*}} !prof ![[CO2:[0-9]+]]
  int k = i ?: 0;

  // PGOGEN-NOT: store {{.*}} @[[COC]],
  // PGOUSE-NOT: br {{.*}} !prof ![0-9]+
}

void do_fallthrough() {
  for (int i = 0; i < 10; ++i) {
    int j = 0;
    do {
      // The number of exits out of this do-loop via the break statement
      // exceeds the counter value for the loop (which does not include the
      // fallthrough count). Make sure that does not violate any assertions.
      if (i < 8) break;
      j++;
    } while (j < 2);
  }
}

// PGOGEN-LABEL: @static_func()
// PGOUSE-LABEL: @static_func()
// PGOGEN: store {{.*}} @[[STC]], i64 0, i64 0
static void static_func() {
  // PGOGEN: store {{.*}} @[[STC]], i64 0, i64 1
  // PGOUSE: br {{.*}} !prof ![[ST1:[0-9]+]]
  for (int i = 0; i < 10; ++i) {
  }
}

// PGOUSE-DAG: ![[SL1]] = metadata !{metadata !"branch_weights", i32 101, i32 2}
// PGOUSE-DAG: ![[SL2]] = metadata !{metadata !"branch_weights", i32 101, i32 2}
// PGOUSE-DAG: ![[SL3]] = metadata !{metadata !"branch_weights", i32 76, i32 2}

// PGOUSE-DAG: ![[EE1]] = metadata !{metadata !"branch_weights", i32 1, i32 2}
// PGOUSE-DAG: ![[EE2]] = metadata !{metadata !"branch_weights", i32 52, i32 1}
// PGOUSE-DAG: ![[EE3]] = metadata !{metadata !"branch_weights", i32 2, i32 51}
// PGOUSE-DAG: ![[EE4]] = metadata !{metadata !"branch_weights", i32 26, i32 26}
// PGOUSE-DAG: ![[EE5]] = metadata !{metadata !"branch_weights", i32 2, i32 1}
// PGOUSE-DAG: ![[EE6]] = metadata !{metadata !"branch_weights", i32 2, i32 26}
// PGOUSE-DAG: ![[EE7]] = metadata !{metadata !"branch_weights", i32 26, i32 1}

// PGOUSE-DAG: ![[IF1]] = metadata !{metadata !"branch_weights", i32 101, i32 2}
// PGOUSE-DAG: ![[IF2]] = metadata !{metadata !"branch_weights", i32 51, i32 51}
// PGOUSE-DAG: ![[IF3]] = metadata !{metadata !"branch_weights", i32 51, i32 1}
// PGOUSE-DAG: ![[IF4]] = metadata !{metadata !"branch_weights", i32 34, i32 18}
// PGOUSE-DAG: ![[IF5]] = metadata !{metadata !"branch_weights", i32 34, i32 1}
// PGOUSE-DAG: ![[IF6]] = metadata !{metadata !"branch_weights", i32 17, i32 2}
// PGOUSE-DAG: ![[IF7]] = metadata !{metadata !"branch_weights", i32 100, i32 2}
// PGOUSE-DAG: ![[IF8]] = metadata !{metadata !"branch_weights", i32 100, i32 2}

// PGOUSE-DAG: ![[JM1]] = metadata !{metadata !"branch_weights", i32 2, i32 1}
// PGOUSE-DAG: ![[JM2]] = metadata !{metadata !"branch_weights", i32 1, i32 2}
// PGOUSE-DAG: ![[JM3]] = metadata !{metadata !"branch_weights", i32 1, i32 2}
// PGOUSE-DAG: ![[JM4]] = metadata !{metadata !"branch_weights", i32 1, i32 2}
// PGOUSE-DAG: ![[JM5]] = metadata !{metadata !"branch_weights", i32 3, i32 2}
// PGOUSE-DAG: ![[JM6]] = metadata !{metadata !"branch_weights", i32 1, i32 2}
// PGOUSE-DAG: ![[JM7]] = metadata !{metadata !"branch_weights", i32 1, i32 2, i32 2, i32 2}
// PGOUSE-DAG: ![[JM8]] = metadata !{metadata !"branch_weights", i32 11, i32 2}
// PGOUSE-DAG: ![[JM9]] = metadata !{metadata !"branch_weights", i32 10, i32 2}

// PGOUSE-DAG: ![[SW1]] = metadata !{metadata !"branch_weights", i32 16, i32 1}
// PGOUSE-DAG: ![[SW2]] = metadata !{metadata !"branch_weights", i32 6, i32 2, i32 3, i32 4, i32 5}
// PGOUSE-DAG: ![[SW3]] = metadata !{metadata !"branch_weights", i32 1, i32 2}
// PGOUSE-DAG: ![[SW4]] = metadata !{metadata !"branch_weights", i32 3, i32 2}
// PGOUSE-DAG: ![[SW5]] = metadata !{metadata !"branch_weights", i32 4, i32 1}
// PGOUSE-DAG: ![[SW6]] = metadata !{metadata !"branch_weights", i32 5, i32 1}
// PGOUSE-DAG: ![[SW7]] = metadata !{metadata !"branch_weights", i32 1, i32 2, i32 2, i32 2, i32 2}
// PGOUSE-DAG: ![[SW8]] = metadata !{metadata !"branch_weights", i32 5, i32 1}
// PGOUSE-DAG: ![[SW9]] = metadata !{metadata !"branch_weights", i32 2, i32 5}

// PGOUSE-DAG: ![[BS1]] = metadata !{metadata !"branch_weights", i32 33, i32 2}
// PGOUSE-DAG: ![[BS2]] = metadata !{metadata !"branch_weights", i32 29, i32 2, i32 2, i32 2, i32 2, i32 1}
// PGOUSE-DAG: ![[BS3]] = metadata !{metadata !"branch_weights", i32 1, i32 2}
// PGOUSE-DAG: ![[BS4]] = metadata !{metadata !"branch_weights", i32 2, i32 2}
// PGOUSE-DAG: ![[BS5]] = metadata !{metadata !"branch_weights", i32 12, i32 1}
// PGOUSE-DAG: ![[BS6]] = metadata !{metadata !"branch_weights", i32 12, i32 3}
// PGOUSE-DAG: ![[BS7]] = metadata !{metadata !"branch_weights", i32 2, i32 1}
// PGOUSE-DAG: ![[BS8]] = metadata !{metadata !"branch_weights", i32 16, i32 1}
// PGOUSE-DAG: ![[BS9]] = metadata !{metadata !"branch_weights", i32 16, i32 14}
// PGOUSE-DAG: ![[BS10]] = metadata !{metadata !"branch_weights", i32 2, i32 1}
// PGOUSE-DAG: ![[BS11]] = metadata !{metadata !"branch_weights", i32 3, i32 1}

// PGOUSE-DAG: ![[BO1]] = metadata !{metadata !"branch_weights", i32 101, i32 2}
// PGOUSE-DAG: ![[BO2]] = metadata !{metadata !"branch_weights", i32 67, i32 35}
// PGOUSE-DAG: ![[BO3]] = metadata !{metadata !"branch_weights", i32 67, i32 35}
// PGOUSE-DAG: ![[BO4]] = metadata !{metadata !"branch_weights", i32 67, i32 35}
// PGOUSE-DAG: ![[BO5]] = metadata !{metadata !"branch_weights", i32 18, i32 18}
// PGOUSE-DAG: ![[BO6]] = metadata !{metadata !"branch_weights", i32 51, i32 51}
// PGOUSE-DAG: ![[BO7]] = metadata !{metadata !"branch_weights", i32 34, i32 18}
// PGOUSE-DAG: ![[BL1]] = metadata !{metadata !"branch_weights", i32 52, i32 1}
// PGOUSE-DAG: ![[BL2]] = metadata !{metadata !"branch_weights", i32 51, i32 2}
// PGOUSE-DAG: ![[BL3]] = metadata !{metadata !"branch_weights", i32 26, i32 27}
// PGOUSE-DAG: ![[BL4]] = metadata !{metadata !"branch_weights", i32 51, i32 2}
// PGOUSE-DAG: ![[BL5]] = metadata !{metadata !"branch_weights", i32 52, i32 1}
// PGOUSE-DAG: ![[BL6]] = metadata !{metadata !"branch_weights", i32 51, i32 2}
// PGOUSE-DAG: ![[BL7]] = metadata !{metadata !"branch_weights", i32 26, i32 27}
// PGOUSE-DAG: ![[BL8]] = metadata !{metadata !"branch_weights", i32 51, i32 2}
// PGOUSE-DAG: ![[CO1]] = metadata !{metadata !"branch_weights", i32 1, i32 2}
// PGOUSE-DAG: ![[CO2]] = metadata !{metadata !"branch_weights", i32 2, i32 1}
// PGOUSE-DAG: ![[ST1]] = metadata !{metadata !"branch_weights", i32 11, i32 2}

int main(int argc, const char *argv[]) {
  simple_loops();
  conditionals();
  early_exits();
  jumps();
  switches();
  big_switch();
  boolean_operators();
  boolop_loops();
  conditional_operator();
  do_fallthrough();
  static_func();
  return 0;
}
