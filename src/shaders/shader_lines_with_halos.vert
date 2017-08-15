#version 330 core

// in attributes from bound vertex array buffers
// note: to draw line as triangle strip we need all line vertices twice: with same position, direction and u, but different v
// basically we have a zero-width triangle strip which we widen by displacing vertices based on the v variable
// which is 0 at one side and 1 at the other of the strip perpendicular to the line direction
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 direction; // direction to next line vertex
layout(location = 2) in vec2 uv; // u is between 0 and 1 based on position in line direction, v is discrete EITHER 0 OR 1 later used to move perpendicular to direction between sides of triangle strip

// out attributes passed to fragment shader
out vec3 vertDirection;
out vec2 vertUV;
out float discardFragment;

// uniforms are not interpolated or passed on
uniform mat4 viewMat;
uniform mat4 projMat;
uniform vec3 cameraPos;
uniform float lineTriangleStripWidth;
uniform vec3 clipPlaneNormal;
uniform float clipPlaneDistance;

void main()
{   
    // VIEW ALIGNED TRIANGLE STRIPS
    // widen zero-width triangle strip and make view aligned:
    // move vertices perpendicular to both line and view direction
    // v = 1 moves half strip width in cross product direction, v = 0 moves half strip width in opposite direction)
    vec3 viewAlignedPerpendicularDirection = normalize(cross(position - cameraPos, direction));
    vec3 viewAlignedPosition = position + viewAlignedPerpendicularDirection * (uv.y-0.5)*lineTriangleStripWidth;

    // tell fragment shader to discard fragments beyond a certain distance from origin in clipping plane direction
    discardFragment = 0;
    if (dot(viewAlignedPosition, clipPlaneNormal) > clipPlaneDistance)
        discardFragment = 1;

    // dirty hack: we use this to discard fragments connecting end and start vertices of two lines
    // this allows us to just use one vbo for all the triangle strip vertices which is much faster.
    if (position.z == 42.4242)
        discardFragment = 1;

    gl_Position = projMat * viewMat * vec4(viewAlignedPosition, 1.0);
    vertDirection = direction;
    vertUV = uv;
}
