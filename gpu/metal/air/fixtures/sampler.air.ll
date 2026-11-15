; SPDX-License-Identifier: BSD-2-Clause
; Authored AIR fixture: original constexpr word, independent states and explicit LOD.
source_filename = "sampler.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v96:128:128-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"
%Position = type { <4 x float> }
@original = internal addrspace(2) constant i64 -9188470239253755319, align 8
@repeated = internal addrspace(2) constant [2 x i64] [i64 -9188470239253757806, i64 0], align 8
@mip = internal addrspace(2) constant i64 -9188470239253738935, align 8

define %Position @sampler_position(<4 x float> addrspace(1)* %buffer, i32 %index) {
  %wide = zext i32 %index to i64
  %address = getelementptr <4 x float>, <4 x float> addrspace(1)* %buffer, i64 %wide
  %value = load <4 x float>, <4 x float> addrspace(1)* %address, align 16
  %result = insertvalue %Position undef, <4 x float> %value, 0
  ret %Position %result
}
define <4 x float> @sampler_original(i8 addrspace(1)* %image) {
  %filtering = bitcast i64 addrspace(2)* @original to i8 addrspace(2)*
  %sample = call { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)* %image, i8 addrspace(2)* %filtering, <2 x float> <float 0.25, float 0.25>, i1 true, <2 x i32> zeroinitializer, i1 false, float 0.0, float 0.0, i32 0)
  %colour = extractvalue { <4 x float>, i8 } %sample, 0
  ret <4 x float> %colour
}
define <4 x float> @sampler_pair(i8 addrspace(1)* %image) {
  %edge = bitcast i64 addrspace(2)* @original to i8 addrspace(2)*
  %repeat = bitcast [2 x i64] addrspace(2)* @repeated to i8 addrspace(2)*
  %a = call { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)* %image, i8 addrspace(2)* %edge, <2 x float> <float 1.25, float 0.25>, i1 true, <2 x i32> zeroinitializer, i1 false, float 0.0, float 0.0, i32 0)
  %b = call { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)* %image, i8 addrspace(2)* %repeat, <2 x float> <float 1.25, float 0.25>, i1 true, <2 x i32> zeroinitializer, i1 false, float 0.0, float 0.0, i32 0)
  %edgeColour = extractvalue { <4 x float>, i8 } %a, 0
  %repeatColour = extractvalue { <4 x float>, i8 } %b, 0
  %red = extractelement <4 x float> %repeatColour, i32 0
  %green = extractelement <4 x float> %edgeColour, i32 1
  %r = insertelement <4 x float> <float 0.0, float 0.0, float 0.0, float 1.0>, float %red, i32 0
  %colour = insertelement <4 x float> %r, float %green, i32 1
  ret <4 x float> %colour
}
define <4 x float> @sampler_lod(i8 addrspace(1)* %image) {
  %filtering = bitcast i64 addrspace(2)* @mip to i8 addrspace(2)*
  %sample = call { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)* %image, i8 addrspace(2)* %filtering, <2 x float> <float 0.25, float 0.25>, i1 true, <2 x i32> zeroinitializer, i1 true, float 1.0, float 0.0, i32 0)
  %colour = extractvalue { <4 x float>, i8 } %sample, 0
  ret <4 x float> %colour
}
declare { <4 x float>, i8 } @air.sample_texture_2d.v4f32(i8 addrspace(1)*, i8 addrspace(2)*, <2 x float>, i1, <2 x i32>, i1, float, float, i32)
!air.version = !{!0}
!air.vertex = !{!1}
!air.fragment = !{!2, !3, !4}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{%Position (<4 x float> addrspace(1)*, i32)* @sampler_position, !5, !6}
!2 = !{<4 x float> (i8 addrspace(1)*)* @sampler_original, !7, !8}
!3 = !{<4 x float> (i8 addrspace(1)*)* @sampler_pair, !7, !8}
!4 = !{<4 x float> (i8 addrspace(1)*)* @sampler_lod, !7, !8}
!5 = !{!9}
!6 = !{!10, !11}
!7 = !{!12}
!8 = !{!13}
!9 = !{!"air.position", !"air.arg_type_name", !"float4"}
!10 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.read", !"air.address_space", i32 1, !"air.arg_type_name", !"float4", !"air.arg_name", !"vertices"}
!11 = !{i32 1, !"air.vertex_id", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
!12 = !{!"air.render_target", i32 0, i32 0, !"air.arg_type_name", !"float4"}
!13 = !{i32 0, !"air.texture", !"air.location_index", i32 5, !"air.sample", !"air.arg_type_name", !"texture2d<float, sample>", !"air.arg_name", !"image"}
