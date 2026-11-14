; SPDX-License-Identifier: BSD-2-Clause
; Authored required function constant over a real nonempty GPU kernel.
source_filename = "constants.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v96:128:128-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"
@step = external addrspace(2) constant i32
define void @add_constant(i32 addrspace(1)* %buffer, i32 %index) {
  %amount = load i32, i32 addrspace(2)* @step, align 4
  %offset = zext i32 %index to i64
  %address = getelementptr i32, i32 addrspace(1)* %buffer, i64 %offset
  %value = load i32, i32 addrspace(1)* %address, align 4
  %sum = add i32 %value, %amount
  store i32 %sum, i32 addrspace(1)* %address, align 4
  ret void
}
!air.version = !{!0}
!air.kernel = !{!1}
!air.function_constants = !{!2}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{void (i32 addrspace(1)*, i32)* @add_constant, !3, !4}
!2 = !{i32 addrspace(2)* @step, !"uint", !"step", i32 7, i1 true}
!3 = !{}
!4 = !{!5, !6}
!5 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read_write", !"air.address_space", i32 1, !"air.arg_type_name", !"uint", !"air.arg_name", !"buffer"}
!6 = !{i32 1, !"air.thread_position_in_grid", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
