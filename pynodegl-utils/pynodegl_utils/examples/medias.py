import pynodegl as ngl
from pynodegl_utils.misc import scene


@scene(uv_corner=scene.Vector(n=2),
       uv_width=scene.Vector(n=2),
       uv_height=scene.Vector(n=2),
       progress_bar=scene.Bool())
def centered_media(cfg, uv_corner=(0, 0), uv_width=(1, 0), uv_height=(0, 1), progress_bar=True):
    '''A simple centered media with an optional progress bar in the shader'''
    m0 = cfg.medias[0]
    cfg.duration = m0.duration
    cfg.aspect_ratio = (m0.width, m0.height)

    q = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0), uv_corner, uv_width, uv_height)
    m = ngl.Media(m0.filename)
    t = ngl.Texture2D(data_src=m)
    p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('texture'))
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    render = ngl.Render(q, p)
    render.update_frag_resources(tex0=t)

    if progress_bar:
        p.set_fragment(cfg.get_frag('progress-bar'))

        media_duration = ngl.UniformFloat(m0.duration)
        ar = ngl.UniformFloat(cfg.aspect_ratio_float)
        render.update_frag_resources(media_duration=media_duration, ar=ar)
    return render


@scene()
def hdr_to_sdr(cfg):
    vert = '''
void main()
{
    ngl_out_pos = ngl_projection_matrix * ngl_modelview_matrix * vec4(ngl_position, 1.0);
    var_uvcoord = ngl_uvcoord;
    var_tex_coord = (tex_coord_matrix * vec4(ngl_uvcoord, 0.0, 1.0)).xy;
}
'''

    frag = '''
const float PQ_M1 = 0.1593017578125;
const float PQ_M2 = 78.84375;
const float PQ_C1 = 0.8359375;
const float PQ_C2 = 18.8515625;
const float PQ_C3 = 18.6875;
const float PQ_C  = 10000.0;

vec4 pq_eotf_inverse(vec4 signal)
{
    const vec4 Y = signal * 1.0 / (10000.0 / 203.0);
    const vec4 Ym = pow(Y, vec4(PQ_M1));
    return pow((PQ_C1 + PQ_C2 * Ym) / (1.0 + PQ_C3 * Ym), vec4(PQ_M2));
}

vec4 pq_eotf(vec4 signal)
{
    return (10000 / 203) * pow(max(pow(signal, vec4(1.0 / PQ_M2)) - vec4(PQ_C1), vec4(0.0)) / (vec4(PQ_C2) - vec4(PQ_C3) * pow(signal, vec4(1.0 / PQ_M2))), vec4(1.0 / PQ_M1));
}

vec3 eetf(vec3 v, float maxLum)
{
    float ks = 1.5 * maxLum - 0.5;
    const vec3 t = (v.rgb - vec3(ks)) / vec3(1.0 - ks);
    const vec3 t2 = t * t;
    const vec3 t3 = t2 * t;
    const vec3 p = (2.0 * t3 - 3.0 * t2 + vec3(1.0)) * vec3(ks) +(t3 - 2.0 * t2 + t) * vec3(1.0 - ks) + (-2.0 * t3 + 3.0 * t2) * vec3(maxLum);
    return mix(p, v, lessThan(v, vec3(ks)));
}

#define HLG 1
#define DESAT 1

void main()
{
    vec4 color = ngl_texvideo(tex, var_tex_coord);

    // hlg eotf (linearize), output range [0, 12]
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = mix(vec3(4.0) * color.rgb * color.rgb, exp((color.rgb - vec3(0.559911)) * vec3(1.0/0.178833)) + vec3(0.284669), lessThan(vec3(0.5), color.rgb));

    // scale hlg eotf output range from [0, 12] against the hlg reference white point (hlg_eotf(0.75)) = 3.179550717
    color.rgb *= (1.0/3.179550717);

    // hlg ootf (scene light -> display light)
    const vec3 luma = vec3(0.2627, 0.6780, 0.0593);
    color.rgb *= vec3(pow(dot(luma, color.rgb), 0.200000));

    float signal_peak = 1000.0/203.0;
    vec3 signal = min(color.rgb, vec3(signal_peak));
    vec3 sig = signal;

#if DESAT == 1
    float l = dot(luma, signal);
    sig = vec3(l);
#elif DESAT == 2
    vec3 sig_orig = sig;
#else
    int sig_idx = 0;
    if (color[1] > color[sig_idx]) sig_idx = 1;
    if (color[2] > color[sig_idx]) sig_idx = 2;
    float sig_max = color[sig_idx];
    float sig_orig = sig[sig_idx];
#endif
    // convert linear light to PQ space
    vec4 sig_pq = vec4(sig.rgb, signal_peak);
    sig_pq = pq_eotf_inverse(sig_pq);

    // normalize pq signal against signal peak
    float scale = 1.0 / sig_pq.a;
    sig_pq.rgb *= vec3(scale);

    // solve EETF
    float maxLum = 0.580690 * scale;
    sig = eetf(sig_pq.rgb, maxLum);

    // invert normalization
    sig *= vec3(sig_pq.a);

    // convert signal from PQ space to linear light
    sig = pq_eotf(vec4(sig, 1.0)).rgb;

#if DESAT == 1
    color.rgb *= sig / l;
#elif DESAT == 2
    color.rgb = color.rgb * (sig / sig_orig);
#else
    vec3 sig_lin = color.rgb * (sig[sig_idx] / sig_orig);
    float coeff = max(sig[sig_idx] - 0.180000, 1e-6) / max(sig[sig_idx], 1.0);
    coeff = 0.9 * pow(coeff, 0.2);
    color.rgb = mix(sig_lin, 1.000000 * sig, coeff);
#endif

    // convert colors from bt2020 to bt709
    const mat3 bt2020_to_bt709 = mat3(
        1.660491  , -0.12455047, -0.01815076,
        -0.58764114,  1.1328999 , -0.1005789 ,
        -0.07284986, -0.00834942,  1.11872966);
    color.rgb = bt2020_to_bt709 * color.rgb;

    // delinearize (gamma 2.2)
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = pow(color.rgb, vec3(1.0/2.2));

    ngl_out_color = color;
}
'''
    m0 = cfg.medias[0]
    cfg.duration = m0.duration
    cfg.aspect_ratio = (m0.width, m0.height)

    q = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    m = ngl.Media(m0.filename)
    t = ngl.Texture2D(data_src=m)
    p = ngl.Program(vertex=vert, fragment=frag)
    p.update_vert_out_vars(var_tex_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    render = ngl.Render(q, p)
    render.update_frag_resources(tex=t)
    return render

@scene(speed=scene.Range(range=[0.01, 2], unit_base=1000))
def playback_speed(cfg, speed=1.0):
    '''Adjust media playback speed using animation keyframes'''
    m0 = cfg.medias[0]
    media_duration = m0.duration
    initial_seek = min(media_duration, 5)
    rush_duration = media_duration - initial_seek
    cfg.duration = rush_duration / speed
    cfg.aspect_ratio = (m0.width, m0.height)

    q = ngl.Quad((-0.5, -0.5, 0), (1, 0, 0), (0, 1, 0))
    time_animkf = [ngl.AnimKeyFrameFloat(0, initial_seek),
                   ngl.AnimKeyFrameFloat(cfg.duration, media_duration)]
    m = ngl.Media(m0.filename, time_anim=ngl.AnimatedTime(time_animkf))
    t = ngl.Texture2D(data_src=m)
    p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('texture'))
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    render = ngl.Render(q, p)
    render.update_frag_resources(tex0=t)
    return render


@scene()
def time_remapping(cfg):
    '''
    Time remapping in the following order:
    - nothing displayed for a while (but media prefetch happening in background)
    - first frame displayed for a while
    - normal playback
    - last frame displayed for a while (even though the media is closed)
    - nothing again until the end
    '''
    m0 = cfg.medias[0]

    media_seek = 10
    noop_duration = 2
    prefetch_duration = 2
    freeze_duration = 3
    playback_duration = 5

    range_start = noop_duration + prefetch_duration
    play_start  = range_start   + freeze_duration
    play_stop   = play_start    + playback_duration
    range_stop  = play_stop     + freeze_duration
    duration    = range_stop    + noop_duration

    cfg.duration = duration
    cfg.aspect_ratio = (m0.width, m0.height)

    media_animkf = [
        ngl.AnimKeyFrameFloat(play_start, media_seek),
        ngl.AnimKeyFrameFloat(play_stop,  media_seek + playback_duration),
    ]

    q = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    m = ngl.Media(m0.filename, time_anim=ngl.AnimatedTime(media_animkf))
    m.set_sxplayer_min_level('verbose')
    t = ngl.Texture2D(data_src=m)
    p = ngl.Program(vertex=cfg.get_vert('texture'), fragment=cfg.get_frag('texture'))
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    r = ngl.Render(q, p)
    r.update_frag_resources(tex0=t)

    time_ranges = [
        ngl.TimeRangeModeNoop(0),
        ngl.TimeRangeModeCont(range_start),
        ngl.TimeRangeModeNoop(range_stop),
    ]
    rf = ngl.TimeRangeFilter(r, ranges=time_ranges, prefetch_time=prefetch_duration)

    base_string = 'media time: {:2g} to {:2g}\nscene time: {:2g} to {:2g}\ntime range: {:2g} to {:2g}'.format(
                  media_seek, media_seek + playback_duration, play_start, play_stop, range_start, range_stop)
    text = ngl.Text(base_string,
                    box_height=(0, 0.3, 0),
                    box_corner=(-1, 1 - 0.3, 0),
                    aspect_ratio=cfg.aspect_ratio,
                    halign='left')

    group = ngl.Group()
    group.add_children(rf, text)

    steps = (
        ('default color, nothing yet', 0, noop_duration),
        ('default color, media prefetched', noop_duration, range_start),
        ('first frame', range_start, play_start),
        ('normal playback', play_start, play_stop),
        ('last frame', play_stop, range_stop),
        ('default color, media released', range_stop, duration),
    )

    for i, (description, start_time, end_time) in enumerate(steps):
        text = ngl.Text(f'{start_time:g} to {end_time:g}: {description}',
                        aspect_ratio=cfg.aspect_ratio,
                        box_height=(0, 0.2, 0))
        text_tr = (
            ngl.TimeRangeModeNoop(0),
            ngl.TimeRangeModeCont(start_time),
            ngl.TimeRangeModeNoop(end_time),
        )
        text_rf = ngl.TimeRangeFilter(text, ranges=text_tr, label='text-step-%d' % i)
        group.add_children(text_rf)

    return ngl.GraphicConfig(group, blend=True,
                             blend_src_factor='src_alpha',
                             blend_dst_factor='one_minus_src_alpha',
                             blend_src_factor_a='zero',
                             blend_dst_factor_a='one')
