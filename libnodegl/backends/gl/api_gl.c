#include "config.h"

#include "backends/gl/gpu_ctx_gl.h"
#include "internal.h"

#define MAKE_CURRENT &(int[]){1}
#define DONE_CURRENT &(int[]){0}

static int cmd_make_current(struct ngl_ctx *s, void *arg)
{
    const int current = *(int *)arg;
    struct gpu_ctx_gl *gpu_ctx_gl = (struct gpu_ctx_gl *)s->gpu_ctx;
    return ngli_glcontext_make_current(gpu_ctx_gl->glcontext, current);
}

static int cmd_configure(struct ngl_ctx *s, void *arg)
{
    const struct ngl_config *config = arg;
    return ngli_ctx_configure(s, config);
}

static int gl_configure(struct ngl_ctx *s, const struct ngl_config *config)
{
    if (config->platform == NGL_PLATFORM_MACOS ||
        config->platform == NGL_PLATFORM_IOS) {
        int ret = ngli_ctx_configure(s, config);
        if (ret < 0)
            return ret;

        struct gpu_ctx_gl *gpu_ctx_gl = (struct gpu_ctx_gl *)s->gpu_ctx;
        ngli_glcontext_make_current(gpu_ctx_gl->glcontext, 0);

        return ngli_ctx_dispatch_cmd(s, cmd_make_current, MAKE_CURRENT);
    }

    return ngli_ctx_dispatch_cmd(s, cmd_configure, (void *)config);
}

struct resize_params {
    int width;
    int height;
    const int *viewport;
};

static int cmd_resize(struct ngl_ctx *s, void *arg)
{
    const struct resize_params *params = arg;
    return ngli_ctx_resize(s, params->width, params->height, params->viewport);
}

static int gl_resize(struct ngl_ctx *s, int width, int height, const int *viewport)
{
    const struct ngl_config *config = &s->config;
    if (config->platform == NGL_PLATFORM_MACOS ||
        config->platform == NGL_PLATFORM_IOS) {
        int ret = ngli_ctx_dispatch_cmd(s, cmd_make_current, DONE_CURRENT);
        if (ret < 0)
            return ret;

        cmd_make_current(s, MAKE_CURRENT);
        ret = ngli_ctx_resize(s, width, height, viewport);
        if (ret < 0)
            return ret;
        cmd_make_current(s, DONE_CURRENT);

        return ngli_ctx_dispatch_cmd(s, cmd_make_current, MAKE_CURRENT);
    }

    struct resize_params params = {
        .width = width,
        .height = height,
        .viewport = viewport,
    };
    return ngli_ctx_dispatch_cmd(s, cmd_resize, &params);
}

static int cmd_set_capture_buffer(struct ngl_ctx *s, void *capture_buffer)
{
    return ngli_ctx_set_capture_buffer(s, capture_buffer);
}

static int gl_set_capture_buffer(struct ngl_ctx *s, void *capture_buffer)
{
    return ngli_ctx_dispatch_cmd(s, cmd_set_capture_buffer, capture_buffer);
}

static int cmd_set_scene(struct ngl_ctx *s, void *arg)
{
    ngli_gpu_ctx_wait_idle(s->gpu_ctx);
    return ngli_ctx_set_scene(s, arg);
}

static int gl_set_scene(struct ngl_ctx *s, struct ngl_node *node)
{
    return ngli_ctx_dispatch_cmd(s, cmd_set_scene, node);
}

static int cmd_prepare_draw(struct ngl_ctx *s, void *arg)
{
    const double t = *(double *)arg;
    return ngli_ctx_prepare_draw(s, t);
}

static int gl_prepare_draw(struct ngl_ctx *s, double t)
{
    return ngli_ctx_dispatch_cmd(s, cmd_prepare_draw, &t);
}

static int cmd_draw(struct ngl_ctx *s, void *arg)
{
    const double t = *(double *)arg;
    return ngli_ctx_draw(s, t);
}

static int gl_draw(struct ngl_ctx *s, double t)
{
    return ngli_ctx_dispatch_cmd(s, cmd_draw, &t);
}

static int cmd_reset(struct ngl_ctx *s, void *arg)
{
    const int action = *(int *)arg;
    ngli_ctx_reset(s, action);
    return 0;
}

static void gl_reset(struct ngl_ctx *s, int action)
{
    ngli_ctx_dispatch_cmd(s, cmd_reset, &action);
}

const struct api_impl api_gl = {
    .configure          = gl_configure,
    .resize             = gl_resize,
    .set_capture_buffer = gl_set_capture_buffer,
    .set_scene          = gl_set_scene,
    .prepare_draw       = gl_prepare_draw,
    .draw               = gl_draw,
    .reset              = gl_reset,
};
