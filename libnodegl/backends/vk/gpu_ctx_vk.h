/*
 * Copyright 2020 GoPro Inc.
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

#ifndef GCTX_VK_H
#define GCTX_VK_H

#include "gpu_ctx.h"
#include "vkcontext.h"
#include "cmd_vk.h"

struct gpu_ctx_vk {
    struct gpu_ctx parent;
    struct vkcontext *vkcontext;

    VkCommandPool cmd_pool;
    VkCommandPool transient_cmd_pool;
    VkFence transient_cmd_fence;

    struct cmd_vk **cmds;
    struct cmd_vk **update_cmds;
    struct cmd_vk *transient_cmd;
    struct cmd_vk *cur_cmd;

    VkQueryPool query_pool;

    VkSurfaceCapabilitiesKHR surface_caps;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkSwapchainKHR swapchain;
    int recreate_swapchain;
    VkImage *images;
    uint32_t nb_images;
    uint32_t cur_image_index;

    int width;
    int height;

    int nb_in_flight_frames;
    int cur_frame_index;

    struct darray colors;
    struct darray ms_colors;
    struct darray depth_stencils;
    struct darray rts;
    struct darray rts_load;
    struct buffer *capture_buffer;
    int staging_buffer_size;
    void *mapped_data;

    struct rendertarget *default_rt;
    struct rendertarget *default_rt_load;
    struct rendertarget_desc default_rt_desc;

    struct texture *dummy_texture;

    VkSemaphore *image_avail_sems;
    VkSemaphore *update_finished_sems;
    VkSemaphore *render_finished_sems;

    struct rendertarget *current_rt;
    int viewport[4];
    int scissor[4];
    float clear_color[4];
};

#endif
