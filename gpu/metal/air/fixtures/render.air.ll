; SPDX-License-Identifier: BSD-2-Clause
; Authored AIR graphics fixture. Semantic order deliberately differs by stage.
source_filename = "render.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v96:128:128-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"
%Position = type { <4 x float> }
%Raster = type { <4 x float>, <4 x float>, <2 x float> }
define %Position @position(<4 x float> addrspace(1)* %buffer, i32 %index) {
  %wide = zext i32 %index to i64
  %address = getelementptr <4 x float>, <4 x float> addrspace(1)* %buffer, i64 %wide
  %value = load <4 x float>, <4 x float> addrspace(1)* %address, align 16
  %result = insertvalue %Position undef, <4 x float> %value, 0
  ret %Position %result
}
define <4 x float> @color() {
  ret <4 x float> <float 0.25, float 0.5, float 0.75, float 1.0>
}
define %Raster @texture_position(<4 x float> addrspace(1)* %buffer, i32 %index) {
  %pair = mul i32 %index, 2
  %next = add i32 %pair, 1
  %offset = zext i32 %pair to i64
  %uv_offset = zext i32 %next to i64
  %p = getelementptr <4 x float>, <4 x float> addrspace(1)* %buffer, i64 %offset
  %t = getelementptr <4 x float>, <4 x float> addrspace(1)* %buffer, i64 %uv_offset
  %position = load <4 x float>, <4 x float> addrspace(1)* %p, align 16
  %texcoord = load <4 x float>, <4 x float> addrspace(1)* %t, align 16
  %uv = shufflevector <4 x float> %texcoord, <4 x float> undef, <2 x i32> <i32 0, i32 1>
  %a = insertvalue %Raster undef, <4 x float> %position, 0
  %b = insertvalue %Raster %a, <4 x float> <float 1.0, float 1.0, float 1.0, float 1.0>, 1
  %result = insertvalue %Raster %b, <2 x float> %uv, 2
  ret %Raster %result
}
define <4 x float> @textured(i8 addrspace(1)* %image, i8 addrspace(2)* %filtering, <2 x float> %pixel, <4 x float> %tint) {
  %width = call i32 @air.get_width_texture_2d(i8 addrspace(1)* %image, i32 0)
  %height = call i32 @air.get_height_texture_2d(i8 addrspace(1)* %image, i32 0)
  %w = uitofp i32 %width to float
  %h = uitofp i32 %height to float
  %a = insertelement <2 x float> undef, float %w, i32 0
  %size = insertelement <2 x float> %a, float %h, i32 1
  %uv = fdiv <2 x float> %pixel, %size
  %sample = call { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)* %image, i8 addrspace(2)* %filtering, <2 x float> %uv, i1 true, <2 x i32> zeroinitializer, i1 false, float 0.0, float 0.0, i32 0)
  %color = extractvalue { <4 x float>, i8 } %sample, 0
  %result = fmul <4 x float> %color, %tint
  ret <4 x float> %result
}
declare i32 @air.get_width_texture_2d(i8 addrspace(1)*, i32)
declare i32 @air.get_height_texture_2d(i8 addrspace(1)*, i32)
declare { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)*, i8 addrspace(2)*, <2 x float>, i1, <2 x i32>, i1, float, float, i32)
!air.version = !{!0}
!air.vertex = !{!1, !2}
!air.fragment = !{!3, !4}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{%Position (<4 x float> addrspace(1)*, i32)* @position, !5, !6}
!2 = !{%Raster (<4 x float> addrspace(1)*, i32)* @texture_position, !7, !6}
!3 = !{<4 x float> ()* @color, !8, !9}
!4 = !{<4 x float> (i8 addrspace(1)*, i8 addrspace(2)*, <2 x float>, <4 x float>)* @textured, !8, !10}
!5 = !{!11}
!6 = !{!12, !13}
!7 = !{!11, !14, !15}
!8 = !{!16}
!9 = !{}
!10 = !{!17, !18, !19, !20}
!11 = !{!"air.position", !"air.arg_type_name", !"float4"}
!12 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read", !"air.address_space", i32 1, !"air.arg_type_name", !"float4", !"air.arg_name", !"vertices"}
!13 = !{i32 1, !"air.vertex_id", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
!14 = !{!"air.vertex_output", !"user(color)", !"air.arg_type_name", !"float4"}
!15 = !{!"air.vertex_output", !"user(texcoord)", !"air.arg_type_name", !"float2"}
!16 = !{!"air.render_target", i32 0, i32 0, !"air.arg_type_name", !"float4"}
!17 = !{i32 0, !"air.texture", !"air.location_index", i32 13, !"air.sample", !"air.arg_type_name", !"texture2d<float, sample>", !"air.arg_name", !"image"}
!18 = !{i32 1, !"air.sampler", !"air.location_index", i32 3, !"air.arg_type_name", !"sampler", !"air.arg_name", !"filtering"}
!19 = !{i32 2, !"air.fragment_input", !"user(texcoord)", !"air.center", !"air.perspective", !"air.arg_type_name", !"float2"}
!20 = !{i32 3, !"air.fragment_input", !"user(color)", !"air.center", !"air.perspective", !"air.arg_type_name", !"float4"}
