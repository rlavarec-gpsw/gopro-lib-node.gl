#ifndef UTILS_DARWIN_VK_H
#define UTILS_DARWIN_VK_H

#include <vulkan/vulkan.h>

#include "gpu_ctx.h"
#include "texture.h"

VkResult wrap_into_texture(struct gpu_ctx *s, const void *buffer, struct texture *t);

void gpu_ctx_vk_cleanup(struct gpu_ctx *s);
int gpu_ctx_vk_init_layer(struct gpu_ctx *s);
#endif
