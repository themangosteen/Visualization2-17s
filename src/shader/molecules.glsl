//////////////////////////////////////////////////////
-- Vertex

#extension GL_ARB_explicit_attrib_location : enable

// in attributes from bound vertex array buffers
in vec3 atomPos;
in float atomRadius;
in vec3 atomColor;

// out attributes are passed on to geometry shader
out vec4 atomPosView;
out float radius;
out vec4 vertColor;

// uniforms are not interpolated or passed on
// note: we actually have no model matrix, atom positions already are in world space
uniform mat4 viewMat;
uniform mat4 projMat;

void main(void)
{
    // note: we need no model matrix, atom positions already are in world space
    atomPosView = viewMat * vec4(atomPos, 1.0);
    gl_Position = atomPosView;
    radius = atomRadius;
    vertColor = vec4(atomColor, 1);
}


//////////////////////////////////////////////////////
-- Geometry
layout(points) in;
layout(triangle_strip, max_vertices = 4) out; // quad has 2 triangles and 4 vertices

// in attributes from vertex shader (not interpolated).
// note that the geometry shader uses arrays for input
// since we might also have multiple vertices as input
// in this case we just input single vertices (points)
// but the input still must be an array.
in vec4 atomPosView[]; // atom position in view space
in float radius[]; // array with one elem only (one input vertex)
in vec4 vertColor[]; // array with one elem only (one input vertex)

// out attributes are interpolated for each fragment and then accessible in fragment shader
out vec3 atomCenter;
out vec4 atomColor;
out float atomRadius;
out vec2 uv;

// uniforms are not interpolated or passed on
uniform mat4 projMat;

void main()
{
    atomCenter = atomPosView[0].xyz; // in view space
    atomColor = vertColor[0];
    atomRadius = radius[0];

    vec2 delta[4];
    delta[0] = vec2(-1, -1);
    delta[1] = vec2( 1, -1);
    delta[2] = vec2(-1,  1);
    delta[3] = vec2( 1,  1);

    // define billboard quad in view space
    // emitted vertices will have the last assigned values of varyings
    // e.g. since atomColor isnt changed, all have the same value
    // but uv is reassigned so values will differ and thus also be interpolated
    for (int i = 0; i < 4; ++i) {

        vec2 vertXY = atomCenter.xy + delta[i] * atomRadius;
        gl_Position = projMat * vec4(vertXY, atomCenter.z, 1);
        uv = (delta[i]+vec2(1))/2;
        EmitVertex();
    }

    EndPrimitive();

}


//////////////////////////////////////////////////////
-- Fragment

// interpolated attributes from geometry shader
in vec3 atomCenter; // atom center in view space
in vec4 atomColor;
in float atomRadius;
in vec2 uv; // fragment uv coordinates interpolated from quad vertex uv

// final intensities passed to framebuffer
out vec4 outColor;

// uniforms are not interpolated or passed on
uniform mat4 viewMat;
uniform mat4 projMat;
uniform vec3 lightPos; // in view space
uniform sampler2D diffuseTex;

uniform float ambientFactor;
uniform float diffuseFactor;
uniform float specularFactor;
uniform float shininess;
uniform float contourWidth;

void main()
{   
    //vec4 texColor = texture(diffuseTex, uv).rgba; // TEXTURES NOT YET NEEDED, ONLY FOR AMBIENT OCCLUSION

    // CALCULATE UNIT HALFSPHERE SURFACE NORMAL AND POSITION IN VIEW SPACE
    // we will be mapping the uv billboard space to surface of a unit sphere x^2 + y^2 + z^2 = 1^2
    // construct halfsphere z component as z = sqrt(1 - (x^2 + y^2))
    vec2 sphereSurfaceXY = uv*2 - vec2(1); // map uv [0,1] to range [-1,1], this will be the xy of a unit halfsphere
    float distFromCenterXYSq = dot(sphereSurfaceXY, sphereSurfaceXY);
    if (distFromCenterXYSq > 1) discard; // DISCARD FRAGMENTS OUTSIDE SPHERE
    float distFromCenterXY = sqrt(distFromCenterXYSq); // distance from sphere center in xy plane
    float sphereSurfaceZ = sqrt(1 - distFromCenterXYSq); // z of unit sphere
    vec3 N = normalize(vec3(sphereSurfaceXY, sphereSurfaceZ)); // normal of unit sphere
    vec3 P = atomCenter + N * atomRadius; // sphere surface position corresponding to fragment, in view space

    // CALCULATE DEPTH FROM VIEW SPACE SPHERE SURFACE Z
    vec4 Pclipspace = projMat * vec4(P, 1); // project to clip space via perspective projection
    float depthNDC = Pclipspace.z / Pclipspace.w; // perspective divide (homogenization after perspective projection)
    gl_FragDepth = ((gl_DepthRange.far - gl_DepthRange.near) * depthNDC + gl_DepthRange.far + gl_DepthRange.near)/2;

    // DRAW DEPTH AWARE CONTOUR LINES
    float contourStart = 1 - contourWidth; // distance from center at which to start drawing contour
    if (distFromCenterXY > contourStart) {
        // draw depth aware contour lines
        float deltaZ = 0.00005; // z displacement on contour will linearly increase from 0 to deltaZ with distance from center
        float contourRelativeDistance = (distFromCenterXY-contourStart)/contourWidth; // range [0, 1] for [contourStart, contourStart+width]
        if (contourRelativeDistance > 0.25) { // don't draw part of contour at this fragment if it would be very thin
            gl_FragDepth = gl_FragCoord.z + deltaZ * contourRelativeDistance;
            outColor = vec4(0, 0, 0, 1);
            return;
        }
    }

    // BLINN-PHONG ILLUMINATION OF UNIT HALFSPHERE

    vec3 L = normalize((viewMat*vec4(lightPos,1)).xyz - P); // light vector (surface to light)
    vec3 V = normalize(-P); // view vector (surface to camera), camera is at origin in view space.

    vec4 lightColor = vec4(1);
    vec4 ambient = ambientFactor * atomColor;
    vec4 diffuse = max(dot(N, L), 0.0) * diffuseFactor * lightColor * atomColor;

    // use N as half vector of light vector and its specular reflection
    // to avoid calculating reflected light vector R
    vec3 H = normalize(L + V); // half vector of light and view vectors (H to N has same ratio as V to R)
    vec4 specular = pow(max(dot(H, N), 0.0), shininess) * specularFactor * lightColor * atomColor;

    outColor = ambient + diffuse + specular;

}
