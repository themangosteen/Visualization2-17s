#version 330 core

in vec3 dir_vert; // direction passed from the vertex shader
in vec2 uv_vert; // uv from the vertex shader

// final intensities passed to framebuffer
layout(location = 0) out vec4 outColor;

// uniforms are not interpolated or passed on
uniform vec3 color;

void main()
{
    outColor = vec4(uv_vert.y, uv_vert.y, uv_vert.y, 1.0);
}
