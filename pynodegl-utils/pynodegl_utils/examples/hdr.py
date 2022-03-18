import pynodegl as ngl
from pynodegl_utils.misc import scene

_TONEMAP_OPERATORS = ('none', 'bt2390', 'reinhard', 'reinhard_extended', 'reinhard_jodie', 'aces', 'bt2446')
_DESAT_METHODS = ('method1', 'method2')

@scene(
    rotate=scene.Bool(),
    debug_logavg=scene.Bool(),
    honor_logavg=scene.Bool(),
    tonemap=scene.List(choices=_TONEMAP_OPERATORS),
    desat=scene.List(choices=_DESAT_METHODS)
)
def hdr_to_sdr(cfg, rotate=True, debug_logavg=True, honor_logavg=True, tonemap='bt2390', desat=_DESAT_METHODS[0]):
    logavg_tex = ngl.Texture2D(format="r32g32b32a32_sfloat", width=1024, height=1024, min_filter="linear", mipmap_filter="linear")
    m0 = cfg.medias[0]
    cfg.duration = m0.duration
    cfg.aspect_ratio = (m0.width, m0.height)

    q = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    m = ngl.Media(m0.filename)
    t = ngl.Texture2D(data_src=m)
    p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('hdr2sdr'))
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    media_render = ngl.Render(q, p, label='Media Render')
    media_render.update_frag_resources(
        tex0=t,
        logavg_tex=logavg_tex,
        tonemap=ngl.UniformInt(_TONEMAP_OPERATORS.index(tonemap)),
        desat=ngl.UniformInt(_DESAT_METHODS.index(desat)),
        honor_logavg=ngl.UniformBool(honor_logavg),
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
