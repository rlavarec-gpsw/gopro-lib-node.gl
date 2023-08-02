#import "utils_darwin_vk.h"

#import <vulkan/vulkan.h>

#if defined(TARGET_DARWIN)
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#endif

#import <MoltenVK/vk_mvk_moltenvk.h>

#import <Metal/Metal.h>
#ifdef TARGET_DARWIN
#import <Cocoa/Cocoa.h>
#else
#import <UIKit/UIKit.h>
#endif
#import <Foundation/Foundation.h>
#import <QuartzCore/QuartzCore.h>
#import <QuartzCore/CAMetalLayer.h>
#import <CoreVideo/CoreVideo.h>

#import "log.h"
#import "texture.h"

#import "format_vk.h"
#import "gpu_ctx_vk.h"
#import "texture_vk.h"
#import "vkutils.h"

VkResult wrap_into_texture(struct gpu_ctx *s, const void *buffer, struct texture *t)
{
    struct gpu_ctx_vk *ctx = (struct gpu_ctx_vk *)s;
    struct vkcontext *vk = ctx->vkcontext;
    struct texture_vk *tex_vk = (struct texture_vk *)t;

    if (ctx->cvbuffer) {
        CFRelease((CVPixelBufferRef)ctx->cvbuffer);
        ctx->cvbuffer = NULL;
    }

    CVPixelBufferRef buffer_ref = (CVPixelBufferRef)buffer;
    const size_t width = CVPixelBufferGetWidth(buffer_ref);
    const size_t height = CVPixelBufferGetHeight(buffer_ref);
    ctx->cvbuffer = (void *)CFRetain(buffer_ref);

    VkFormat vk_format = VK_FORMAT_B8G8R8A8_UNORM;
    MTLPixelFormat mtl_format = MTLPixelFormatBGRA8Unorm;
    OSType format_type = CVPixelBufferGetPixelFormatType(buffer_ref);
    if (format_type == kCVPixelFormatType_32RGBA) {
        vk_format = VK_FORMAT_R8G8B8A8_UNORM;
        mtl_format = MTLPixelFormatRGBA8Unorm;
    } else if (format_type != kCVPixelFormatType_32BGRA) {
        LOG(ERROR, "Unrecognized corevideo format");
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }

    id<MTLDevice> device = (id<MTLDevice>)ctx->metal_device;
    if (!device) {
#if defined(TARGET_DARWIN)
        if (!vk->get_mtl_device_mvk_fn) {
            VK_LOAD_FUNC(vk->instance, GetMTLDeviceMVK);
            if (!GetMTLDeviceMVK) {
                LOG(ERROR, "could not find vkGetMTLDeviceMVK function");
                return VK_ERROR_UNKNOWN;
            }
            vk->get_mtl_device_mvk_fn = GetMTLDeviceMVK;
        }
        ((PFN_vkGetMTLDeviceMVK)vk->get_mtl_device_mvk_fn)(vk->phy_device, &device);
#else
        vkGetMTLDeviceMVK(vk->phy_device, &device);
#endif
        [device retain];
    }
    ctx->metal_device = device;

    CVMetalTextureCacheRef texture_cache = (CVMetalTextureCacheRef)ctx->texture_cache;
    if (!ctx->texture_cache) {
        CVReturn status = CVMetalTextureCacheCreate(NULL, NULL, (id<MTLDevice>)ctx->metal_device,
                                                    NULL, &texture_cache);
        if (status != kCVReturnSuccess) {
            LOG(ERROR, "could not create metal texture cache for corevideo: %d", status);
            return VK_ERROR_UNKNOWN;
        }
    }
    ctx->texture_cache = texture_cache;

    CVMetalTextureRef texture_ref = NULL;
    CVReturn status = CVMetalTextureCacheCreateTextureFromImage(NULL, 
                          texture_cache, buffer_ref, NULL, mtl_format,
                          width, height, 0, &texture_ref);
    if (status != kCVReturnSuccess) {
        LOG(ERROR, "could not create texture from buffer for corevideo: %d", status);
        return VK_ERROR_UNKNOWN;
    }

    const VkImageCreateInfo image_create_info = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .extent        = {width, height, 1},
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .format        = vk_format,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkResult res = vkCreateImage(vk->device, &image_create_info, NULL, &tex_vk->image);
    if (res != VK_SUCCESS) {
        LOG(ERROR, "could not create image for corevideo: %s", ngli_vk_res2str(res));
        CFRelease(texture_ref);
        return res;
    }

    const struct texture_params texture_params = {
        .type       = NGLI_TEXTURE_TYPE_2D,
        .format     = ngli_format_vk_to_ngl(vk_format),
        .width      = width,
        .height      = height,
        .min_filter = NGLI_FILTER_NEAREST,
        .mag_filter = NGLI_FILTER_NEAREST,
        .wrap_s     = NGLI_WRAP_CLAMP_TO_EDGE,
        .wrap_t     = NGLI_WRAP_CLAMP_TO_EDGE,
        .usage      = NGLI_TEXTURE_USAGE_TRANSFER_DST_BIT,
    };

    const struct texture_vk_wrap_params wrap_params = {
        .params       = &texture_params,
        .image        = tex_vk->image,
        .image_layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    res = ngli_texture_vk_wrap(t, &wrap_params, 1);
    if (res != VK_SUCCESS) {
        LOG(ERROR, "could not wrap texture for corevideo: %s", ngli_vk_res2str(res));
        CFRelease(texture_ref);
        return res;
    }

    id<MTLTexture> metal_texture = CVMetalTextureGetTexture(texture_ref);
#if defined(TARGET_DARWIN)
    if (!vk->set_mtl_texture_mvk_fn) {
        VK_LOAD_FUNC(vk->instance, SetMTLTextureMVK);
        if (!SetMTLTextureMVK) {
            LOG(ERROR, "could not find function vkSetMTLTextureMVK");
            CFRelease(texture_ref);
            return VK_ERROR_UNKNOWN;
        }
        vk->set_mtl_texture_mvk_fn = SetMTLTextureMVK;
    }

    res = ((PFN_vkSetMTLTextureMVK)vk->set_mtl_texture_mvk_fn)(tex_vk->image, metal_texture);
#else
    res = vkSetMTLTextureMVK(tex_vk->image, metal_texture);
#endif

    if (res != VK_SUCCESS) {
        LOG(ERROR, "could not set metal texure: %s", ngli_vk_res2str(res));
        CFRelease(texture_ref);
        return res;
    }

    CVMetalTextureCacheFlush(texture_cache, 0);
    CFRelease(texture_ref);
    return VK_SUCCESS;
}

void gpu_ctx_vk_cleanup(struct gpu_ctx *s)
{
    struct gpu_ctx_vk *ctx = (struct gpu_ctx_vk *)s;
    if (!ctx) {
        return;
    }

    if (ctx->metal_device) {
        id<MTLDevice> device = (id<MTLDevice>)ctx->metal_device;
        [device release];
        ctx->metal_device = 0;
    }

    if (ctx->texture_cache) {
        CFRelease(ctx->texture_cache);
        ctx->texture_cache = NULL;
    }

    if (ctx->cvbuffer) {
        CFRelease((CVPixelBufferRef)ctx->cvbuffer);
        ctx->cvbuffer = NULL;
    }
}

int gpu_ctx_vk_init_layer(struct gpu_ctx *s) {
    if (!s || s->config.offscreen) {
        return 0;
    }

    if (!s->config.window) {
        LOG(ERROR, "invalid window");
        return NGL_ERROR_INVALID_ARG;
    }

    NSObject *object = (NSObject *)s->config.window;
#ifdef TARGET_DARWIN
    NSView *view = nil;
    if ([object isKindOfClass:[NSView class]]) {
        view = (NSView *)object;
    } else if ([object isKindOfClass:[NSWindow class]]) {
        NSWindow *w = (NSWindow *)object;
        view = [w contentView];
    } else if ([object isKindOfClass:[CAMetalLayer class]]) {
        return 0;
    }

    if (view == nil) {
        LOG(ERROR, "window must be either NSView or NSWindow");
        return NGL_ERROR_INVALID_ARG;
    }

    CALayer *view_layer = view.layer;
    if (view_layer && [view_layer isKindOfClass:[CAMetalLayer class]]) {
        return 0;
    }

    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    CAMetalLayer *metal_layer = [CAMetalLayer layer];
    metal_layer.device = device;
    view.layer = metal_layer;

    NSSize size = view.bounds.size;
#else
    if (![object isKindOfClass:[UIView class]]) {
        LOG(ERROR, "view is not a UIView class");
        return NGL_ERROR_INVALID_ARG;
    }

    UIView *view = (UIView *)object;
    CALayer *layer = view.layer;
    if (![layer isKindOfClass:[CAMetalLayer class]]) {
        LOG(ERROR, "layer is not a CAMetalLayer class");
        return NGL_ERROR_INVALID_ARG;
    }

    CAMetalLayer *metal_layer = (CAMetalLayer *)layer;
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    metal_layer.device = device;

    CGSize size = view.bounds.size;
#endif
    CGSize layer_size = CGSizeMake(s->config.width, s->config.height);
    if (layer_size.width == 0) {
        if (size.width == 0) {
            layer_size.width = 1;
        } else {
            layer_size.width = size.width;
        }
    }
        
    if (layer_size.height == 0) {
        if (size.height == 0) {
            layer_size.height = 1;
        } else {
            layer_size.height = size.height;
        }
    }

    metal_layer.drawableSize = layer_size;

    s->config.width = layer_size.width;
    s->config.height = layer_size.height;
    return 0;
}

