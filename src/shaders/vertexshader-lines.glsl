varying vec3 tpos;
attribute vec3 direction;
uniform float stripWidth;

vec4 stripOrientation()
{
	float stripSize = stripWidth*0.5;
    float side = gl_MultiTexCoord0.y-0.5;
	vec3 referenceVector = normalize(vec3(gl_ModelViewMatrixInverse * vec4(0.0,0.0,0.0,1.0) - gl_Vertex));
	vec3 offset = side*stripSize*normalize(cross(direction,referenceVector));
	return gl_Vertex + vec4(offset,0.0);
}


void main(void)
{
	vec4 position = stripOrientation();
	gl_Position = gl_ModelViewProjectionMatrix * position;
	gl_TexCoord[0] = vec4(gl_MultiTexCoord0);
}

