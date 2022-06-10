/*
 * Copyright 2016-2022 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "config.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if !defined(restrict)
#if defined(_WIN32)
#define restrict __restrict
#elif defined(DARWIN)
#define restrict __restrict__
#endif
#endif

#define NGLI_POLY1(a, b, x)       ((a) * (x) + (b))
#define NGLI_POLY2(a, b, c, x)    (NGLI_POLY1(a, b, x) * (x) + (c))
#define NGLI_POLY3(a, b, c, d, x) (NGLI_POLY2(a, b, c, x) * (x) + (d))

#define NGLI_DEG2RAD(x) ((x) * (2.f * M_PI / 360.f))
#define NGLI_MIX(x, y, a) ((x)*(1.-(a)) + (y)*(a))
#define NGLI_LINEAR_INTERP(x, y, a) (((a) - (x)) / ((y) - (x)))
#define NGLI_CEIL_RSHIFT(a, b) -((-(a)) >> (b))

#define NGLI_VEC2_ADD(a, b) {(a)[0] + (b)[0], (a)[1] + (b)[1]}
#define NGLI_VEC3_ADD(a, b) {(a)[0] + (b)[0], (a)[1] + (b)[1], (a)[2] + (b)[2]}
#define NGLI_VEC4_ADD(a, b) {(a)[0] + (b)[0], (a)[1] + (b)[1], (a)[2] + (b)[2], (a)[3] + (b)[3]}

#define NGLI_VEC2_SUB(a, b) {(a)[0] - (b)[0], (a)[1] - (b)[1]}
#define NGLI_VEC3_SUB(a, b) {(a)[0] - (b)[0], (a)[1] - (b)[1], (a)[2] - (b)[2]}
#define NGLI_VEC4_SUB(a, b) {(a)[0] - (b)[0], (a)[1] - (b)[1], (a)[2] - (b)[2], (a)[3] - (b)[3]}

#define NGLI_VEC2_SCALE(v, s) {(v)[0] * (s), (v)[1] * (s)}
#define NGLI_VEC3_SCALE(v, s) {(v)[0] * (s), (v)[1] * (s), (v)[2] * (s)}
#define NGLI_VEC4_SCALE(v, s) {(v)[0] * (s), (v)[1] * (s), (v)[2] * (s), (v)[3] * (s)}

#define NGLI_VEC2_NEG(v) {-(v)[0], -(v)[1]}
#define NGLI_VEC3_NEG(v) {-(v)[0], -(v)[1], -(v)[2]}
#define NGLI_VEC4_NEG(v) {-(v)[0], -(v)[1], -(v)[2], -(v)[3]}

void ngli_vec2_add(float *dst, const float *v1, const float *v2);
void ngli_vec2_sub(float *dst, const float *v1, const float *v2);
void ngli_vec2_neg(float *dst, const float *v);
void ngli_vec2_scale(float *dst, const float *v, const float s);
void ngli_vec2_norm(float *dst, const float *v);
float ngli_vec2_length(const float *v);

void ngli_vec3_add(float *dst, const float *v1, const float *v2);
void ngli_vec3_sub(float *dst, const float *v1, const float *v2);
void ngli_vec3_neg(float *dst, const float *v);
void ngli_vec3_scale(float *dst, const float *v, const float s);
void ngli_vec3_norm(float *dst, const float *v);
float ngli_vec3_length(const float *v);
void ngli_vec3_cross(float *dst, const float *v1, const float *v2);
float ngli_vec3_dot(const float *v1, const float *v2);
void ngli_vec3_normalvec(float *dst, const float *a, const float *b, const float *c);

void ngli_vec4_add(float *dst, const float *v1, const float *v2);
void ngli_vec4_sub(float *dst, const float *v1, const float *v2);
void ngli_vec4_neg(float *dst, const float *v);
void ngli_vec4_scale(float *dst, const float *v, float s);
void ngli_vec4_norm(float *dst, const float *v);
float ngli_vec4_dot(const float *v1, const float *v2);
float ngli_vec4_length(const float *v);
void ngli_vec4_lerp(float *dst, const float *v1, const float *v2, float c);

void ngli_mat3_from_mat4(float *dst, const float *m);
void ngli_mat3_mul_scalar(float *dst, const float *m, float s);
void ngli_mat3_transpose(float *dst, const float *m);
float ngli_mat3_determinant(const float *m);
void ngli_mat3_adjugate(float *dst, const float* m);
void ngli_mat3_inverse(float *dst, const float *m);

#define NGLI_MAT4_IDENTITY {1.0f, 0.0f, 0.0f, 0.0f, \
                            0.0f, 1.0f, 0.0f, 0.0f, \
                            0.0f, 0.0f, 1.0f, 0.0f, \
                            0.0f, 0.0f, 0.0f, 1.0f} \

void ngli_mat4_identity(float *dst);
void ngli_mat4_mul_c(float *dst, const float *m1, const float *m2);
void ngli_mat4_mul_vec4_c(float *dst, const float *m, const float *v);
void ngli_mat4_look_at(float * restrict dst, float *eye, float *center, float *up);
void ngli_mat4_orthographic(float * restrict dst, float left, float right, float bottom, float top, float near, float far);
void ngli_mat4_perspective(float * restrict dst, float fov, float aspect, float near, float far);
void ngli_mat4_rotate(float * restrict dst, float angle, float *axis, const float *anchor);
void ngli_mat4_rotate_from_quat(float * restrict dst, const float *quat, const float *anchor);
void ngli_mat4_translate(float * restrict dst, float x, float y, float z);
void ngli_mat4_scale(float * restrict dst, float x, float y, float z, const float *anchor);
void ngli_mat4_skew(float * restrict dst, float x, float y, float z, const float *axis, const float *anchor);

/* Arch specific versions */

#ifdef ARCH_AARCH64
# define ngli_mat4_mul          ngli_mat4_mul_aarch64
# define ngli_mat4_mul_vec4     ngli_mat4_mul_vec4_aarch64
#elif defined(HAVE_X86_INTR)
# define ngli_mat4_mul          ngli_mat4_mul_sse
# define ngli_mat4_mul_vec4     ngli_mat4_mul_vec4_sse
#else
# define ngli_mat4_mul          ngli_mat4_mul_c
# define ngli_mat4_mul_vec4     ngli_mat4_mul_vec4_c
#endif

void ngli_mat4_mul_aarch64(float *dst, const float *m1, const float *m2);
void ngli_mat4_mul_vec4_aarch64(float *dst, const float *m, const float *v);
void ngli_mat4_mul_sse(float *dst, const float *m1, const float *m2);
void ngli_mat4_mul_vec4_sse(float *dst, const float *m, const float *v);

#define NGLI_QUAT_IDENTITY {0.0f, 0.0f, 0.0f, 1.0f}

void ngli_quat_slerp(float * restrict dst, const float *q1, const float *q2, float t);

#endif
