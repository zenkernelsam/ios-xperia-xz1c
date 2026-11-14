; SPDX-License-Identifier: BSD-2-Clause
; Author binary16 buffer arithmetic and vertex/fragment outputs for GLES fallback.
source_filename = "half.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"
define void @half_arithmetic(<4 x half> addrspace(1)* %output, <4 x half> addrspace(2)* %input, float addrspace(2)* %full, i32 %index) {
  %wide = zext i32 %index to i64
  %inputp = getelementptr <4 x half>, <4 x half> addrspace(2)* %input, i64 %wide
  %h = load <4 x half>, <4 x half> addrspace(2)* %inputp, align 8
  %added = fadd <4 x half> %h, <half 0xH1000, half 0xH9000, half 0xH3555, half 0xHB400>
  %mul = fmul <4 x half> %added, <half 0xH3800, half 0xH4000, half 0xH3E00, half 0xH3C00>
  %fp = getelementptr float, float addrspace(2)* %full, i64 %wide
  %value = load float, float addrspace(2)* %fp, align 4
  %narrow = fptrunc float %value to half
  %a = extractelement <4 x half> %h, i32 0
  %b = extractelement <4 x half> %h, i32 1
  %c = extractelement <4 x half> %h, i32 2
  %sum = fadd half %narrow, %a
  %product = fmul half %narrow, %b
  %division = fdiv half %c, 0xH4000
  %n0 = insertelement <4 x half> undef, half %narrow, i32 0
  %n1 = insertelement <4 x half> %n0, half %sum, i32 1
  %n2 = insertelement <4 x half> %n1, half %product, i32 2
  %n3 = insertelement <4 x half> %n2, half %division, i32 3
  %index0 = mul i32 %index, 3
  %index1 = add i32 %index0, 1
  %index2 = add i32 %index0, 2
  %out0 = zext i32 %index0 to i64
  %out1 = zext i32 %index1 to i64
  %out2 = zext i32 %index2 to i64
  %p0 = getelementptr <4 x half>, <4 x half> addrspace(1)* %output, i64 %out0
  %p1 = getelementptr <4 x half>, <4 x half> addrspace(1)* %output, i64 %out1
  %p2 = getelementptr <4 x half>, <4 x half> addrspace(1)* %output, i64 %out2
  store <4 x half> %h, <4 x half> addrspace(1)* %p0, align 8
  store <4 x half> %mul, <4 x half> addrspace(1)* %p1, align 8
  store <4 x half> %n3, <4 x half> addrspace(1)* %p2, align 8
  ret void
}
define <4 x float> @half_position(<4 x half> addrspace(2)* %vertices, i32 %index) {
  %wide = zext i32 %index to i64
  %p = getelementptr <4 x half>, <4 x half> addrspace(2)* %vertices, i64 %wide
  %h = load <4 x half>, <4 x half> addrspace(2)* %p, align 8
  %full = fpext <4 x half> %h to <4 x float>
  ret <4 x float> %full
}
define <4 x half> @half_colour() {
  %a = fadd <4 x half> <half 0xH3400, half 0xH3400, half 0xH3A00, half 0xH3C00>, <half 0xH0000, half 0xH3400, half 0xH0000, half 0xH0000>
  ret <4 x half> %a
}
!air.version = !{!0}
!air.kernel = !{!1}
!air.vertex = !{!2}
!air.fragment = !{!3}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{void (<4 x half> addrspace(1)*, <4 x half> addrspace(2)*, float addrspace(2)*, i32)* @half_arithmetic, !4, !5}
!2 = !{<4 x float> (<4 x half> addrspace(2)*, i32)* @half_position, !6, !7}
!3 = !{<4 x half> ()* @half_colour, !8, !4}
!4 = !{}
!5 = !{!9, !10, !11, !12}
!6 = !{!13}
!7 = !{!14, !15}
!8 = !{!16}
!9 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.write", !"air.address_space", i32 1, !"air.arg_type_name", !"half4", !"air.arg_name", !"output"}
!10 = !{i32 1, !"air.buffer", !"air.location_index", i32 1, !"air.read", !"air.address_space", i32 2, !"air.arg_type_name", !"half4", !"air.arg_name", !"input"}
!11 = !{i32 2, !"air.buffer", !"air.location_index", i32 2, !"air.read", !"air.address_space", i32 2, !"air.arg_type_name", !"float", !"air.arg_name", !"full"}
!12 = !{i32 3, !"air.thread_position_in_grid", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
!13 = !{!"air.position", !"air.arg_type_name", !"float4"}
!14 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read", !"air.address_space", i32 2, !"air.arg_type_name", !"half4", !"air.arg_name", !"vertices"}
!15 = !{i32 1, !"air.vertex_id", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
!16 = !{!"air.render_target", i32 0, i32 0, !"air.arg_type_name", !"half4"}
