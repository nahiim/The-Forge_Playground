#version 450

layout(location = 0) in vec4 in_color;

layout(location = 0) out vec4 out_color;

void main()
{
    vec2 coord = gl_PointCoord - vec2(0.5);
    out_color = vec4(in_color.rgb, 0.5 - length(coord));
}
