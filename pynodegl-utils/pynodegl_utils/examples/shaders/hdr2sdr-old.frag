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

vec3 tonemap_bt2446_a(vec3 x)
{
    const float l_hdr = 1000.0;
    const float l_sdr = 100.0;
    const float p_hdr = 1.0 + 32.0 * pow(l_hdr / 10000.0, 1.0 / 2.4);
    const float p_sdr = 1.0 + 32.0 * pow(l_sdr / 10000.0, 1.0 / 2.4);

    //float luma = dot(luma_coeff, pow(x, vec3(1.0 / 2.4)));
    //x = pow(x, vec3(1.0 / 2.4));

    vec3 yp = log(1.0 + (p_hdr - 1.0) * x) / log(p_hdr);

    vec3 yc = mix(
        mix(
            0.5 * yp + 0.5,
            (-1.1510 * yp + 2.7811) * yp - 0.6302,
            lessThan(yp, vec3(0.9909))
        ),
        1.077 * yp,
        lessThan(yp, vec3(0.7399))
    );

    return (pow(vec3(p_sdr), yc) - 1.0) / (vec3(p_sdr) - 1.0);
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
vec3 hlg_ootf(vec3 x, float gain)
{
    return gain * x * pow(dot(luma_coeff, x), 0.2); // 0.2 is 1-gamma with gamma=1.2
}

vec3 tonemap_trf(vec3 x, float peak)
{
    if (tonemap == 1) return tonemap_bt2390(x, peak);
    if (tonemap == 2) return tonemap_reinhard(x);
    if (tonemap == 3) return tonemap_reinhard_extended(x, peak);
    if (tonemap == 4) return tonemap_reinhard_jodie(x);
    if (tonemap == 5) return tonemap_aces(x);
    if (tonemap == 6) return tonemap_bt2446_a(x);
    return x;
}

float get_norm(vec3 x)
{
    if (norm_method == 1)
        return max(x.r, max(x.g, x.b));
    if (norm_method == 2)
        return (x.r*x.r*x.r + x.g*x.g*x.g + x.b*x.b*x.b) / (x.r*x.r + x.g*x.g + x.b*x.b);
    return dot(luma_coeff, x);
}

vec3 apply_tonemap(vec3 x)
{
    float peak = 1000.0 / 203.0;
    vec3 sig = min(x, vec3(peak));

    float norm = get_norm(sig);
    sig = vec3(norm);

    if (honor_logavg) {
        vec4 log_avg = textureLod(logavg_tex, vec2(0.0), 20.0);
        float luma_avg = 4.0 * exp(log_avg).r;
        sig /= luma_avg;
        peak /= luma_avg;
    }

    sig = tonemap_trf(sig, peak);

    x *= sig.r / norm;

    return x;
}

vec3 bt2020_to_bt709(vec3 x)
{
    const mat3 bt2020_to_bt709 = mat3(
        1.660491,    -0.12455047, -0.01815076,
        -0.58764114,  1.1328999,  -0.1005789,
        -0.07284986, -0.00834942,  1.11872966);
    return bt2020_to_bt709 * x;
}

void main()
{
    // BT.2020 HLG
    vec4 color = ngl_texvideo(tex0, var_tex0_coord);

    vec3 x = color.rgb;
    x = clamp(x, 0.0, 1.0);
    x = hlg_eotf(x);

    // scale hlg eotf output range from [0, 12] against the hlg reference white
    // point (hlg_eotf(0.75)) = 3.179550717436802 recommended by ITU-R BT.2408 which
    // gives an output range of [0, 3.774118127505075]
    x /= 3.179550717436802;
    //x /= 12.0;

    // from [0, 3.774118127505075**1.2] to [0, 1000/203]
    x = hlg_ootf(x, 1.0007494845008182);
    //x = hlg_ootf(x, 1.0);

    x = apply_tonemap(x);

    x = bt2020_to_bt709(x);

    // delinearize (gamma 2.2)
    x = clamp(x, 0.0, 1.0);
    x = pow(x, vec3(1.0 / 2.2));

    ngl_out_color = vec4(x, color.a);
}
