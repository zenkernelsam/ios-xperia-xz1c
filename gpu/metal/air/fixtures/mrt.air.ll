; SPDX-License-Identifier: BSD-2-Clause
; Authored simultaneous2/8-target colors for actual native/GLES validation.
source_filename = "mrt.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v96:128:128-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"
%Dual = type { <4 x float>, <4 x float> }
%Ordered = type { <4 x float>, <4 x float> }
%Eight = type { <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float> }
define <4 x float> @mrt_position(<4 x float> addrspace(1)* %vertices, i32 %index) {
  %wide = zext i32 %index to i64
  %p = getelementptr <4 x float>, <4 x float> addrspace(1)* %vertices, i64 %wide
  %position = load <4 x float>, <4 x float> addrspace(1)* %p, align 16
  ret <4 x float> %position
}
define %Dual @mrt_colour() {
  ret %Dual { <4 x float> <float 0.25, float 0.5, float 0.75, float 0.5>, <4 x float> <float 0.75, float 0.25, float 0.5, float 0.25> }
}
define %Dual @mrt_changed() {
  ret %Dual { <4 x float> <float 0.5, float 0.25, float 0.125, float 0.5>, <4 x float> <float 0.25, float 0.75, float 1.0, float 0.75> }
}
define %Dual @mrt_sample(i8 addrspace(1)* %image, i8 addrspace(2)* %filtering) {
  %sample = call { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)* %image, i8 addrspace(2)* %filtering, <2 x float> <float 0.5, float 0.5>, i1 true, <2 x i32> zeroinitializer, i1 false, float 0.0, float 0.0, i32 0)
  %color = extractvalue { <4 x float>, i8 } %sample, 0
  %a = insertvalue %Dual undef, <4 x float> %color, 0
  %b = insertvalue %Dual %a, <4 x float> %color, 1
  ret %Dual %b
}
declare { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)*, i8 addrspace(2)*, <2 x float>, i1, <2 x i32>, i1, float, float, i32)
define %Ordered @mrt_order_position(<4 x float> addrspace(1)* %vertices, i32 %index, i32 %instance) {
  %wide = zext i32 %index to i64
  %p = getelementptr <4 x float>, <4 x float> addrspace(1)* %vertices, i64 %wide
  %position = load <4 x float>, <4 x float> addrspace(1)* %p, align 16
  %group = udiv i32 %index, 3
  %base = mul i32 %instance, 2
  %a = add i32 %group, %base
  %b = add i32 %a, 1
  %value = uitofp i32 %b to float
  %r = fmul float %value, 0.125
  %color = insertelement <4 x float> <float 0.0, float -0.25, float 1.125, float 0.5>, float %r, i32 0
  %out = insertvalue %Ordered undef, <4 x float> %position, 0
  %result = insertvalue %Ordered %out, <4 x float> %color, 1
  ret %Ordered %result
}
define <4 x float> @mrt_order_colour(<4 x float> %color) {
  ret <4 x float> %color
}
define %Eight @mrt_eight() {
  ret %Eight { <4 x float> <float 0.0, float 0.25, float 0.5, float 1.0>, <4 x float> <float 0.125, float 0.25, float 0.5, float 1.0>,
    <4 x float> <float 0.25, float 0.25, float 0.5, float 1.0>, <4 x float> <float 0.375, float 0.25, float 0.5, float 1.0>,
    <4 x float> <float 0.5, float 0.25, float 0.5, float 1.0>, <4 x float> <float 0.625, float 0.25, float 0.5, float 1.0>,
    <4 x float> <float 0.75, float 0.25, float 0.5, float 1.0>, <4 x float> <float 0.875, float 0.25, float 0.5, float 1.0> }
}
!air.version = !{!0}
!air.vertex = !{!1, !25}
!air.fragment = !{!2, !3, !4, !21, !26}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{<4 x float> (<4 x float> addrspace(1)*, i32)* @mrt_position, !5, !6}
!2 = !{%Dual ()* @mrt_colour, !7, !8}
!3 = !{%Dual ()* @mrt_changed, !7, !8}
!4 = !{%Eight ()* @mrt_eight, !9, !8}
!5 = !{!10}
!6 = !{!11, !12}
!7 = !{!13, !14}
!8 = !{}
!9 = !{!13, !14, !15, !16, !17, !18, !19, !20}
!10 = !{!"air.position", !"air.arg_type_name", !"float4"}
!11 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read", !"air.address_space", i32 1, !"air.arg_type_name", !"float4", !"air.arg_name", !"vertices"}
!12 = !{i32 1, !"air.vertex_id", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
!13 = !{!"air.render_target", i32 0, i32 0, !"air.arg_type_name", !"float4"}
!14 = !{!"air.render_target", i32 1, i32 0, !"air.arg_type_name", !"float4"}
!15 = !{!"air.render_target", i32 2, i32 0, !"air.arg_type_name", !"float4"}
!16 = !{!"air.render_target", i32 3, i32 0, !"air.arg_type_name", !"float4"}
!17 = !{!"air.render_target", i32 4, i32 0, !"air.arg_type_name", !"float4"}
!18 = !{!"air.render_target", i32 5, i32 0, !"air.arg_type_name", !"float4"}
!19 = !{!"air.render_target", i32 6, i32 0, !"air.arg_type_name", !"float4"}
!20 = !{!"air.render_target", i32 7, i32 0, !"air.arg_type_name", !"float4"}
!21 = !{%Dual (i8 addrspace(1)*, i8 addrspace(2)*)* @mrt_sample, !7, !22}
!22 = !{!23, !24}
!23 = !{i32 0, !"air.texture", !"air.location_index", i32 13, !"air.sample", !"air.arg_type_name", !"texture2d<float, sample>", !"air.arg_name", !"image"}
!24 = !{i32 1, !"air.sampler", !"air.location_index", i32 3, !"air.arg_type_name", !"sampler", !"air.arg_name", !"filtering"}
!25 = !{%Ordered (<4 x float> addrspace(1)*, i32, i32)* @mrt_order_position, !27, !28}
!26 = !{<4 x float> (<4 x float>)* @mrt_order_colour, !29, !30}
!27 = !{!10, !31}
!28 = !{!11, !12, !32}
!29 = !{!13}
!30 = !{!33}
!31 = !{!"air.vertex_output", !"user(order)", !"air.arg_type_name", !"float4"}
!32 = !{i32 2, !"air.instance_id", !"air.arg_type_name", !"uint", !"air.arg_name", !"instance"}
!33 = !{i32 0, !"air.fragment_input", !"user(order)", !"air.center", !"air.perspective", !"air.arg_type_name", !"float4"}
