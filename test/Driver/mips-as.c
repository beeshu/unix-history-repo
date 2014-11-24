// Check passing options to the assembler for MIPS targets.
//
// RUN: %clang -target mips-linux-gnu -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS32R2-EB-AS %s
// RUN: %clang -target mipsel-linux-gnu -### \
// RUN:   -no-integrated-as -c -EB %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS32R2-EB-AS %s
// MIPS32R2-EB-AS: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
// MIPS32R2-EB-AS-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-KPIC"
//
// RUN: %clang -target mips-linux-gnu -### \
// RUN:   -no-integrated-as -fPIC -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS32R2-EB-PIC %s
// MIPS32R2-EB-PIC: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-call_nonpic" "-EB"
// MIPS32R2-EB-PIC: "-KPIC"
//
// RUN: %clang -target mipsel-linux-gnu -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS32R2-DEF-EL-AS %s
// MIPS32R2-DEF-EL-AS: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EL"
//
// RUN: %clang -target mips64-linux-gnu -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS64R2-EB-AS %s
// MIPS64R2-EB-AS: as{{(.exe)?}}" "-march" "mips64r2" "-mabi" "64" "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips64el-linux-gnu -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS64R2-DEF-EL-AS %s
// MIPS64R2-DEF-EL-AS: as{{(.exe)?}}" "-march" "mips64r2" "-mabi" "64"  "-mno-shared" "-KPIC" "-EL"
//
// RUN: %clang -target mips-linux-gnu -mabi=eabi -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-EABI %s
// MIPS-EABI: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "eabi" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips64-linux-gnu -mabi=n32 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-N32 %s
// MIPS-N32: as{{(.exe)?}}" "-march" "mips64r2" "-mabi" "n32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mipsel-linux-gnu -mabi=32 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS32R2-EL-AS %s
// RUN: %clang -target mips-linux-gnu -mabi=32 -### \
// RUN:   -no-integrated-as -c %s -EL 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS32R2-EL-AS %s
// MIPS32R2-EL-AS: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EL"
//
// RUN: %clang -target mips64el-linux-gnu -mabi=64 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS64R2-EL-AS %s
// MIPS64R2-EL-AS: as{{(.exe)?}}" "-march" "mips64r2" "-mabi" "64"  "-mno-shared" "-KPIC" "-EL"
//
// RUN: %clang -target mips-linux-gnu -march=mips32r2 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-32R2 %s
// MIPS-32R2: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips64-linux-gnu -march=octeon -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-OCTEON %s
// MIPS-OCTEON: as{{(.exe)?}}" "-march" "octeon" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mips1 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-1 %s
// MIPS-ALIAS-1: as{{(.exe)?}}" "-march" "mips1" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mips2 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-2 %s
// MIPS-ALIAS-2: as{{(.exe)?}}" "-march" "mips2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mips3 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-3 %s
// MIPS-ALIAS-3: as{{(.exe)?}}" "-march" "mips3" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mips4 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-4 %s
// MIPS-ALIAS-4: as{{(.exe)?}}" "-march" "mips4" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mips5 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-5 %s
// MIPS-ALIAS-5: as{{(.exe)?}}" "-march" "mips5" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mips32 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-32 %s
// MIPS-ALIAS-32: as{{(.exe)?}}" "-march" "mips32" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mips32r2 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-32R2 %s
// MIPS-ALIAS-32R2: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mips32r6 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-32R6 %s
// MIPS-ALIAS-32R6: as{{(.exe)?}}" "-march" "mips32r6" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
//
// RUN: %clang -target mips64-linux-gnu -mips64 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-64 %s
// MIPS-ALIAS-64: as{{(.exe)?}}" "-march" "mips64" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips64-linux-gnu -mips64r2 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-64R2 %s
// MIPS-ALIAS-64R2: as{{(.exe)?}}" "-march" "mips64r2" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips64-linux-gnu -mips64r6 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-ALIAS-64R6 %s
// MIPS-ALIAS-64R6: as{{(.exe)?}}" "-march" "mips64r6" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips-linux-gnu -mno-mips16 -mips16 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-16 %s
// MIPS-16: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mips16"
//
// RUN: %clang -target mips-linux-gnu -mips16 -mno-mips16 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-N16 %s
// MIPS-N16: as{{(.exe)?}}"
// MIPS-N16: -no-mips16
//
// RUN: %clang -target mips-linux-gnu -mno-micromips -mmicromips -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-MICRO %s
// MIPS-MICRO: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mmicromips"
//
// RUN: %clang -target mips-linux-gnu -mmicromips -mno-micromips -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-NMICRO %s
// MIPS-NMICRO: as{{(.exe)?}}"
// MIPS-NMICRO-NOT: {{[A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-mmicromips"
//
// RUN: %clang -target mips-linux-gnu -mno-dsp -mdsp -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-DSP %s
// MIPS-DSP: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mdsp"
//
// RUN: %clang -target mips-linux-gnu -mdsp -mno-dsp -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-NDSP %s
// MIPS-NDSP: as{{(.exe)?}}"
// MIPS-NDSP-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-mdsp"
//
// RUN: %clang -target mips-linux-gnu -mno-dspr2 -mdspr2 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-DSPR2 %s
// MIPS-DSPR2: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mdspr2"
//
// RUN: %clang -target mips-linux-gnu -mdspr2 -mno-dspr2 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-NDSPR2 %s
// MIPS-NDSPR2: as{{(.exe)?}}"
// MIPS-NDSPR2-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-mdspr2"
//
// RUN: %clang -target mips-linux-gnu -mnan=legacy -mnan=2008 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-NAN2008 %s
// MIPS-NAN2008: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mnan=2008"
//
// RUN: %clang -target mips-linux-gnu -mnan=2008 -mnan=legacy -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-NAN-LEGACY %s
// MIPS-NAN-LEGACY: as{{(.exe)?}}"
// MIPS-NAN-LEGACY-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-mnan={{.*}}"
//
// RUN: %clang -target mips-linux-gnu -mfp64 -mfpxx -mfp32 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-MFP32 %s
// MIPS-MFP32: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mfp32"
//
// RUN: %clang -target mips-linux-gnu -mfp32 -mfp64 -mfpxx -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-MFPXX %s
// MIPS-MFPXX: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mfpxx"
//
// RUN: %clang -target mips-linux-gnu -mfpxx -mfp32 -mfp64 -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-MFP64 %s
// MIPS-MFP64: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mfp64"
//
// RUN: %clang -target mips-linux-gnu -mno-msa -mmsa -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-MSA %s
// MIPS-MSA: as{{(.exe)?}}" "-march" "mips32r2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB" "-mmsa"
//
// RUN: %clang -target mips-linux-gnu -mmsa -mno-msa -### \
// RUN:   -no-integrated-as -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=MIPS-NMSA %s
// MIPS-NMSA: as{{(.exe)?}}"
// MIPS-NMSA-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-mmsa"
//
// We've already tested MIPS32r2 and MIPS64r2 thoroughly. Do minimal tests on
// the remaining CPU's since it was possible to pass on a -mabi with no value
// when the CPU name is absent from a StringSwitch in getMipsCPUAndABI()
// RUN: %clang -target mips-linux-gnu -### -no-integrated-as -c %s -mcpu=mips1 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS1-EB-AS %s
// MIPS1-EB-AS: as{{(.exe)?}}" "-march" "mips1" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
// MIPS1-EB-AS-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-KPIC"
//
// RUN: %clang -target mips-linux-gnu -### -no-integrated-as -c %s -mcpu=mips2 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS2-EB-AS %s
// MIPS2-EB-AS: as{{(.exe)?}}" "-march" "mips2" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
// MIPS2-EB-AS-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-KPIC"
//
// RUN: %clang -target mips64-linux-gnu -### -no-integrated-as -c %s -mcpu=mips3 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS3-EB-AS %s
// MIPS3-EB-AS: as{{(.exe)?}}" "-march" "mips3" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips64-linux-gnu -### -no-integrated-as -c %s -mcpu=mips4 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS4-EB-AS %s
// MIPS4-EB-AS: as{{(.exe)?}}" "-march" "mips4" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips64-linux-gnu -### -no-integrated-as -c %s -mcpu=mips5 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS5-EB-AS %s
// MIPS5-EB-AS: as{{(.exe)?}}" "-march" "mips5" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips-linux-gnu -### -no-integrated-as -c %s -mcpu=mips32 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS32-EB-AS %s
// MIPS32-EB-AS: as{{(.exe)?}}" "-march" "mips32" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
// MIPS32-EB-AS-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-KPIC"
//
// RUN: %clang -target mips-linux-gnu -### -no-integrated-as -c %s -mcpu=mips32r6 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS32R6-EB-AS %s
// MIPS32R6-EB-AS: as{{(.exe)?}}" "-march" "mips32r6" "-mabi" "32" "-mno-shared" "-call_nonpic" "-EB"
// MIPS32R6-EB-AS-NOT: "{{[ A-Za-z\\\/]*}}as{{(.exe)?}}{{.*}}"-KPIC"
//
// RUN: %clang -target mips64-linux-gnu -### -no-integrated-as -c %s -mcpu=mips64 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS64-EB-AS %s
// MIPS64-EB-AS: as{{(.exe)?}}" "-march" "mips64" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips64-linux-gnu -### -no-integrated-as -c %s -mcpu=mips64r6 \
// RUN:   2>&1 | FileCheck -check-prefix=MIPS64R6-EB-AS %s
// MIPS64R6-EB-AS: as{{(.exe)?}}" "-march" "mips64r6" "-mabi" "64"  "-mno-shared" "-KPIC" "-EB"
//
// RUN: %clang -target mips-linux-gnu -### -no-integrated-as -msoft-float -mhard-float -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=HARDFLOAT --implicit-check-not=-msoft-float %s
// HARDFLOAT: as{{(.exe)?}}"
// HARDFLOAT: -mhard-float
//
// RUN: %clang -target mips-linux-gnu -### -no-integrated-as -mhard-float -msoft-float -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=SOFTFLOAT --implicit-check-not=-mhard-float %s
// SOFTFLOAT: as{{(.exe)?}}"
// SOFTFLOAT: -msoft-float
//
// RUN: %clang -target mips-linux-gnu -### -no-integrated-as -mno-odd-spreg -modd-spreg -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=ODDSPREG --implicit-check-not=-mno-odd-spreg %s
// ODDSPREG: as{{(.exe)?}}"
// ODDSPREG: -modd-spreg
//
// RUN: %clang -target mips-linux-gnu -### -no-integrated-as -modd-spreg -mno-odd-spreg -c %s 2>&1 \
// RUN:   | FileCheck -check-prefix=NOODDSPREG --implicit-check-not=-modd-spreg %s
// NOODDSPREG: as{{(.exe)?}}"
// NOODDSPREG: -mno-odd-spreg
