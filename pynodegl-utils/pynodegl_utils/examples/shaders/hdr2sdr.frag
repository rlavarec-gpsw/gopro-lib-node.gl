const vec3 luma_coeff = vec3(0.2627, 0.6780, 0.0593); // luma weights for BT.2020
const float l_hdr = 1000.0;
const float l_sdr = 100.0;
const float p_hdr = 1.0 + 32.0 * pow(l_hdr / 10000.0, 1.0 / 2.4);
const float p_sdr = 1.0 + 32.0 * pow(l_sdr / 10000.0, 1.0 / 2.4);
const float gcr = luma_coeff.r / luma_coeff.g;
const float gcb = luma_coeff.b / luma_coeff.g;

/* BT.2446 */
vec3 tonemap(vec3 x)
{
    vec3 xp = pow(x, vec3(1.0 / 2.4));
    float y_hdr = dot(luma_coeff, xp);

    /* Step 1: convert signal to perceptually linear space */
    float yp = log(1.0 + (p_hdr - 1.0) * y_hdr) / log(p_hdr);

    /* Step 2: apply knee function in perceptual domain */
    float yc = mix(
        1.077 * yp,
        mix((-1.1510 * yp + 2.7811) * yp - 0.6302, 0.5 * yp + 0.5, yp > 0.9909),
        yp > 0.7399);

    /* Step 3: convert back to gamma domain */
    float y_sdr = (pow(p_sdr, yc) - 1.0) / (p_sdr - 1.0);

    /* Colour correction */
    float scale = y_sdr / (1.1 * y_hdr); // XXX is 1.1 for video range bs?
    float cb_tmo = scale * (xp.b - y_hdr);
    float cr_tmo = scale * (xp.r - y_hdr);
    float y_tmo = y_sdr - max(0.1 * cr_tmo, 0.0);

    /* Convert from Y'Cb'Cr' to R'G'B' (still in BT.2020) */
    float cg_tmo = -(gcr * cr_tmo + gcb * cb_tmo);
    return y_tmo + vec3(cr_tmo, cg_tmo, cb_tmo);
}

/* HLG Reference EOTF (linearize: R'G'B' HDR → RGB HDR), normalized, ITU-R BT.2100 */
vec3 hlg_eotf(vec3 x)
{
    const float a = 0.17883277;
    const float b = 0.28466892;
    const float c = 0.55991073;
    return mix(x * x / 3.0, (exp((x - c) / a) + b) / 12.0, lessThan(vec3(0.5), x));
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
    vec4 hdr = ngl_texvideo(tex0, var_tex0_coord); // BT.2020 HLG
    vec3 sdr = bt2020_to_bt709(tonemap(hlg_eotf(hdr.rgb)));
    ngl_out_color = vec4(sdr, hdr.a);
}
