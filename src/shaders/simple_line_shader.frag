#version 330 core

// final intensities passed to framebuffer
layout(location = 0) out vec4 outColor;

// uniforms are not interpolated or passed on
uniform vec3 color;

void main()
{
    outColor = vec4(color, 1.0);
}
