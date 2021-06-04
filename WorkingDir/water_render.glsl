#ifdef WATER_REFL_REFR_RENDER_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=2) in vec2 aTexCoord;


uniform mat4 uWorldMatrix;
uniform mat4 uWorldViewProjectionMatrix;


out vec2 vTexCoord;
out vec3 vPosition;

uniform vec4 clippingPlane;

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3( uWorldMatrix * vec4(aPosition,1.0));

	gl_ClipDistance[0] = dot(vec4(vPosition,1.0),clippingPlane);

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = vec4(0.0f);
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif
