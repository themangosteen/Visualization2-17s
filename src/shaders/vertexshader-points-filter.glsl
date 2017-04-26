attribute float extraData;
uniform vec3 upBackP;
uniform vec3 sideBackP;
varying float extraDataValue;

void main(void)
{
	gl_TexCoord[0] = vec4(gl_MultiTexCoord0);
	vec3 side = (gl_TexCoord[0].x - 0.5)*sideBackP;
	vec3 up = (gl_TexCoord[0].y - 0.5)*upBackP;
	vec4 position = gl_Vertex + vec4(side,0.0) +  vec4(up,0.0);
	gl_Position = gl_ModelViewProjectionMatrix * position;
	extraDataValue = extraData;
}
