#version 330 core

// in attributes from bound vertex array buffers
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 direction;
layout(location = 2) in vec2 uv;

// out attributes passed to fragment shader
out vec3 dir_vert;
out vec2 uv_vert;

// uniforms are not interpolated or passed on
uniform mat4 viewMat;
uniform mat4 projMat;
uniform vec3 cameraPos;
uniform float lineHaloWidth;

void main()
{
    vec3 viewAlignedPosition = position + normalize(cross(position-cameraPos, direction))*(uv.y-0.5)*lineHaloWidth;
    gl_Position = projMat * viewMat * vec4(viewAlignedPosition, 1.0);
    dir_vert = direction;
    uv_vert = uv;
}
