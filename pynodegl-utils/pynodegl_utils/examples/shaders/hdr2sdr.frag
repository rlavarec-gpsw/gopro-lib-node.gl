const vec3 luma_coeff = vec3(0.2627, 0.6780, 0.0593); // luma_coeff for BT.2020

vec3 tonemap_bt2446(vec3 x)
{
    const float l_hdr = 1000.0;
    const float l_sdr = 100.0;
    const float p_hdr = 1.0 + 32.0 * pow(l_hdr / 10000.0, 1.0 / 2.4);
    const float p_sdr = 1.0 + 32.0 * pow(l_sdr / 10000.0, 1.0 / 2.4);

    float luma = dot(luma_coeff, pow(x, vec3(1.0 / 2.4)));

    x = vec3(luma); // XXX

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

vec3 apply_tonemap(vec3 x)
{
    //float peak = 1000.0 / 203.0;
    //vec3 sig = min(x, vec3(peak));

    float luma = dot(luma_coeff, pow(x, vec3(1.0 / 2.4)));
    vec3 sig = tonemap_bt2446(x);

    x *= sig.r / luma;

    return x;
}

vec3 bt2020_to_bt709(vec3 x)
{
    const mat3 bt2020_to_bt709 = mat3(
         1.660491,   -0.12455047, -0.01815076,
        -0.58764114,  1.1328999,  -0.1005789,
        -0.07284986, -0.00834942,  1.11872966);
    return bt2020_to_bt709 * x;
}

void main()
{
    // BT.2020 HLG
    vec4 color = ngl_texvideo(tex0, var_tex0_coord);

    vec3 x = color.rgb;
    x = hlg_eotf(x) / 12.0;

    x = hlg_ootf(x, 1.0);

    x = apply_tonemap(x);

    x = bt2020_to_bt709(x);

    // delinearize (gamma 2.2)
    x = clamp(x, 0.0, 1.0);
    x = pow(x, vec3(1.0 / 2.2));

    ngl_out_color = vec4(x, color.a);
}
