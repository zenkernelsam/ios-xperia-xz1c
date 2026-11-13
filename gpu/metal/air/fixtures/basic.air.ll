; SPDX-License-Identifier: BSD-2-Clause
; Authored typed-pointer AIR semantic fixture, with no captured proprietary ISA.
source_filename = "basic.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v96:128:128-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"
%Position = type { <4 x float> }
define void @add_seven(i32 addrspace(1)* %buffer, i32 %index) {
  %offset = zext i32 %index to i64
  %address = getelementptr inbounds i32, i32 addrspace(1)* %buffer, i64 %offset
  %old = load i32, i32 addrspace(1)* %address, align 4
  %sum = add i32 %old, 7
  store i32 %sum, i32 addrspace(1)* %address, align 4
  ret void
}
define %Position @position(<4 x float> addrspace(1)* %buffer, i32 %index) {
  %offset = zext i32 %index to i64
  %address = getelementptr inbounds <4 x float>, <4 x float> addrspace(1)* %buffer, i64 %offset
  %vertex = load <4 x float>, <4 x float> addrspace(1)* %address, align 16
  %output = insertvalue %Position undef, <4 x float> %vertex, 0
  ret %Position %output
}
define <4 x float> @color() {
  ret <4 x float> <float 0.25, float 0.5, float 0.75, float 1.0>
}
!air.version = !{!0}
!air.kernel = !{!1}
!air.vertex = !{!2}
!air.fragment = !{!3}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{void (i32 addrspace(1)*, i32)* @add_seven, !4, !5}
!2 = !{%Position (<4 x float> addrspace(1)*, i32)* @position, !6, !7}
!3 = !{<4 x float> ()* @color, !8, !4}
!4 = !{}
!5 = !{!9, !10}
!6 = !{!11}
!7 = !{!12, !13}
!8 = !{!14}
!9 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read_write", !"air.address_space", i32 1, !"air.arg_type_name", !"uint", !"air.arg_name", !"buffer"}
!10 = !{i32 1, !"air.thread_position_in_grid", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
!11 = !{!"air.position", !"air.arg_type_name", !"float4"}
!12 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read", !"air.address_space", i32 1, !"air.arg_type_name", !"float4", !"air.arg_name", !"vertices"}
!13 = !{i32 1, !"air.vertex_id", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
!14 = !{!"air.render_target", i32 0, i32 0, !"air.arg_type_name", !"float4"}
