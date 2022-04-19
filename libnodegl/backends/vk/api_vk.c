#include "internal.h"

const struct api_impl api_vk = {
    .configure          = ngli_ctx_configure,
    .resize             = ngli_ctx_resize,
    .set_capture_buffer = ngli_ctx_set_capture_buffer,
    .set_scene          = ngli_ctx_set_scene,
    .prepare_draw       = ngli_ctx_prepare_draw,
    .draw               = ngli_ctx_draw,
    .reset              = ngli_ctx_reset,
};
