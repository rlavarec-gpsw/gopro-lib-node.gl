const float PQ_M1 = 0.1593017578125;
const float PQ_M2 = 78.84375;
const float PQ_C1 = 0.8359375;
const float PQ_C2 = 18.8515625;
const float PQ_C3 = 18.6875;
const float PQ_C  = 10000.0;

vec4 pq_eotf_inverse(vec4 signal)
{
    const vec4 Y = signal / (10000.0 / 203.0);
    const vec4 Ym = pow(Y, vec4(PQ_M1));
    return pow((PQ_C1 + PQ_C2 * Ym) / (1.0 + PQ_C3 * Ym), vec4(PQ_M2));
}

vec4 pq_eotf(vec4 signal)
{
    return (10000.0 / 203.0) * pow(max(pow(signal, vec4(1.0 / PQ_M2)) - vec4(PQ_C1), vec4(0.0)) / (vec4(PQ_C2) - vec4(PQ_C3) * pow(signal, vec4(1.0 / PQ_M2))), vec4(1.0 / PQ_M1));
}

vec3 bt2390_eetf(vec3 v, float maxLum)
{
    float ks = 1.5 * maxLum - 0.5;
    const vec3 t = (v.rgb - vec3(ks)) / vec3(1.0 - ks);
    const vec3 t2 = t * t;
    const vec3 t3 = t2 * t;
    const vec3 p = (2.0 * t3 - 3.0 * t2 + vec3(1.0)) * vec3(ks) + (t3 - 2.0 * t2 + t) * vec3(1.0 - ks) + (-2.0 * t3 + 3.0 * t2) * vec3(maxLum);
    return mix(p, v, lessThan(v, vec3(ks)));
}

vec4 tonemap_bt2390(vec4 signal)
{
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

vec4 tonemap_reinhard(vec4 signal)
{
    return (signal / (signal + 1.0));
}

vec4 tonemap_reinhard_extended(vec4 signal)
{
    signal.a = 1000.0 / 203.0;
    vec3 numerator = signal.rgb * (1.0 + (signal.rgb / vec3(signal.a * signal.a)));
    return vec4(numerator / (1.0 + signal.rgb), 1.0);
}

vec4 tonemap_reinhard_jodie(vec4 signal, float luma)
{
    vec4 tv = signal / (1.0 + signal);
    return mix(signal / (1.0 + luma), tv, tv);
}

float aces(float x)
{
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

vec3 tonemap_aces(vec3 signal)
{
    return vec3(aces(signal.r), aces(signal.g), aces(signal.b));
}

void main()
{
    // BT.2020 HLG
    vec4 color = ngl_texvideo(tex0, var_tex0_coord);

    // hlg eotf (linearize), output range [0, 12] (ITU-R BT.2100)
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = mix(vec3(4.0) * color.rgb * color.rgb,
                    exp((color.rgb - vec3(0.559911)) * vec3(1.0/0.178833)) + vec3(0.284669),
                    lessThan(vec3(0.5), color.rgb));

    // scale hlg eotf output range from [0, 12] against the hlg reference white
    // point (hlg_eotf(0.75)) = 3.179550717 recommended by ITU-R BT.2408 which
    // gives an output range of [0, 3.774118128023557]
    color.rgb /= 3.179550717;

    // hlg ootf (scene light -> display light) (ITU-R BT.2100) and scale it
    // from [0, (3.774118128023557)**1.2] to [0, 1000/203] which a scale factor of 1.0007494843358407
    const vec3 luma_coeff = vec3(0.2627, 0.6780, 0.0593); // luma_coeff fot BT.2020
    color.rgb *= vec3(1.0007494843358407 * pow(dot(luma_coeff, color.rgb), 0.2));

    float signal_peak = 1000.0 / 203.0;
    vec3 signal = min(color.rgb, vec3(signal_peak));
    vec3 sig = signal;

    /*
     * TODO
     * - max: `N=max(r,g,b)`
     * - luminance Y: `N=rR+gG+bB` (luma weights)
     * - power norm: `N=(r³+g³+b³)/(r²+g²+b²)`
     * → `tonemap(color) * vec3(r,g,b)/N`
     */

    vec3 sig_orig = sig;
    float l = dot(luma_coeff, signal);
    if (desat == 0)
        sig = vec3(l);

    if (honor_logavg) {
        vec4 log_avg = textureLod(logavg_tex, vec2(0.0), 20.0);
        float luma_avg = exp(log_avg).r;
        //if (a.r < 0.48)
        //    discard;
        //a.r=0.56;

        sig *= 0.25/luma_avg;
        signal_peak *= 0.25/luma_avg;
    }

    if (tonemap == 0) {
        sig = tonemap_bt2390(vec4(sig.rgb, signal_peak)).rgb;
    } else if (tonemap == 1) {
        sig = tonemap_reinhard(vec4(sig.rgb, signal_peak)).rgb;
    } else if (tonemap == 2) {
        sig = tonemap_reinhard_extended(vec4(sig.rgb, signal_peak)).rgb;
    } else if (tonemap == 3) {
        sig = tonemap_reinhard_jodie(vec4(sig.rgb, signal_peak), l).rgb;
    } else if (tonemap == 4) {
        sig = tonemap_aces(sig.rgb);
    }

    if (desat == 0)
        color.rgb *= sig.r / l;
    else if (desat == 1)
        color.rgb = color.rgb * (sig / sig_orig);

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
