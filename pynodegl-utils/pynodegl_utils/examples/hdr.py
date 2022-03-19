import pynodegl as ngl
from pynodegl_utils.misc import scene

_TONEMAP_OPERATORS = ('none', 'bt2390', 'reinhard', 'reinhard_extended', 'reinhard_jodie', 'aces', 'bt2446')
_TONEMAP_NORM = ('luma', 'max', 'magic')

@scene(
    rotate=scene.Bool(),
    debug_logavg=scene.Bool(),
    honor_logavg=scene.Bool(),
    tonemap=scene.List(choices=_TONEMAP_OPERATORS),
    norm=scene.List(choices=_TONEMAP_NORM),
)
def hdr_to_sdr(cfg, rotate=True, debug_logavg=False, honor_logavg=False, tonemap='bt2446', norm='luma'):
    logavg_tex = ngl.Texture2D(format="r32g32b32a32_sfloat", width=1024, height=1024, min_filter="linear", mipmap_filter="linear")
    m0 = cfg.medias[0]
    cfg.duration = m0.duration
    cfg.aspect_ratio = (m0.width, m0.height)

    q = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    m = ngl.Media(m0.filename)
    t = ngl.Texture2D(data_src=m)
    p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('hdr2sdr-old'))
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    media_render = ngl.Render(q, p, label='Media Render')
    media_render.update_frag_resources(
        tex0=t,
        logavg_tex=logavg_tex,
        tonemap=ngl.UniformInt(_TONEMAP_OPERATORS.index(tonemap)),
        honor_logavg=ngl.UniformBool(honor_logavg),
        norm_method=ngl.UniformInt(_TONEMAP_NORM.index(norm)),
    )
    if rotate:
        media_render = ngl.Rotate(media_render, angle=180)

    p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('hdrlogavg'))
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    hdrlogavg = ngl.Render(q, p, label='Log average')
    hdrlogavg.update_frag_resources(tex0=t)
    rtt = ngl.RenderToTexture(hdrlogavg, color_textures=(logavg_tex,))

    grp = ngl.Group(children=(rtt, media_render))

    if debug_logavg:
        p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('hdrlogavg_dbg'))
        p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
        hdrlogavg_dbg = ngl.Render(q, p, label='Debug Log average')
        hdrlogavg_dbg.update_frag_resources(tex0=logavg_tex)
        if rotate:
            hdrlogavg_dbg = ngl.Rotate(hdrlogavg_dbg, angle=180)
        hdrlogavg_dbg = ngl.Scale(hdrlogavg_dbg, factors=(1/3, 1/3, 1), anchor=(1, 1, 0))
        grp.add_children(hdrlogavg_dbg)

    return grp


@scene(rotate=scene.Bool())
def hdr_to_sdr2(cfg, rotate=True):
    m0 = cfg.medias[0]
    cfg.duration = m0.duration
    cfg.aspect_ratio = (m0.width, m0.height)

    q = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    m = ngl.Media(m0.filename)
    t = ngl.Texture2D(data_src=m)
    p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('hdr2sdr'))
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    media_render = ngl.Render(q, p, label='Media Render')
    media_render.update_frag_resources(tex0=t)
    if rotate:
        media_render = ngl.Rotate(media_render, angle=180)
    return media_render
