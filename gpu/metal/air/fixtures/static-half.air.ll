; SPDX-License-Identifier: BSD-2-Clause
; Authored scalar binary16 constant-memory read/write aliases.
source_filename = "static-half.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f16:16:16-f32:32:32-f64:64:64-v64:64:64-v96:128:128-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"

@input_half = external addrspace(2) constant half
@input_bits = external addrspace(2) constant i16
@word_storage = internal addrspace(2) global i16 0
@half_storage = internal addrspace(2) global half 0xH0000
@llvm.global_ctors = appending global [1 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @initialize, i8* null }]

define internal void @initialize() {
  %half_value = load half, half addrspace(2)* @input_half, align 2
  store half %half_value, half addrspace(2)* bitcast (i16 addrspace(2)* @word_storage to half addrspace(2)*), align 2
  %word_value = load i16, i16 addrspace(2)* @input_bits, align 2
  store i16 %word_value, i16 addrspace(2)* bitcast (half addrspace(2)* @half_storage to i16 addrspace(2)*), align 2
  ret void
}

define void @static_half(i32 addrspace(1)* %buffer) {
  %a = load i16, i16 addrspace(2)* @word_storage, align 2
  %b = load half, half addrspace(2)* bitcast (i16 addrspace(2)* @word_storage to half addrspace(2)*), align 2
  %c = load half, half addrspace(2)* @half_storage, align 2
  %d = load i16, i16 addrspace(2)* bitcast (half addrspace(2)* @half_storage to i16 addrspace(2)*), align 2
  %b_bits = bitcast half %b to i16
  %c_bits = bitcast half %c to i16
  %a32 = zext i16 %a to i32
  %b32 = zext i16 %b_bits to i32
  %c32 = zext i16 %c_bits to i32
  %d32 = zext i16 %d to i32
  %p1 = getelementptr i32, i32 addrspace(1)* %buffer, i64 1
  %p2 = getelementptr i32, i32 addrspace(1)* %buffer, i64 2
  %p3 = getelementptr i32, i32 addrspace(1)* %buffer, i64 3
  store i32 %a32, i32 addrspace(1)* %buffer, align 4
  store i32 %b32, i32 addrspace(1)* %p1, align 4
  store i32 %c32, i32 addrspace(1)* %p2, align 4
  store i32 %d32, i32 addrspace(1)* %p3, align 4
  ret void
}
!air.version = !{!0}
!air.kernel = !{!1}
!air.function_constants = !{!2, !3}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{void (i32 addrspace(1)*)* @static_half, !4, !5}
!2 = !{half addrspace(2)* @input_half, !"half", !"input_half", i32 1, i1 true}
!3 = !{i16 addrspace(2)* @input_bits, !"ushort", !"input_bits", i32 2, i1 true}
!4 = !{}
!5 = !{!6}
!6 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read_write", !"air.address_space", i32 1, !"air.arg_type_name", !"uint", !"air.arg_name", !"buffer"}
