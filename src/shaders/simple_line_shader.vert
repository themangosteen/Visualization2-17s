#version 330 core

// in attributes from bound vertex array buffers
layout(location = 0) in vec3 position;

// uniforms are not interpolated or passed on
uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;

void main()
{
    gl_Position = projMat * viewMat * modelMat * vec4(position, 1.0);
}
