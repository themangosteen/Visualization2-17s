uniform float maxDisplacement;
varying float extraDataValue;
uniform float attributeThreshold;
uniform int useExtraData;
uniform float psize2;

void main(void)
{
	if (useExtraData==1 && extraDataValue < attributeThreshold) {
		discard;
	}
	float x = abs(gl_TexCoord[0].x-0.5);
	float y = abs(gl_TexCoord[0].y-0.5);
	float d2 = x*x+y*y;
	if (d2 > 0.25) {
		discard;
	}
	gl_FragDepth = gl_FragCoord.z;
	if (d2 < psize2) {
		gl_FragColor = vec4(0.0,0.0,0.0,1.0);
	} else {
		gl_FragColor = vec4(1.0,1.0,1.0,1.0);
		float d = sqrt(d2)*4.0; // *4.0 -> == /(0.5)^2
		float displacement = d*maxDisplacement;
		gl_FragDepth += displacement;
	}
}


