#version 330 core

// in attributes from bound vertex array buffers
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 direction;
layout(location = 1) in vec2 uv;

// out attributes passed to fragment shader
out vec3 dir_vert;
out vec2 uv_vert;

// uniforms are not interpolated or passed on
uniform mat4 viewMat;
uniform mat4 projMat;

void main()
{
    gl_Position = projMat * viewMat * vec4(position, 1.0);
    dir_vert = direction;
    uv_vert = uv;
}
