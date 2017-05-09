; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+bmi,+bmi2 | FileCheck %s

declare i8 @llvm.cttz.i8(i8, i1)
declare i16 @llvm.cttz.i16(i16, i1)
declare i32 @llvm.cttz.i32(i32, i1)
declare i64 @llvm.cttz.i64(i64, i1)

define i8 @t1(i8 %x)   {
; CHECK-LABEL: t1:
; CHECK:       # BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    orl $256, %eax # imm = 0x100
; CHECK-NEXT:    tzcntl %eax, %eax
; CHECK-NEXT:    # kill: %AL<def> %AL<kill> %EAX<kill>
; CHECK-NEXT:    retq
  %tmp = tail call i8 @llvm.cttz.i8( i8 %x, i1 false )
  ret i8 %tmp
}

define i16 @t2(i16 %x)   {
; CHECK-LABEL: t2:
; CHECK:       # BB#0:
; CHECK-NEXT:    tzcntw %di, %ax
; CHECK-NEXT:    retq
  %tmp = tail call i16 @llvm.cttz.i16( i16 %x, i1 false )
  ret i16 %tmp
}

define i32 @t3(i32 %x)   {
; CHECK-LABEL: t3:
; CHECK:       # BB#0:
; CHECK-NEXT:    tzcntl %edi, %eax
; CHECK-NEXT:    retq
  %tmp = tail call i32 @llvm.cttz.i32( i32 %x, i1 false )
  ret i32 %tmp
}

define i32 @tzcnt32_load(i32* %x)   {
; CHECK-LABEL: tzcnt32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    tzcntl (%rdi), %eax
; CHECK-NEXT:    retq
  %x1 = load i32, i32* %x
  %tmp = tail call i32 @llvm.cttz.i32(i32 %x1, i1 false )
  ret i32 %tmp
}

define i64 @t4(i64 %x)   {
; CHECK-LABEL: t4:
; CHECK:       # BB#0:
; CHECK-NEXT:    tzcntq %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = tail call i64 @llvm.cttz.i64( i64 %x, i1 false )
  ret i64 %tmp
}

define i8 @t5(i8 %x)   {
; CHECK-LABEL: t5:
; CHECK:       # BB#0:
; CHECK-NEXT:    movzbl %dil, %eax
; CHECK-NEXT:    tzcntl %eax, %eax
; CHECK-NEXT:    # kill: %AL<def> %AL<kill> %EAX<kill>
; CHECK-NEXT:    retq
  %tmp = tail call i8 @llvm.cttz.i8( i8 %x, i1 true )
  ret i8 %tmp
}

define i16 @t6(i16 %x)   {
; CHECK-LABEL: t6:
; CHECK:       # BB#0:
; CHECK-NEXT:    tzcntw %di, %ax
; CHECK-NEXT:    retq
  %tmp = tail call i16 @llvm.cttz.i16( i16 %x, i1 true )
  ret i16 %tmp
}

define i32 @t7(i32 %x)   {
; CHECK-LABEL: t7:
; CHECK:       # BB#0:
; CHECK-NEXT:    tzcntl %edi, %eax
; CHECK-NEXT:    retq
  %tmp = tail call i32 @llvm.cttz.i32( i32 %x, i1 true )
  ret i32 %tmp
}

define i64 @t8(i64 %x)   {
; CHECK-LABEL: t8:
; CHECK:       # BB#0:
; CHECK-NEXT:    tzcntq %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = tail call i64 @llvm.cttz.i64( i64 %x, i1 true )
  ret i64 %tmp
}

define i32 @andn32(i32 %x, i32 %y)   {
; CHECK-LABEL: andn32:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnl %esi, %edi, %eax
; CHECK-NEXT:    retq
  %tmp1 = xor i32 %x, -1
  %tmp2 = and i32 %y, %tmp1
  ret i32 %tmp2
}

define i32 @andn32_load(i32 %x, i32* %y)   {
; CHECK-LABEL: andn32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnl (%rsi), %edi, %eax
; CHECK-NEXT:    retq
  %y1 = load i32, i32* %y
  %tmp1 = xor i32 %x, -1
  %tmp2 = and i32 %y1, %tmp1
  ret i32 %tmp2
}

define i64 @andn64(i64 %x, i64 %y)   {
; CHECK-LABEL: andn64:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
  %tmp1 = xor i64 %x, -1
  %tmp2 = and i64 %tmp1, %y
  ret i64 %tmp2
}

; Don't choose a 'test' if an 'andn' can be used.
define i1 @andn_cmp(i32 %x, i32 %y) {
; CHECK-LABEL: andn_cmp:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnl %esi, %edi, %eax
; CHECK-NEXT:    sete %al
; CHECK-NEXT:    retq
  %notx = xor i32 %x, -1
  %and = and i32 %notx, %y
  %cmp = icmp eq i32 %and, 0
  ret i1 %cmp
}

; Recognize a disguised andn in the following 4 tests.
define i1 @and_cmp1(i32 %x, i32 %y) {
; CHECK-LABEL: and_cmp1:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnl %esi, %edi, %eax
; CHECK-NEXT:    sete %al
; CHECK-NEXT:    retq
  %and = and i32 %x, %y
  %cmp = icmp eq i32 %and, %y
  ret i1 %cmp
}

define i1 @and_cmp2(i32 %x, i32 %y) {
; CHECK-LABEL: and_cmp2:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnl %esi, %edi, %eax
; CHECK-NEXT:    setne %al
; CHECK-NEXT:    retq
  %and = and i32 %y, %x
  %cmp = icmp ne i32 %and, %y
  ret i1 %cmp
}

define i1 @and_cmp3(i32 %x, i32 %y) {
; CHECK-LABEL: and_cmp3:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnl %esi, %edi, %eax
; CHECK-NEXT:    sete %al
; CHECK-NEXT:    retq
  %and = and i32 %x, %y
  %cmp = icmp eq i32 %y, %and
  ret i1 %cmp
}

define i1 @and_cmp4(i32 %x, i32 %y) {
; CHECK-LABEL: and_cmp4:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnl %esi, %edi, %eax
; CHECK-NEXT:    setne %al
; CHECK-NEXT:    retq
  %and = and i32 %y, %x
  %cmp = icmp ne i32 %y, %and
  ret i1 %cmp
}

; A mask and compare against constant is ok for an 'andn' too
; even though the BMI instruction doesn't have an immediate form.
define i1 @and_cmp_const(i32 %x) {
; CHECK-LABEL: and_cmp_const:
; CHECK:       # BB#0:
; CHECK-NEXT:    movl $43, %eax
; CHECK-NEXT:    andnl %eax, %edi, %eax
; CHECK-NEXT:    sete %al
; CHECK-NEXT:    retq
  %and = and i32 %x, 43
  %cmp = icmp eq i32 %and, 43
  ret i1 %cmp
}

; But don't use 'andn' if the mask is a power-of-two.
define i1 @and_cmp_const_power_of_two(i32 %x, i32 %y) {
; CHECK-LABEL: and_cmp_const_power_of_two:
; CHECK:       # BB#0:
; CHECK-NEXT:    btl %esi, %edi
; CHECK-NEXT:    setae %al
; CHECK-NEXT:    retq
  %shl = shl i32 1, %y
  %and = and i32 %x, %shl
  %cmp = icmp ne i32 %and, %shl
  ret i1 %cmp
}

; Don't transform to 'andn' if there's another use of the 'and'.
define i32 @and_cmp_not_one_use(i32 %x) {
; CHECK-LABEL: and_cmp_not_one_use:
; CHECK:       # BB#0:
; CHECK-NEXT:    andl $37, %edi
; CHECK-NEXT:    xorl %eax, %eax
; CHECK-NEXT:    cmpl $37, %edi
; CHECK-NEXT:    sete %al
; CHECK-NEXT:    addl %edi, %eax
; CHECK-NEXT:    retq
  %and = and i32 %x, 37
  %cmp = icmp eq i32 %and, 37
  %ext = zext i1 %cmp to i32
  %add = add i32 %and, %ext
  ret i32 %add
}

; Verify that we're not transforming invalid comparison predicates.
define i1 @not_an_andn1(i32 %x, i32 %y) {
; CHECK-LABEL: not_an_andn1:
; CHECK:       # BB#0:
; CHECK-NEXT:    andl %esi, %edi
; CHECK-NEXT:    cmpl %edi, %esi
; CHECK-NEXT:    setg %al
; CHECK-NEXT:    retq
  %and = and i32 %x, %y
  %cmp = icmp sgt i32 %y, %and
  ret i1 %cmp
}

define i1 @not_an_andn2(i32 %x, i32 %y) {
; CHECK-LABEL: not_an_andn2:
; CHECK:       # BB#0:
; CHECK-NEXT:    andl %esi, %edi
; CHECK-NEXT:    cmpl %edi, %esi
; CHECK-NEXT:    setbe %al
; CHECK-NEXT:    retq
  %and = and i32 %y, %x
  %cmp = icmp ule i32 %y, %and
  ret i1 %cmp
}

; Don't choose a 'test' if an 'andn' can be used.
define i1 @andn_cmp_swap_ops(i64 %x, i64 %y) {
; CHECK-LABEL: andn_cmp_swap_ops:
; CHECK:       # BB#0:
; CHECK-NEXT:    andnq %rsi, %rdi, %rax
; CHECK-NEXT:    sete %al
; CHECK-NEXT:    retq
  %notx = xor i64 %x, -1
  %and = and i64 %y, %notx
  %cmp = icmp eq i64 %and, 0
  ret i1 %cmp
}

; Use a 'test' (not an 'and') because 'andn' only works for i32/i64.
define i1 @andn_cmp_i8(i8 %x, i8 %y) {
; CHECK-LABEL: andn_cmp_i8:
; CHECK:       # BB#0:
; CHECK-NEXT:    notb %sil
; CHECK-NEXT:    testb %sil, %dil
; CHECK-NEXT:    sete %al
; CHECK-NEXT:    retq
  %noty = xor i8 %y, -1
  %and = and i8 %x, %noty
  %cmp = icmp eq i8 %and, 0
  ret i1 %cmp
}

define i32 @bextr32(i32 %x, i32 %y)   {
; CHECK-LABEL: bextr32:
; CHECK:       # BB#0:
; CHECK-NEXT:    bextrl %esi, %edi, %eax
; CHECK-NEXT:    retq
  %tmp = tail call i32 @llvm.x86.bmi.bextr.32(i32 %x, i32 %y)
  ret i32 %tmp
}

define i32 @bextr32_load(i32* %x, i32 %y)   {
; CHECK-LABEL: bextr32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    bextrl %esi, (%rdi), %eax
; CHECK-NEXT:    retq
  %x1 = load i32, i32* %x
  %tmp = tail call i32 @llvm.x86.bmi.bextr.32(i32 %x1, i32 %y)
  ret i32 %tmp
}

declare i32 @llvm.x86.bmi.bextr.32(i32, i32)

define i32 @bextr32b(i32 %x)  uwtable  ssp {
; CHECK-LABEL: bextr32b:
; CHECK:       # BB#0:
; CHECK-NEXT:    movl $3076, %eax # imm = 0xC04
; CHECK-NEXT:    bextrl %eax, %edi, %eax
; CHECK-NEXT:    retq
  %1 = lshr i32 %x, 4
  %2 = and i32 %1, 4095
  ret i32 %2
}

define i32 @bextr32b_load(i32* %x)  uwtable  ssp {
; CHECK-LABEL: bextr32b_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    movl $3076, %eax # imm = 0xC04
; CHECK-NEXT:    bextrl %eax, (%rdi), %eax
; CHECK-NEXT:    retq
  %1 = load i32, i32* %x
  %2 = lshr i32 %1, 4
  %3 = and i32 %2, 4095
  ret i32 %3
}

define i64 @bextr64(i64 %x, i64 %y)   {
; CHECK-LABEL: bextr64:
; CHECK:       # BB#0:
; CHECK-NEXT:    bextrq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = tail call i64 @llvm.x86.bmi.bextr.64(i64 %x, i64 %y)
  ret i64 %tmp
}

declare i64 @llvm.x86.bmi.bextr.64(i64, i64)

define i64 @bextr64b(i64 %x)  uwtable  ssp {
; CHECK-LABEL: bextr64b:
; CHECK:       # BB#0:
; CHECK-NEXT:    movl $3076, %eax # imm = 0xC04
; CHECK-NEXT:    bextrl %eax, %edi, %eax
; CHECK-NEXT:    retq
  %1 = lshr i64 %x, 4
  %2 = and i64 %1, 4095
  ret i64 %2
}

define i64 @bextr64b_load(i64* %x) {
; CHECK-LABEL: bextr64b_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    movl $3076, %eax # imm = 0xC04
; CHECK-NEXT:    bextrl %eax, (%rdi), %eax
; CHECK-NEXT:    retq
  %1 = load i64, i64* %x, align 8
  %2 = lshr i64 %1, 4
  %3 = and i64 %2, 4095
  ret i64 %3
}

define i32 @non_bextr32(i32 %x) {
; CHECK-LABEL: non_bextr32:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    shrl $2, %edi
; CHECK-NEXT:    andl $111, %edi
; CHECK-NEXT:    movl %edi, %eax
; CHECK-NEXT:    retq
entry:
  %shr = lshr i32 %x, 2
  %and = and i32 %shr, 111
  ret i32 %and
}

define i64 @non_bextr64(i64 %x) {
; CHECK-LABEL: non_bextr64:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    shrq $2, %rdi
; CHECK-NEXT:    movabsq $8589934590, %rax # imm = 0x1FFFFFFFE
; CHECK-NEXT:    andq %rdi, %rax
; CHECK-NEXT:    retq
entry:
  %shr = lshr i64 %x, 2
  %and = and i64 %shr, 8589934590
  ret i64 %and
}

define i32 @bzhi32(i32 %x, i32 %y)   {
; CHECK-LABEL: bzhi32:
; CHECK:       # BB#0:
; CHECK-NEXT:    bzhil %esi, %edi, %eax
; CHECK-NEXT:    retq
  %tmp = tail call i32 @llvm.x86.bmi.bzhi.32(i32 %x, i32 %y)
  ret i32 %tmp
}

define i32 @bzhi32_load(i32* %x, i32 %y)   {
; CHECK-LABEL: bzhi32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    bzhil %esi, (%rdi), %eax
; CHECK-NEXT:    retq
  %x1 = load i32, i32* %x
  %tmp = tail call i32 @llvm.x86.bmi.bzhi.32(i32 %x1, i32 %y)
  ret i32 %tmp
}

declare i32 @llvm.x86.bmi.bzhi.32(i32, i32)

define i64 @bzhi64(i64 %x, i64 %y)   {
; CHECK-LABEL: bzhi64:
; CHECK:       # BB#0:
; CHECK-NEXT:    bzhiq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = tail call i64 @llvm.x86.bmi.bzhi.64(i64 %x, i64 %y)
  ret i64 %tmp
}

declare i64 @llvm.x86.bmi.bzhi.64(i64, i64)

define i32 @bzhi32b(i32 %x, i8 zeroext %index) {
; CHECK-LABEL: bzhi32b:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    bzhil %esi, %edi, %eax
; CHECK-NEXT:    retq
entry:
  %conv = zext i8 %index to i32
  %shl = shl i32 1, %conv
  %sub = add nsw i32 %shl, -1
  %and = and i32 %sub, %x
  ret i32 %and
}

define i32 @bzhi32b_load(i32* %w, i8 zeroext %index) {
; CHECK-LABEL: bzhi32b_load:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    bzhil %esi, (%rdi), %eax
; CHECK-NEXT:    retq
entry:
  %x = load i32, i32* %w
  %conv = zext i8 %index to i32
  %shl = shl i32 1, %conv
  %sub = add nsw i32 %shl, -1
  %and = and i32 %sub, %x
  ret i32 %and
}

define i32 @bzhi32c(i32 %x, i8 zeroext %index) {
; CHECK-LABEL: bzhi32c:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    bzhil %esi, %edi, %eax
; CHECK-NEXT:    retq
entry:
  %conv = zext i8 %index to i32
  %shl = shl i32 1, %conv
  %sub = add nsw i32 %shl, -1
  %and = and i32 %x, %sub
  ret i32 %and
}

define i32 @bzhi32d(i32 %a, i32 %b) {
; CHECK-LABEL: bzhi32d:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    bzhil %esi, %edi, %eax
; CHECK-NEXT:    retq
entry:
  %sub = sub i32 32, %b
  %shr = lshr i32 -1, %sub
  %and = and i32 %shr, %a
  ret i32 %and
}

define i32 @bzhi32e(i32 %a, i32 %b) {
; CHECK-LABEL: bzhi32e:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    bzhil %esi, %edi, %eax
; CHECK-NEXT:    retq
entry:
  %sub = sub i32 32, %b
  %shl = shl i32 %a, %sub
  %shr = lshr i32 %shl, %sub
  ret i32 %shr
}

define i64 @bzhi64b(i64 %x, i8 zeroext %index) {
; CHECK-LABEL: bzhi64b:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    # kill: %ESI<def> %ESI<kill> %RSI<def>
; CHECK-NEXT:    bzhiq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
entry:
  %conv = zext i8 %index to i64
  %shl = shl i64 1, %conv
  %sub = add nsw i64 %shl, -1
  %and = and i64 %x, %sub
  ret i64 %and
}

define i64 @bzhi64c(i64 %a, i64 %b) {
; CHECK-LABEL: bzhi64c:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    bzhiq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
entry:
  %sub = sub i64 64, %b
  %shr = lshr i64 -1, %sub
  %and = and i64 %shr, %a
  ret i64 %and
}

define i64 @bzhi64d(i64 %a, i32 %b) {
; CHECK-LABEL: bzhi64d:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    # kill: %ESI<def> %ESI<kill> %RSI<def>
; CHECK-NEXT:    bzhiq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
entry:
  %sub = sub i32 64, %b
  %sh_prom = zext i32 %sub to i64
  %shr = lshr i64 -1, %sh_prom
  %and = and i64 %shr, %a
  ret i64 %and
}

define i64 @bzhi64e(i64 %a, i64 %b) {
; CHECK-LABEL: bzhi64e:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    bzhiq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
entry:
  %sub = sub i64 64, %b
  %shl = shl i64 %a, %sub
  %shr = lshr i64 %shl, %sub
  ret i64 %shr
}

define i64 @bzhi64f(i64 %a, i32 %b) {
; CHECK-LABEL: bzhi64f:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    # kill: %ESI<def> %ESI<kill> %RSI<def>
; CHECK-NEXT:    bzhiq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
entry:
  %sub = sub i32 64, %b
  %sh_prom = zext i32 %sub to i64
  %shl = shl i64 %a, %sh_prom
  %shr = lshr i64 %shl, %sh_prom
  ret i64 %shr
}

define i64 @bzhi64_constant_mask(i64 %x) {
; CHECK-LABEL: bzhi64_constant_mask:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    movb $62, %al
; CHECK-NEXT:    bzhiq %rax, %rdi, %rax
; CHECK-NEXT:    retq
entry:
  %and = and i64 %x, 4611686018427387903
  ret i64 %and
}

define i64 @bzhi64_small_constant_mask(i64 %x) {
; CHECK-LABEL: bzhi64_small_constant_mask:
; CHECK:       # BB#0: # %entry
; CHECK-NEXT:    andl $2147483647, %edi # imm = 0x7FFFFFFF
; CHECK-NEXT:    movq %rdi, %rax
; CHECK-NEXT:    retq
entry:
  %and = and i64 %x, 2147483647
  ret i64 %and
}

define i32 @blsi32(i32 %x)   {
; CHECK-LABEL: blsi32:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsil %edi, %eax
; CHECK-NEXT:    retq
  %tmp = sub i32 0, %x
  %tmp2 = and i32 %x, %tmp
  ret i32 %tmp2
}

define i32 @blsi32_load(i32* %x)   {
; CHECK-LABEL: blsi32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsil (%rdi), %eax
; CHECK-NEXT:    retq
  %x1 = load i32, i32* %x
  %tmp = sub i32 0, %x1
  %tmp2 = and i32 %x1, %tmp
  ret i32 %tmp2
}

define i64 @blsi64(i64 %x)   {
; CHECK-LABEL: blsi64:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsiq %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = sub i64 0, %x
  %tmp2 = and i64 %tmp, %x
  ret i64 %tmp2
}

define i32 @blsmsk32(i32 %x)   {
; CHECK-LABEL: blsmsk32:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsmskl %edi, %eax
; CHECK-NEXT:    retq
  %tmp = sub i32 %x, 1
  %tmp2 = xor i32 %x, %tmp
  ret i32 %tmp2
}

define i32 @blsmsk32_load(i32* %x)   {
; CHECK-LABEL: blsmsk32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsmskl (%rdi), %eax
; CHECK-NEXT:    retq
  %x1 = load i32, i32* %x
  %tmp = sub i32 %x1, 1
  %tmp2 = xor i32 %x1, %tmp
  ret i32 %tmp2
}

define i64 @blsmsk64(i64 %x)   {
; CHECK-LABEL: blsmsk64:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsmskq %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = sub i64 %x, 1
  %tmp2 = xor i64 %tmp, %x
  ret i64 %tmp2
}

define i32 @blsr32(i32 %x)   {
; CHECK-LABEL: blsr32:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsrl %edi, %eax
; CHECK-NEXT:    retq
  %tmp = sub i32 %x, 1
  %tmp2 = and i32 %x, %tmp
  ret i32 %tmp2
}

define i32 @blsr32_load(i32* %x)   {
; CHECK-LABEL: blsr32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsrl (%rdi), %eax
; CHECK-NEXT:    retq
  %x1 = load i32, i32* %x
  %tmp = sub i32 %x1, 1
  %tmp2 = and i32 %x1, %tmp
  ret i32 %tmp2
}

define i64 @blsr64(i64 %x)   {
; CHECK-LABEL: blsr64:
; CHECK:       # BB#0:
; CHECK-NEXT:    blsrq %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = sub i64 %x, 1
  %tmp2 = and i64 %tmp, %x
  ret i64 %tmp2
}

define i32 @pdep32(i32 %x, i32 %y)   {
; CHECK-LABEL: pdep32:
; CHECK:       # BB#0:
; CHECK-NEXT:    pdepl %esi, %edi, %eax
; CHECK-NEXT:    retq
  %tmp = tail call i32 @llvm.x86.bmi.pdep.32(i32 %x, i32 %y)
  ret i32 %tmp
}

define i32 @pdep32_load(i32 %x, i32* %y)   {
; CHECK-LABEL: pdep32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    pdepl (%rsi), %edi, %eax
; CHECK-NEXT:    retq
  %y1 = load i32, i32* %y
  %tmp = tail call i32 @llvm.x86.bmi.pdep.32(i32 %x, i32 %y1)
  ret i32 %tmp
}

declare i32 @llvm.x86.bmi.pdep.32(i32, i32)

define i64 @pdep64(i64 %x, i64 %y)   {
; CHECK-LABEL: pdep64:
; CHECK:       # BB#0:
; CHECK-NEXT:    pdepq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = tail call i64 @llvm.x86.bmi.pdep.64(i64 %x, i64 %y)
  ret i64 %tmp
}

declare i64 @llvm.x86.bmi.pdep.64(i64, i64)

define i32 @pext32(i32 %x, i32 %y)   {
; CHECK-LABEL: pext32:
; CHECK:       # BB#0:
; CHECK-NEXT:    pextl %esi, %edi, %eax
; CHECK-NEXT:    retq
  %tmp = tail call i32 @llvm.x86.bmi.pext.32(i32 %x, i32 %y)
  ret i32 %tmp
}

define i32 @pext32_load(i32 %x, i32* %y)   {
; CHECK-LABEL: pext32_load:
; CHECK:       # BB#0:
; CHECK-NEXT:    pextl (%rsi), %edi, %eax
; CHECK-NEXT:    retq
  %y1 = load i32, i32* %y
  %tmp = tail call i32 @llvm.x86.bmi.pext.32(i32 %x, i32 %y1)
  ret i32 %tmp
}

declare i32 @llvm.x86.bmi.pext.32(i32, i32)

define i64 @pext64(i64 %x, i64 %y)   {
; CHECK-LABEL: pext64:
; CHECK:       # BB#0:
; CHECK-NEXT:    pextq %rsi, %rdi, %rax
; CHECK-NEXT:    retq
  %tmp = tail call i64 @llvm.x86.bmi.pext.64(i64 %x, i64 %y)
  ret i64 %tmp
}

declare i64 @llvm.x86.bmi.pext.64(i64, i64)

