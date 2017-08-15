#version 330 core

in vec3 vertDirection; // direction to next line vertex
in vec2 vertUV; // u is in [0,1] interpolated along line length, v is in [0,1] interpolated perpendicular to direction between sides of triangle strip
in float discardFragment;

// final intensities passed to framebuffer
layout(location = 0) out vec4 outColor;

// uniforms are not interpolated or passed on
uniform vec3 colorLine;
uniform vec3 colorHalo;
uniform float lineTriangleStripWidth;
uniform float lineWidthPercentageBlack; // percentage of triangle strip drawn black to represent line (rest is white halo)
uniform float lineWidthDepthCueingFactor; // how much the black line is drawn thinner with increasing depth
uniform float lineHaloMaxDepth; // maximum depth displacement for white halo fragments
uniform mat4 inverseProjMat;

float getLinearizedFragmentDepth()
{
    // get linear depth at fragment
    // gl_FragCoord.z is nonlinear due to perspective projection, we must linearize it
    vec3 NDC = gl_FragCoord.xyz * 2.0 - 1.0; // map fragment coordinates [0,1] to normalized device coordinates [-1,1]
    vec4 unprojectedNDC = inverseProjMat * vec4(NDC, 1.0); // undo the perspective projection by applying inverse projMat
    unprojectedNDC /= unprojectedNDC.w;
    float depth = -(unprojectedNDC.z / 2.0 + 0.5);

    return depth; // depth in [0,1] where 0 is near plane, 1 is flar plane
}

void main()
{
    // discard fragments beyond a certain distance from origin in clipping plane direction
    if (discardFragment > 0)
        discard;

    // DEPTH-DEPENDENT HALOS
    // we assign either black (representation of line) or white (halo around line) to fragments on the triangle strip
    // we assign black for a certain percentage of fragments based on fragment offset from centerline of strip (v coordinate)
    // and assign the remaining fragments white for the surrounding halo.
    // the depth of the white halo fragments is displaced based on the offset
    // so that lines that are close in depth are occluded by thinner or no halo so that they appear as black bundles.
    //
    //  | d         xxxxx BLACK (no depth displacement)
    //  | e         |   |
    //  | p        /     \ WHITE (halo with depth displacement)
    //  | t   x---/       \------------> a perpendicular line occluding the rest of the halo
    //  v h      /         \

    float offset = 2*abs(vertUV.y - 0.5); // relative offset of fragment from centerline of strip (perpendicular to line direction)
    float depth = getLinearizedFragmentDepth(); // depth in [0,1]
    float offsetThreshold = lineWidthPercentageBlack * (1 - depth*lineWidthDepthCueingFactor); // allow for black percentage to depend on depth (distant line thinning)

    if (offset < offsetThreshold) {
        outColor = vec4(colorLine,1); // assign black (to represent line)
        gl_FragDepth = depth; // depth unchanged, but we must assign gl_FragDepth for all cases if we assign it somewhere
    }
    else {
        outColor = vec4(colorHalo,1); // assign white (for surrounding halo)
        gl_FragDepth = depth + offset*lineHaloMaxDepth; // displace depth with increasing offset
    }
}
