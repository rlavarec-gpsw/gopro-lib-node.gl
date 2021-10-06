/*
 * Copyright 2016 GoPro Inc.
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

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "math_utils.h"
#include "nodegl.h"
#include "internal.h"
#include "transforms.h"
#include "type.h"


static int is_live_update_supported(const struct ngl_node *node)
{
    struct variable_priv *s = node->priv_data;
    if (s->dynamic) {
        LOG(ERROR, "updating data on a dynamic uniform is unsupported");
        return NGL_ERROR_INVALID_USAGE;
    }
    return 0;
}

#define DECLARE_UPDATE_FUNC(name, src)                        \
static int uniform##name##_update_func(struct ngl_node *node) \
{                                                             \
    int ret = is_live_update_supported(node);                 \
    if (ret < 0)                                              \
        return ret;                                           \
    struct variable_priv *s = node->priv_data;                \
    memcpy(s->data, src, s->data_size);                       \
    return 0;                                                 \
}                                                             \

DECLARE_UPDATE_FUNC(ivec,  s->opt.ivec)
DECLARE_UPDATE_FUNC(uivec, s->opt.uvec)
DECLARE_UPDATE_FUNC(vec,   s->opt.vec)
DECLARE_UPDATE_FUNC(mat4,  s->opt.mat)

static int uniformquat_update_func(struct ngl_node *node)
{
    int ret = is_live_update_supported(node);
    if (ret < 0)
        return ret;
    struct variable_priv *s = node->priv_data;
    memcpy(s->vector, s->opt.vec, s->data_size);
    if (s->as_mat4)
        ngli_mat4_rotate_from_quat(s->matrix, s->vector);
    return 0;
}

#define OFFSET(x) offsetof(struct variable_priv, x)

#define DECLARE_PARAMS(type, ptype, dst, upd_fn)                        \
static const struct node_param uniform##type##_params[] = {             \
    {"value",  ptype, OFFSET(dst),                                      \
               .flags=NGLI_PARAM_FLAG_ALLOW_LIVE_CHANGE,                     \
               .update_func=upd_fn,                                     \
               .desc=NGLI_DOCSTRING("value exposed to the shader")},    \
    {NULL}                                                              \
}

DECLARE_PARAMS(bool,   NGLI_PARAM_TYPE_BOOL,  opt.ivec, uniformivec_update_func);
DECLARE_PARAMS(float,  NGLI_PARAM_TYPE_F32,   opt.vec,  uniformvec_update_func);
DECLARE_PARAMS(vec2,   NGLI_PARAM_TYPE_VEC2,  opt.vec,  uniformvec_update_func);
DECLARE_PARAMS(vec3,   NGLI_PARAM_TYPE_VEC3,  opt.vec,  uniformvec_update_func);
DECLARE_PARAMS(vec4,   NGLI_PARAM_TYPE_VEC4,  opt.vec,  uniformvec_update_func);
DECLARE_PARAMS(int,    NGLI_PARAM_TYPE_I32,   opt.ivec, uniformivec_update_func);
DECLARE_PARAMS(ivec2,  NGLI_PARAM_TYPE_IVEC2, opt.ivec, uniformivec_update_func);
DECLARE_PARAMS(ivec3,  NGLI_PARAM_TYPE_IVEC3, opt.ivec, uniformivec_update_func);
DECLARE_PARAMS(ivec4,  NGLI_PARAM_TYPE_IVEC4, opt.ivec, uniformivec_update_func);
DECLARE_PARAMS(uint,   NGLI_PARAM_TYPE_U32,   opt.uvec, uniformuivec_update_func);
DECLARE_PARAMS(uivec2, NGLI_PARAM_TYPE_UVEC2, opt.uvec, uniformuivec_update_func);
DECLARE_PARAMS(uivec3, NGLI_PARAM_TYPE_UVEC3, opt.uvec, uniformuivec_update_func);
DECLARE_PARAMS(uivec4, NGLI_PARAM_TYPE_UVEC4, opt.uvec, uniformuivec_update_func);

static const struct node_param uniformquat_params[] = {
    {"value",  NGLI_PARAM_TYPE_VEC4, OFFSET(opt.vec), {.vec=NGLI_QUAT_IDENTITY},
               .flags=NGLI_PARAM_FLAG_ALLOW_LIVE_CHANGE,
               .update_func=uniformquat_update_func,
               .desc=NGLI_DOCSTRING("value exposed to the shader")},
    {"as_mat4", NGLI_PARAM_TYPE_BOOL, OFFSET(as_mat4), {.i64=0},
                .desc=NGLI_DOCSTRING("exposed as a 4x4 rotation matrix in the program")},
    {NULL}
};

static const struct node_param uniformmat4_params[] = {
    {"value",     NGLI_PARAM_TYPE_MAT4, OFFSET(opt.mat), {.mat=NGLI_MAT4_IDENTITY},
                  .flags=NGLI_PARAM_FLAG_ALLOW_LIVE_CHANGE,
                  .update_func=uniformmat4_update_func,
                  .desc=NGLI_DOCSTRING("value exposed to the shader")},
    {"transform", NGLI_PARAM_TYPE_NODE, OFFSET(transform), .node_types=TRANSFORM_TYPES_LIST,
                  .desc=NGLI_DOCSTRING("`value` transformation chain")},
    {NULL}
};

#define uniformbool_update   NULL
#define uniformfloat_update  NULL
#define uniformvec2_update   NULL
#define uniformvec3_update   NULL
#define uniformvec4_update   NULL
#define uniformint_update    NULL
#define uniformivec2_update  NULL
#define uniformivec3_update  NULL
#define uniformivec4_update  NULL
#define uniformuint_update   NULL
#define uniformuivec2_update NULL
#define uniformuivec3_update NULL
#define uniformuivec4_update NULL

static int uniformquat_update(struct ngl_node *node, double t)
{
    struct variable_priv *s = node->priv_data;
    if (s->as_mat4)
        ngli_mat4_rotate_from_quat(s->matrix, s->vector);
    return 0;
}

static int uniformmat4_update(struct ngl_node *node, double t)
{
    struct variable_priv *s = node->priv_data;
    if (s->transform) {
        int ret = ngli_node_update(s->transform, t);
        if (ret < 0)
            return ret;
        ngli_transform_chain_compute(s->transform, s->matrix);
    }
    return 0;
}

#define DECLARE_INIT_FUNC(type, dtype, count, dst, src)     \
static int uniform##type##_init(struct ngl_node *node)      \
{                                                           \
    struct variable_priv *s = node->priv_data;              \
    s->data = dst;                                          \
    s->data_size = count * sizeof(*dst);                    \
    s->data_type = dtype;                                   \
    memcpy(s->data, src, s->data_size);                     \
    return 0;                                               \
}

DECLARE_INIT_FUNC(bool,   NGLI_TYPE_BOOL,   1, s->ivector, s->opt.ivec)
DECLARE_INIT_FUNC(int,    NGLI_TYPE_INT,    1, s->ivector, s->opt.ivec)
DECLARE_INIT_FUNC(ivec2,  NGLI_TYPE_IVEC2,  2, s->ivector, s->opt.ivec)
DECLARE_INIT_FUNC(ivec3,  NGLI_TYPE_IVEC3,  3, s->ivector, s->opt.ivec)
DECLARE_INIT_FUNC(ivec4,  NGLI_TYPE_IVEC4,  4, s->ivector, s->opt.ivec)
DECLARE_INIT_FUNC(uint,   NGLI_TYPE_UINT,   1, s->uvector, s->opt.uvec)
DECLARE_INIT_FUNC(uivec2, NGLI_TYPE_UIVEC2, 2, s->uvector, s->opt.uvec)
DECLARE_INIT_FUNC(uivec3, NGLI_TYPE_UIVEC3, 3, s->uvector, s->opt.uvec)
DECLARE_INIT_FUNC(uivec4, NGLI_TYPE_UIVEC4, 4, s->uvector, s->opt.uvec)
DECLARE_INIT_FUNC(float,  NGLI_TYPE_FLOAT,  1, s->vector,  s->opt.vec)
DECLARE_INIT_FUNC(vec2,   NGLI_TYPE_VEC2,   2, s->vector,  s->opt.vec)
DECLARE_INIT_FUNC(vec3,   NGLI_TYPE_VEC3,   3, s->vector,  s->opt.vec)
DECLARE_INIT_FUNC(vec4,   NGLI_TYPE_VEC4,   4, s->vector,  s->opt.vec)

static int uniformquat_init(struct ngl_node *node)
{
    struct variable_priv *s = node->priv_data;
    s->data = s->vector;
    s->data_size = 4 * sizeof(*s->vector);
    s->data_type = NGLI_TYPE_VEC4;
    memcpy(s->data, s->opt.vec, s->data_size);
    if (s->as_mat4) {
        s->data = s->matrix;
        s->data_size = sizeof(s->matrix);
        s->data_type = NGLI_TYPE_MAT4;
        ngli_mat4_rotate_from_quat(s->matrix, s->vector);
    }
    return 0;
}

static int uniformmat4_init(struct ngl_node *node)
{
    struct variable_priv *s = node->priv_data;

    int ret = ngli_transform_chain_check(s->transform);
    if (ret < 0)
        return ret;

    s->data = s->matrix;
    s->data_size = sizeof(s->matrix);
    s->data_type = NGLI_TYPE_MAT4;
    /* Note: we assume here that a transformation chain includes at least one
     * dynamic transform. We could crawl the chain to figure it out in the
     * details, but that would be limited since we would have to also detect
     * live changes in any of the transform node at update as well. That extra
     * complexity is probably not worth just for handling the case of a static
     * transformation list. */
    s->dynamic = !!s->transform;
    memcpy(s->data, s->opt.mat, s->data_size);
    return 0;
}

#define DEFINE_UNIFORM_CLASS(class_id, class_name, type)        \
const struct node_class ngli_uniform##type##_class = {          \
    .id        = class_id,                                      \
    .category  = NGLI_NODE_CATEGORY_UNIFORM,                    \
    .name      = class_name,                                    \
    .init      = uniform##type##_init,                          \
    .update    = uniform##type##_update,                        \
    .priv_size = sizeof(struct variable_priv),                  \
    .params    = uniform##type##_params,                        \
    .file      = __FILE__,                                      \
};

DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMBOOL,   "UniformBool",   bool)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMFLOAT,  "UniformFloat",  float)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMVEC2,   "UniformVec2",   vec2)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMVEC3,   "UniformVec3",   vec3)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMVEC4,   "UniformVec4",   vec4)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMQUAT,   "UniformQuat",   quat)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMINT,    "UniformInt",    int)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMIVEC2,  "UniformIVec2",  ivec2)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMIVEC3,  "UniformIVec3",  ivec3)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMIVEC4,  "UniformIVec4",  ivec4)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMUINT,   "UniformUInt",   uint)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMUIVEC2, "UniformUIVec2", uivec2)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMUIVEC3, "UniformUIVec3", uivec3)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMUIVEC4, "UniformUIVec4", uivec4)
DEFINE_UNIFORM_CLASS(NGL_NODE_UNIFORMMAT4,   "UniformMat4",   mat4)
