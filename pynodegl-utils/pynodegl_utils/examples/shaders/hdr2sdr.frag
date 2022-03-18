const float PQ_M1 = 0.1593017578125;
const float PQ_M2 = 78.84375;
const float PQ_C1 = 0.8359375;
const float PQ_C2 = 18.8515625;
const float PQ_C3 = 18.6875;
const float PQ_C  = 10000.0;

const vec3 luma_coeff = vec3(0.2627, 0.6780, 0.0593); // luma_coeff for BT.2020

vec4 pq_eotf_inverse(vec4 signal)
{
    const vec4 Y = signal / (10000.0 / 203.0);
    const vec4 Ym = pow(Y, vec4(PQ_M1));
    return pow((PQ_C1 + PQ_C2 * Ym) / (1.0 + PQ_C3 * Ym), vec4(PQ_M2));
}

vec3 pq_eotf(vec3 x)
{
    return (10000.0 / 203.0) * pow(max(pow(x, vec3(1.0 / PQ_M2)) - PQ_C1, 0.0) / (PQ_C2 - PQ_C3 * pow(x, vec3(1.0 / PQ_M2))), vec3(1.0 / PQ_M1));
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

vec3 tonemap_bt2390(vec3 x, float peak)
{
    // tone map ITU-R BT.2390
    // convert linear light to PQ space
    vec4 sig_pq = vec4(x, peak);
    sig_pq = pq_eotf_inverse(sig_pq);

    // normalize pq signal against signal peak
    float scale = 1.0 / sig_pq.a;

    // solve EETF ITU-R BT.2390
    float maxLum = 0.580690;
    vec3 sig = bt2390_eetf(sig_pq.rgb * scale, maxLum * scale);

    // invert normalization
    sig /= scale;

    // convert signal from PQ space to linear light
    return pq_eotf(sig);
}

vec3 tonemap_bt2446_a(vec3 x)
{
    // TODO
    return x;
}

vec3 tonemap_reinhard(vec3 x)
{
    return x / (x + 1.0);
}

vec3 tonemap_reinhard_extended(vec3 x, float peak)
{
    return x * (1.0 + (x / (peak * peak))) / (1.0 + x);
}

vec3 tonemap_reinhard_jodie(vec3 x)
{
    float luma = dot(luma_coeff, x);
    vec3 tv = x / (1.0 + x);
    return mix(x / (1.0 + luma), tv, tv);
}

vec3 tonemap_aces(vec3 x)
{
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

/* HLG EOTF (linearize), non-normalized output range of [0, 12] (ITU-R BT.2100) */
vec3 hlg_eotf(vec3 x)
{
    const float a = 0.17883277;
    const float b = 0.28466892;
    const float c = 0.55991073;
    return mix(4.0 * x * x, exp((x - c) / a) + b, lessThan(vec3(0.5), x));
}

/*
 * HLG input signal to OOTF: maps scene linear light to display linear light
 * (ITU-R BT.2100)
 */
vec3 hlg_ootf(vec3 x)
{
    return x * pow(dot(luma_coeff, x), 0.2); // 0.2 is 1-gamma with gamma=1.2
}

void main()
{
    // BT.2020 HLG
    vec4 color = ngl_texvideo(tex0, var_tex0_coord);

    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = hlg_eotf(color.rgb);

    // scale hlg eotf output range from [0, 12] against the hlg reference white
    // point (hlg_eotf(0.75)) = 3.179550717436802 recommended by ITU-R BT.2408 which
    // gives an output range of [0, 3.774118127505075]
    color.rgb /= 3.179550717436802;

    color.rgb = hlg_ootf(color.rgb);

    // from [0, 3.774118127505075**1.2] to [0, 1000/203] which a scale factor of 1.0007494843358407
    color.rgb *= 1.0007494845008182;


    float signal_peak = 1000.0 / 203.0;
    vec3 sig = min(color.rgb, vec3(signal_peak));

    vec3 sig_orig = sig;
    float l = dot(luma_coeff, sig);
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

    if (tonemap == 1) {
        sig = tonemap_bt2390(sig, signal_peak);
    } else if (tonemap == 2) {
        sig = tonemap_reinhard(sig);
    } else if (tonemap == 3) {
        sig = tonemap_reinhard_extended(sig, signal_peak);
    } else if (tonemap == 4) {
        sig = tonemap_reinhard_jodie(sig);
    } else if (tonemap == 5) {
        sig = tonemap_aces(sig);
    } else if (tonemap == 6) {
        sig = tonemap_bt2446_a(sig);
    }

    if (desat == 0)
        //color.rgb *= max(sig.r, max(sig.g, sig.b)) / l;
        color.rgb *= sig / l;
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
