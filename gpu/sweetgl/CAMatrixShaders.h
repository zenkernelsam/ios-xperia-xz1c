/* SPDX-License-Identifier: BSD-2-Clause
 * The existing matrix equations, shared with their real GLI acceptance probe. */
#pragma once
static const char vcm_func[] =
    "lowp vec4 sgl_vcm(lowp vec4 s) {\n"
    "  lowp vec4 d = gl_LastFragData[0];\n"
    "  lowp vec4 u = vec4(d.rgb / max(d.a, .005), d.a);\n"
    "  u = clamp(u.r * color_matrix[0] + u.g * color_matrix[1] + u.b * color_matrix[2]\n"
    "    + u.a * color_matrix[3] + color_matrix[4], 0., 1.);\n"
    "  u.rgb *= u.a;\n"
    "  return mix(d, d.a > 0. ? u : vec4(0.), s.a);\n"
    "}\n";
static const char cm4_func[] =
    "lowp vec4 sgl_cm4(lowp vec4 s) {\n"
    "  lowp vec4 u = clamp(s.r * color_matrix[0] + s.g * color_matrix[1] + s.b * color_matrix[2]\n"
    "    + s.a * color_matrix[3] + color_matrix[4], 0., 1.);\n"
    "  return vec4(min(u.rgb, u.a), u.a);\n"
    "}\n";
