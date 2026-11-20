; SPDX-License-Identifier: BSD-2-Clause
; Authored independent per-draw sample colors and positions, no captured input.
source_filename = "multisample.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v96:128:128-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"

define <4 x float> @multisample_position(<4 x float> addrspace(1)* %vertices, i32 %index) {
  %wide = zext i32 %index to i64
  %p = getelementptr <4 x float>, <4 x float> addrspace(1)* %vertices, i64 %wide
  %value = load <4 x float>, <4 x float> addrspace(1)* %p, align 16
  ret <4 x float> %value
}
define <4 x float> @multisample_color(<4 x float> addrspace(2)* %color) {
  %value = load <4 x float>, <4 x float> addrspace(2)* %color, align 16
  ret <4 x float> %value
}
!air.version = !{!0}
!air.vertex = !{!1}
!air.fragment = !{!2}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{<4 x float> (<4 x float> addrspace(1)*, i32)* @multisample_position, !3, !4}
!2 = !{<4 x float> (<4 x float> addrspace(2)*)* @multisample_color, !5, !6}
!3 = !{!7}
!4 = !{!8, !9}
!5 = !{!10}
!6 = !{!11}
!7 = !{!"air.position", !"air.arg_type_name", !"float4"}
!8 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read", !"air.address_space", i32 1, !"air.arg_type_name", !"float4", !"air.arg_name", !"vertices"}
!9 = !{i32 1, !"air.vertex_id", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
!10 = !{!"air.render_target", i32 0, i32 0, !"air.arg_type_name", !"float4"}
!11 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read", !"air.address_space", i32 2, !"air.arg_type_name", !"float4", !"air.arg_name", !"color"}
