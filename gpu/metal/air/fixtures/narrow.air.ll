; SPDX-License-Identifier: BSD-2-Clause
; Authored dynamic packed8/16-bit integer conversion, original buffers and signs.
source_filename = "narrow.air"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-n8:16:32"
target triple = "air64-apple-ios16.0"
define void @narrow_convert(<4 x i32> addrspace(1)* %output, <2 x i16> addrspace(2)* %shorts, <2 x i8> addrspace(2)* %bytes, i32 %index) {
  %wide = zext i32 %index to i64
  %sp = getelementptr <2 x i16>, <2 x i16> addrspace(2)* %shorts, i64 %wide
  %bp = getelementptr <2 x i8>, <2 x i8> addrspace(2)* %bytes, i64 %wide
  %sv = load <2 x i16>, <2 x i16> addrspace(2)* %sp, align 4
  %bv = load <2 x i8>, <2 x i8> addrspace(2)* %bp, align 2
  %s = extractelement <2 x i16> %sv, i32 0
  %b = extractelement <2 x i8> %bv, i32 1
  %sf = call float @air.convert.f.f32.s.i16(i16 %s)
  %uf = call float @air.convert.f.f32.u.i16(i16 %s)
  %bf = call float @air.convert.f.f32.s.i8(i8 %b)
  %ubf = call float @air.convert.f.f32.u.i8(i8 %b)
  %a = insertelement <4 x float> undef, float %sf, i32 0
  %c = insertelement <4 x float> %a, float %uf, i32 1
  %d = insertelement <4 x float> %c, float %bf, i32 2
  %scalar = insertelement <4 x float> %d, float %ubf, i32 3
  %signed16 = call <2 x float> @air.convert.f.v2f32.s.v2i16(<2 x i16> %sv)
  %unsigned16 = call <2 x float> @air.convert.f.v2f32.u.v2i16(<2 x i16> %sv)
  %signed8 = call <2 x float> @air.convert.f.v2f32.s.v2i8(<2 x i8> %bv)
  %unsigned8 = call <2 x float> @air.convert.f.v2f32.u.v2i8(<2 x i8> %bv)
  %v16 = shufflevector <2 x float> %signed16, <2 x float> %unsigned16, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %v8 = shufflevector <2 x float> %signed8, <2 x float> %unsigned8, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %base32 = mul i32 %index, 3
  %next32 = add i32 %base32, 1
  %last32 = add i32 %base32, 2
  %base = zext i32 %base32 to i64
  %next = zext i32 %next32 to i64
  %last = zext i32 %last32 to i64
  %dest0 = getelementptr <4 x i32>, <4 x i32> addrspace(1)* %output, i64 %base
  %dest1 = getelementptr <4 x i32>, <4 x i32> addrspace(1)* %output, i64 %next
  %dest2 = getelementptr <4 x i32>, <4 x i32> addrspace(1)* %output, i64 %last
  %bits0 = bitcast <4 x float> %scalar to <4 x i32>
  %bits1 = bitcast <4 x float> %v16 to <4 x i32>
  %bits2 = bitcast <4 x float> %v8 to <4 x i32>
  store <4 x i32> %bits0, <4 x i32> addrspace(1)* %dest0, align 16
  store <4 x i32> %bits1, <4 x i32> addrspace(1)* %dest1, align 16
  store <4 x i32> %bits2, <4 x i32> addrspace(1)* %dest2, align 16
  ret void
}
declare float @air.convert.f.f32.s.i16(i16)
declare float @air.convert.f.f32.u.i16(i16)
declare float @air.convert.f.f32.s.i8(i8)
declare float @air.convert.f.f32.u.i8(i8)
declare <2 x float> @air.convert.f.v2f32.s.v2i16(<2 x i16>)
declare <2 x float> @air.convert.f.v2f32.u.v2i16(<2 x i16>)
declare <2 x float> @air.convert.f.v2f32.s.v2i8(<2 x i8>)
declare <2 x float> @air.convert.f.v2f32.u.v2i8(<2 x i8>)
!air.version = !{!0}
!air.kernel = !{!1}
!0 = !{i32 2, i32 0, i32 0}
!1 = !{void (<4 x i32> addrspace(1)*, <2 x i16> addrspace(2)*, <2 x i8> addrspace(2)*, i32)* @narrow_convert, !2, !3}
!2 = !{}
!3 = !{!4, !5, !6, !7}
!4 = !{i32 0, !"air.buffer", !"air.location_index", i32 0, !"air.write", !"air.address_space", i32 1, !"air.arg_type_name", !"uint4", !"air.arg_name", !"output"}
!5 = !{i32 1, !"air.buffer", !"air.location_index", i32 1, !"air.read", !"air.address_space", i32 2, !"air.arg_type_size", i32 4, !"air.arg_type_name", !"short2", !"air.arg_name", !"shorts"}
!6 = !{i32 2, !"air.buffer", !"air.location_index", i32 2, !"air.read", !"air.address_space", i32 2, !"air.arg_type_size", i32 2, !"air.arg_type_name", !"char2", !"air.arg_name", !"bytes"}
!7 = !{i32 3, !"air.thread_position_in_grid", !"air.arg_type_name", !"uint", !"air.arg_name", !"index"}
