#version 330 core

in vec3 dir_vert; // direction passed from the vertex shader
in vec2 uv_vert; // uv from the vertex shader

// final intensities passed to framebuffer
layout(location = 0) out vec4 outColor;

// uniforms are not interpolated or passed on
uniform vec3 color;
uniform float lineHaloWidth;
uniform float lineWidthPercentage;

void main()
{
    //eq. 2 from Everts et al.
//    float s = lineHaloWidth*abs(uv_vert.y-0.5); // distance s to the center of the strip
    float s = abs(uv_vert.y-0.5); // distance s to the center of the strip
//    if(s<(0.5*lineHaloWidth)){ // "If this distance s is smaller than half the line width..."<--mistake in paper, should be quarter
    if(s < lineWidthPercentage*0.5){ // "If this distance s is smaller than half the line width..."
        // "... the fragment's output color is set to white and its depth is adjusted depending on the distance [...]"
            outColor = vec4(0,0,0,1);
    } else { // HALO
        outColor = vec4(1,1,1,1);
    }
}
