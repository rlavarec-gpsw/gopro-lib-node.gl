void main()
{
    vec4 color = ngl_texvideo(tex0, var_tex0_coord);

    // hlg eotf (linearize), output range [0, 12]
    color.rgb = clamp(color.rgb, 0.0, 1.0);
    color.rgb = mix(vec3(4.0) * color.rgb * color.rgb, exp((color.rgb - vec3(0.559911)) * vec3(1.0/0.178833)) + vec3(0.284669), lessThan(vec3(0.5), color.rgb));

    // scale hlg eotf output range from [0, 12] against the hlg reference white point (hlg_eotf(0.75)) = 3.179550717
    color.rgb /= 3.179550717;

    // hlg ootf (scene light -> display light)
    const vec3 luma = vec3(0.2627, 0.6780, 0.0593);
    color.rgb *= vec3(1.0007494843358407 * pow(dot(luma, color.rgb), 0.2));

    color.rgb = vec3(log(max(dot(luma, color.rgb), 1e-3)));
    color.a = 1.0;

    ngl_out_color = color;
}
