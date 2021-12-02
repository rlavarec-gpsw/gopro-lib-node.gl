/*
 * Copyright 2017 GoPro Inc.
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

#include <stdlib.h>
#include <string.h>
#include "gpu_ctx_gl.h"
#include "glcontext.h"
#include "glincludes.h"
#include "glstate.h"
#include "graphicstate.h"
#include "internal.h"

static const GLenum gl_blend_factor_map[NGLI_BLEND_FACTOR_NB] = {
    [NGLI_BLEND_FACTOR_ZERO]                = GL_ZERO,
    [NGLI_BLEND_FACTOR_ONE]                 = GL_ONE,
    [NGLI_BLEND_FACTOR_SRC_COLOR]           = GL_SRC_COLOR,
    [NGLI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR] = GL_ONE_MINUS_SRC_COLOR,
    [NGLI_BLEND_FACTOR_DST_COLOR]           = GL_DST_COLOR,
    [NGLI_BLEND_FACTOR_ONE_MINUS_DST_COLOR] = GL_ONE_MINUS_DST_COLOR,
    [NGLI_BLEND_FACTOR_SRC_ALPHA]           = GL_SRC_ALPHA,
    [NGLI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA] = GL_ONE_MINUS_SRC_ALPHA,
    [NGLI_BLEND_FACTOR_DST_ALPHA]           = GL_DST_ALPHA,
    [NGLI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA] = GL_ONE_MINUS_DST_ALPHA,
};

static GLenum get_gl_blend_factor(int blend_factor)
{
    return gl_blend_factor_map[blend_factor];
}

static const GLenum gl_blend_op_map[NGLI_BLEND_OP_NB] = {
    [NGLI_BLEND_OP_ADD]              = GL_FUNC_ADD,
    [NGLI_BLEND_OP_SUBTRACT]         = GL_FUNC_SUBTRACT,
    [NGLI_BLEND_OP_REVERSE_SUBTRACT] = GL_FUNC_REVERSE_SUBTRACT,
    [NGLI_BLEND_OP_MIN]              = GL_MIN,
    [NGLI_BLEND_OP_MAX]              = GL_MAX,
};

static GLenum get_gl_blend_op(int blend_op)
{
    return gl_blend_op_map[blend_op];
}

static const GLenum gl_compare_op_map[NGLI_COMPARE_OP_NB] = {
    [NGLI_COMPARE_OP_NEVER]            = GL_NEVER,
    [NGLI_COMPARE_OP_LESS]             = GL_LESS,
    [NGLI_COMPARE_OP_EQUAL]            = GL_EQUAL,
    [NGLI_COMPARE_OP_LESS_OR_EQUAL]    = GL_LEQUAL,
    [NGLI_COMPARE_OP_GREATER]          = GL_GREATER,
    [NGLI_COMPARE_OP_NOT_EQUAL]        = GL_NOTEQUAL,
    [NGLI_COMPARE_OP_GREATER_OR_EQUAL] = GL_GEQUAL,
    [NGLI_COMPARE_OP_ALWAYS]           = GL_ALWAYS,
};

static GLenum get_gl_compare_op(int compare_op)
{
    return gl_compare_op_map[compare_op];
}

static const GLenum gl_stencil_op_map[NGLI_STENCIL_OP_NB] = {
    [NGLI_STENCIL_OP_KEEP]                = GL_KEEP,
    [NGLI_STENCIL_OP_ZERO]                = GL_ZERO,
    [NGLI_STENCIL_OP_REPLACE]             = GL_REPLACE,
    [NGLI_STENCIL_OP_INCREMENT_AND_CLAMP] = GL_INCR,
    [NGLI_STENCIL_OP_DECREMENT_AND_CLAMP] = GL_DECR,
    [NGLI_STENCIL_OP_INVERT]              = GL_INVERT,
    [NGLI_STENCIL_OP_INCREMENT_AND_WRAP]  = GL_INCR_WRAP,
    [NGLI_STENCIL_OP_DECREMENT_AND_WRAP]  = GL_DECR_WRAP,
};

static GLenum get_gl_stencil_op(int stencil_op)
{
    return gl_stencil_op_map[stencil_op];
}

static const GLenum gl_cull_mode_map[NGLI_CULL_MODE_NB] = {
    [NGLI_CULL_MODE_NONE]           = GL_BACK,
    [NGLI_CULL_MODE_FRONT_BIT]      = GL_FRONT,
    [NGLI_CULL_MODE_BACK_BIT]       = GL_BACK,
};

static GLenum get_gl_cull_mode(int cull_mode)
{
    return gl_cull_mode_map[cull_mode];
}

void ngli_glstate_reset(const struct glcontext *gl, struct glstate *glstate)
{
    memset(glstate, 0, sizeof(*glstate));

    /* Blending */
    ngli_glDisable(gl, GL_BLEND);
    glstate->blend = 0;

    ngli_glBlendFuncSeparate(gl, GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glstate->blend_src_factor = GL_ONE;
    glstate->blend_dst_factor = GL_ZERO;
    glstate->blend_src_factor_a = GL_ONE;
    glstate->blend_dst_factor_a = GL_ZERO;

    ngli_glBlendEquationSeparate(gl, GL_FUNC_ADD, GL_FUNC_ADD);
    glstate->blend_op = GL_FUNC_ADD;
    glstate->blend_op_a = GL_FUNC_ADD;

    /* Color write mask */
    ngli_glColorMask(gl, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glstate->color_write_mask[0] = GL_TRUE;
    glstate->color_write_mask[1] = GL_TRUE;
    glstate->color_write_mask[2] = GL_TRUE;
    glstate->color_write_mask[3] = GL_TRUE;

    /* Depth */
    ngli_glDisable(gl, GL_DEPTH_TEST);
    glstate->depth_test = 0;

    ngli_glDepthMask(gl, GL_TRUE);
    glstate->depth_write_mask = GL_TRUE;

    ngli_glDepthFunc(gl, GL_LESS);
    glstate->depth_func = GL_LESS;

    /* Stencil */
    ngli_glDisable(gl, GL_STENCIL_TEST);
    glstate->stencil_test = 0;

    ngli_glStencilMask(gl, GL_TRUE);
    glstate->stencil_write_mask = GL_TRUE;

    ngli_glStencilFunc(gl, GL_ALWAYS, 0, 1);
    glstate->stencil_func = GL_ALWAYS;
    glstate->stencil_ref = 0;
    glstate->stencil_read_mask = 1;

    ngli_glStencilOp(gl, GL_KEEP, GL_KEEP, GL_KEEP);
    glstate->stencil_fail = GL_KEEP;
    glstate->stencil_depth_fail = GL_KEEP;
    glstate->stencil_depth_pass = GL_KEEP;

    /* Face culling */
    ngli_glDisable(gl, GL_CULL_FACE);
    glstate->cull_face = 0;

    ngli_glCullFace(gl, GL_BACK);
    glstate->cull_face_mode = GL_BACK;

    /* Scissor */
    ngli_glDisable(gl, GL_SCISSOR_TEST);
    glstate->scissor_test = 0;

    /* Program */
    ngli_glUseProgram(gl, 0);
    glstate->program_id = 0;

    /* VAO */
    if (gl->features & NGLI_FEATURE_VERTEX_ARRAY_OBJECT)
        ngli_glBindVertexArray(gl, 0);
}

static void init_state(struct glstate *s, const struct graphicstate *gc)
{
    s->blend              = gc->blend;
    s->blend_dst_factor   = get_gl_blend_factor(gc->blend_dst_factor);
    s->blend_src_factor   = get_gl_blend_factor(gc->blend_src_factor);
    s->blend_dst_factor_a = get_gl_blend_factor(gc->blend_dst_factor_a);
    s->blend_src_factor_a = get_gl_blend_factor(gc->blend_src_factor_a);
    s->blend_op           = get_gl_blend_op(gc->blend_op);
    s->blend_op_a         = get_gl_blend_op(gc->blend_op_a);

    for (int i = 0; i < 4; i++)
        s->color_write_mask[i] = gc->color_write_mask >> i & 1;

    s->depth_test         = gc->depth_test;
    s->depth_write_mask   = gc->depth_write_mask;
    s->depth_func         = get_gl_compare_op(gc->depth_func);

    s->stencil_test       = gc->stencil_test;
    s->stencil_write_mask = gc->stencil_write_mask;
    s->stencil_func       = get_gl_compare_op(gc->stencil_func);
    s->stencil_ref        = gc->stencil_ref;
    s->stencil_read_mask  = gc->stencil_read_mask;
    s->stencil_fail       = get_gl_stencil_op(gc->stencil_fail);
    s->stencil_depth_fail = get_gl_stencil_op(gc->stencil_depth_fail);
    s->stencil_depth_pass = get_gl_stencil_op(gc->stencil_depth_pass);

    s->cull_face      = gc->cull_mode != NGLI_CULL_MODE_NONE;
    s->cull_face_mode = get_gl_cull_mode(gc->cull_mode);

    s->scissor_test = gc->scissor_test;
}

static int honor_state(const struct glcontext *gl,
                       const struct glstate *next,
                       const struct glstate *prev)
{
    if (!memcmp(prev, next, sizeof(*prev)))
        return 0;

    /* Blend */
    if (next->blend != prev->blend) {
        if (next->blend)
            ngli_glEnable(gl, GL_BLEND);
        else
            ngli_glDisable(gl, GL_BLEND);
    }

    if (next->blend_dst_factor   != prev->blend_dst_factor   ||
        next->blend_src_factor   != prev->blend_src_factor   ||
        next->blend_dst_factor_a != prev->blend_dst_factor_a ||
        next->blend_src_factor_a != prev->blend_src_factor_a) {
        ngli_glBlendFuncSeparate(gl,
                                 next->blend_src_factor,
                                 next->blend_dst_factor,
                                 next->blend_src_factor_a,
                                 next->blend_dst_factor_a);
    }

    if (next->blend_op   != prev->blend_op ||
        next->blend_op_a != prev->blend_op_a) {
        ngli_glBlendEquationSeparate(gl,
                                     next->blend_op,
                                     next->blend_op_a);
    }

    /* Color */
    if (memcmp(next->color_write_mask, prev->color_write_mask, sizeof(prev->color_write_mask))) {
        ngli_glColorMask(gl,
                         next->color_write_mask[0],
                         next->color_write_mask[1],
                         next->color_write_mask[2],
                         next->color_write_mask[3]);
    }

    /* Depth */
    if (next->depth_test != prev->depth_test) {
        if (next->depth_test)
            ngli_glEnable(gl, GL_DEPTH_TEST);
        else
            ngli_glDisable(gl, GL_DEPTH_TEST);
    }

    if (next->depth_write_mask != prev->depth_write_mask) {
        ngli_glDepthMask(gl, next->depth_write_mask);
    }

    if (next->depth_func != prev->depth_func) {
        ngli_glDepthFunc(gl, next->depth_func);
    }

    /* Stencil */
    if (next->stencil_test != prev->stencil_test) {
        if (next->stencil_test)
            ngli_glEnable(gl, GL_STENCIL_TEST);
        else
            ngli_glDisable(gl, GL_STENCIL_TEST);
    }

    if (next->stencil_write_mask != prev->stencil_write_mask) {
        ngli_glStencilMask(gl, next->stencil_write_mask);
    }

    if (next->stencil_func      != prev->stencil_func ||
        next->stencil_ref       != prev->stencil_ref  ||
        next->stencil_read_mask != prev->stencil_read_mask) {
        ngli_glStencilFunc(gl,
                           next->stencil_func,
                           next->stencil_ref,
                           next->stencil_read_mask);
    }

    if (next->stencil_fail       != prev->stencil_fail       ||
        next->stencil_depth_fail != prev->stencil_depth_fail ||
        next->stencil_depth_pass != prev->stencil_depth_pass) {
        ngli_glStencilOp(gl,
                         next->stencil_fail,
                         next->stencil_depth_fail,
                         next->stencil_depth_pass);
    }

    /* Face Culling */
    if (next->cull_face != prev->cull_face) {
        if (next->cull_face)
            ngli_glEnable(gl, GL_CULL_FACE);
        else
            ngli_glDisable(gl, GL_CULL_FACE);
    }

    if (next->cull_face_mode != prev->cull_face_mode) {
        ngli_glCullFace(gl, next->cull_face_mode);
    }

    /* Scissor */
    if (next->scissor_test != prev->scissor_test) {
        if (next->scissor_test)
            ngli_glEnable(gl, GL_SCISSOR_TEST);
        else
            ngli_glDisable(gl, GL_SCISSOR_TEST);
    }

    return 1;
}

void ngli_glstate_update(const struct glcontext *gl, struct glstate *glstate, const struct graphicstate *state)
{
    struct glstate new_glstate = {0};
    init_state(&new_glstate, state);

    int ret = honor_state(gl, &new_glstate, glstate);
    if (ret > 0)
        *glstate = new_glstate;
}

void ngli_glstate_use_program(const struct glcontext *gl, struct glstate *glstate, GLuint program_id)
{
    if (glstate->program_id != program_id) {
        ngli_glUseProgram(gl, program_id);
        glstate->program_id = program_id;
    }
}

void ngli_glstate_update_scissor(const struct glcontext *gl, struct glstate *glstate, const int *scissor)
{
    if (!memcmp(glstate->scissor, scissor, sizeof(glstate->scissor)))
        return;
    memcpy(glstate->scissor, scissor, sizeof(glstate->scissor));
    ngli_glScissor(gl, scissor[0], scissor[1], scissor[2], scissor[3]);
}

void ngli_glstate_update_viewport(const struct glcontext *gl, struct glstate *glstate, const int *viewport)
{
    if (!memcmp(glstate->viewport, viewport, sizeof(glstate->viewport)))
        return;
    memcpy(glstate->viewport, viewport, sizeof(glstate->viewport));
    ngli_glViewport(gl, viewport[0], viewport[1], viewport[2], viewport[3]);
}
