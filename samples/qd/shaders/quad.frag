#version 450

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_texcoord;

layout(set = 0, binding = 1) uniform sampler uSampler0;

layout(location = 0) out vec4 out_color;

void main()
{
//    vec3 color = texture(uSampler0, in_texcoord).rgb;
    out_color = in_color;
}
