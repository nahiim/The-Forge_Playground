#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec3 in_tangent;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec2 out_texcoord;

layout(set = 0, binding = 0) uniform QuadUBO
{
    vec4 color;
    mat4 model;
    mat4 project_view;
}quadUBO;

void main()
{
    gl_Position = quadUBO.project_view * quadUBO.model * vec4(in_position, 1.0);
    out_color = quadUBO.color;
    out_texcoord = in_texcoord;
}
