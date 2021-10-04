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
    target = ngl.Texture2D(format="r32g32b32a32_sfloat", width=1024, height=1024, min_filter="linear", mipmap_filter="linear")
    vert = '''
void main()
{
    ngl_out_pos = ngl_projection_matrix * ngl_modelview_matrix * vec4(ngl_position, 1.0);
    var_uvcoord = ngl_uvcoord;
    mat4 m = tex_coord_matrix * rotate_matrix;
    /*mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, -1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 1.0, 0.0, 1.0
    );
    */
    var_tex_coord = (m * vec4(ngl_uvcoord, 0.0, 1.0)).xy;
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

vec3 bt2390_eetf(vec3 v, float maxLum)
{
    float ks = 1.5 * maxLum - 0.5;
    const vec3 t = (v.rgb - vec3(ks)) / vec3(1.0 - ks);
    const vec3 t2 = t * t;
    const vec3 t3 = t2 * t;
    const vec3 p = (2.0 * t3 - 3.0 * t2 + vec3(1.0)) * vec3(ks) +(t3 - 2.0 * t2 + t) * vec3(1.0 - ks) + (-2.0 * t3 + 3.0 * t2) * vec3(maxLum);
    return mix(p, v, lessThan(v, vec3(ks)));
}

vec3 saturation(vec3 rgb, float sat)
{
    vec3 luma_weights = vec3(0.2627, 0.6780, 0.0593);
    return mix(vec3(dot(luma_weights, rgb)), rgb, sat);
}

float Tonemap_ACES(float x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

vec4 tonemap_bt2390(vec4 signal) {
    // tone map ITU-R BT.2390
    // convert linear light to PQ space
    vec4 sig_pq = signal;
    sig_pq = pq_eotf_inverse(sig_pq);

    // normalize pq signal against signal peak
    float scale = 1.0 / sig_pq.a;
    sig_pq.rgb *= vec3(scale);

    // solve EETF ITU-R BT.2390
    float maxLum = 0.580690 * scale;
    vec3 sig = bt2390_eetf(sig_pq.rgb, maxLum);

    // invert normalization
    sig *= vec3(sig_pq.a);

    // convert signal from PQ space to linear light
    return pq_eotf(vec4(sig, 1.0));
}

vec4 tonemap_reinhard(vec4 signal) {
    return (signal / (signal + 1.0f));
}

vec4 tonemap_reinhard_extended(vec4 signal) {
    signal.a = 1000/203;
    vec3 numerator = signal.rgb * (1.0f + (signal.rgb / vec3(signal.a * signal.a)));
    return vec4(numerator / (1.0f + signal.rgb), 1.0);
}

vec4 tonemap_reinhard_jodie(vec4 signal, float luma) {
    vec4 tv = signal / (1.0f + signal);
    return mix(signal / (1.0f + luma), tv, tv);
}

#define HLG 1
#define DESAT 1

void main()
{
    // BT.2020 HLG
    vec4 color = ngl_texvideo(tex, var_tex_coord);

    // hlg eotf (linearize), output range [0, 12] (ITU-R BT.2100)
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = mix(vec3(4.0) * color.rgb * color.rgb,
                    exp((color.rgb - vec3(0.559911)) * vec3(1.0/0.178833)) + vec3(0.284669),
                    lessThan(vec3(0.5), color.rgb));

    // scale hlg eotf output range from [0, 12] against the hlg reference white
    // point (hlg_eotf(0.75)) = 3.179550717 recommended by ITU-R BT.2408 which
    // gives an output range of [0, 3.774118128023557]
    color.rgb *= (1.0/3.179550717);

    // hlg ootf (scene light -> display light) (ITU-R BT.2100) and scale it
    // from [0, (3.774118128023557)**1.2] to [0, 1000/203] which a scale factor of 1.0007494843358407
    const vec3 luma_coeff = vec3(0.2627, 0.6780, 0.0593); // luma_coeff fot BT.2020
    color.rgb *= vec3(1.0007494843358407 * pow(dot(luma_coeff, color.rgb), 0.200000));

    float signal_peak = 1000.0/203.0;
    vec3 signal = min(color.rgb, vec3(signal_peak));
    vec3 sig = signal;

    /*
        - max: `N=max(r,g,b)`
        - luminance Y: `N=rR+gG+bB` (luma weights)
        - power norm: `N=(r³+g³+b³)/(r²+g²+b²)`
        → `tonemap(color) * vec3(r,g,b)/N`
    */

#if DESAT == 1
    float l = dot(luma_coeff, signal);
    sig = vec3(l);

    //float l = signal;
    //sig = signal;

#elif DESAT == 2
    vec3 sig_orig = sig;
#else
#endif

#if 0
    vec4 log_avg = textureLod(tex2, vec2(0, 0), 20.0);
    float luma_avg = exp(log_avg).r;
    //if (a.r < 0.48)
    //    discard;
    //a.r=0.56;

    sig *= 0.25/luma_avg;
    signal_peak *= 0.25/luma_avg;
#endif

#if 1

    //sig = tonemap_bt2390(vec4(sig.rgb, signal_peak)).rgb;
    sig = tonemap_reinhard(vec4(sig.rgb, signal_peak)).rgb;
    //sig = tonemap_reinhard_extended(vec4(sig.rgb, signal_peak)).rgb;
    //sig = tonemap_reinhard_jodie(vec4(sig.rgb, signal_peak), l).rgb;

#else

    sig.r = Tonemap_ACES(sig.r);
    sig.b = Tonemap_ACES(sig.b);
    sig.g = Tonemap_ACES(sig.g);

#endif

    vec3 b = sig;


#if DESAT == 1
    color.rgb *= sig.r / l;
#elif DESAT == 2
    color.rgb = color.rgb * (sig / sig_orig);
#else
#endif

    //color.rgb = b;


    // convert colors from BT.2020 to BT.709
    const mat3 bt2020_to_bt709 = mat3(
        1.660491,    -0.12455047, -0.01815076,
        -0.58764114,  1.1328999,  -0.1005789,
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
    p.update_vert_out_vars(var_tex2_coord=ngl.IOVec2())
    render = ngl.Render(q, p)
    render.update_vert_resources(rotate_matrix=ngl.UniformMat4(transform=ngl.Rotate(ngl.Identity(), axis=(0, 0, 1), angle=180, anchor=(0.5, 0.5, 0.0))))
    render.update_frag_resources(tex=t, tex2=target)


    vert2 = '''
void main()
{
    ngl_out_pos = ngl_projection_matrix * ngl_modelview_matrix * vec4(ngl_position, 1.0);
    var_uvcoord = ngl_uvcoord;
    mat4 m = tex_coord_matrix * mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, -1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 1.0, 0.0, 1.0
    );
    var_tex_coord = (m * vec4(ngl_uvcoord, 0.0, 1.0)).xy;
}
'''

    frag2 = '''
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
    color.rgb *= vec3(1.0007494843358407 * pow(dot(luma, color.rgb), 0.200000));


    color.rgb = vec3(log(max(dot(luma, color.rgb), 1e-3)));
    color.a = 1.0;


    ngl_out_color = color;
}
'''

    q = ngl.Quad((-1, -1, 0), (2, 0, 0), (0, 2, 0))
    p = ngl.Program(vertex=vert2, fragment=frag2)
    p.update_vert_out_vars(var_tex_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    render2 = ngl.Render(q, p)
    render2.update_frag_resources(tex=t)

    rtt = ngl.RenderToTexture(render2, color_textures=(target,))

    vert3='''
void main()
{
    ngl_out_pos = ngl_projection_matrix * ngl_modelview_matrix * vec4(ngl_position, 1.0);
    var_uvcoord = ngl_uvcoord;
    var_tex0_coord = (tex0_coord_matrix * vec4(ngl_uvcoord, 0.0, 1.0)).xy;
}
'''

    frag3='''
void main()
{
    vec4 color = ngl_tex2dlod(tex0, var_tex0_coord, 20.0);
    color = exp(color) / (1000.0/203.0);
    //if (color.r > 0.17)
    //discard;
    color.a = 1.0;

    ngl_out_color = color;
}
'''

    q = ngl.Quad((0.25, 0.25, 0), (0.75, 0, 0), (0, 0.75, 0))
    p = ngl.Program(vertex=vert3, fragment=frag3)
    p.update_vert_out_vars(var_tex0_coord=ngl.IOVec2(), var_uvcoord=ngl.IOVec2())
    render3 = ngl.Render(q, p)
    render3.update_frag_resources(tex0=target)

    return ngl.Group(children=(rtt, render, render3))

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
    return ngl.RenderTexture(t, geometry=q)


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
    r = ngl.RenderTexture(t)

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

    return group
