void main()
{
    vec4 color = ngl_tex2dlod(tex0, var_tex0_coord, 20.0);
    ngl_out_color = vec4(exp(color.rgb) / (1000.0 / 203.0), 1.0);
}
