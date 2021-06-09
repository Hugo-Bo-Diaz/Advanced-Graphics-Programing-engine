#ifdef WATER_PLANE_RENDER_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;

uniform mat4 uProjectionMatrix;
uniform mat4 uWorldViewMatrix;

out Data
{
	vec3 positionViewspace;
	vec3 normalViewspace;
} VSOut;

uniform vec4 clippingPlane;

void main()
{
	VSOut.positionViewspace = vec3(uWorldViewMatrix * vec4(aPosition,1.0));
	VSOut.normalViewspace = vec3(uWorldViewMatrix * vec4(aNormal,0.0));

	gl_Position = uProjectionMatrix * uWorldViewMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////


uniform vec2 viewportSize;
uniform mat4 modelViewMatrix;
uniform mat4 viewMatrixInv;
uniform mat4 projectionMatrixInv;
uniform sampler2D reflectionMap;
uniform sampler2D refractionMap;
uniform sampler2D reflectionDepth;
uniform sampler2D refractionDepth;
uniform sampler2D normalMap;
uniform sampler2D dudvMap;

uniform sampler2D currdepthMap;
uniform int isDeferred;

in Data{
	vec3 positionViewspace;
	vec3 normalViewspace;
} FSIn;

layout(location = 0) out vec4 oColor;

vec3 fresnelSchlick(float cosTheta, vec3 FO){
	return FO + (1.0 - FO) * pow(1.0 - cosTheta, 5.0);
}

vec3 reconstructPixelPosition(float depth){
	vec2 texCoords = gl_FragCoord.xy / viewportSize;
	vec3 positionNDC = vec3(texCoords * 2.0 - vec2(1.0), depth * 2.0 - 1.0);
	vec4 positionEyespace = projectionMatrixInv * vec4(positionNDC, 1.0);
	positionEyespace.xyz /= positionEyespace.w;
	return positionEyespace.xyz;
}

void main()
{
	vec3 N = normalize(FSIn.normalViewspace);
	vec3 V = normalize(-FSIn.positionViewspace);
	vec3 Pw = vec3(viewMatrixInv * vec4(FSIn.positionViewspace,1.0));
	vec2 texCoord = gl_FragCoord.xy / viewportSize;

	const vec2 waveLength = vec2(2.0);
	const vec2 waveStrength = vec2(0.05);
	const float turbidityDistance = 10.0;

	vec2 distorsion = (2.0 * texture(dudvMap, Pw.xz / waveLength).rg - vec2(1.0))* waveStrength+waveStrength/7;

	vec2 reflectionTexCoord = vec2(texCoord.s, 1.0 - texCoord.t) + distorsion;
	vec2 refractionTexCoord = texCoord + distorsion;
	vec3 reflectionColor = texture(reflectionMap,reflectionTexCoord).rgb;
	vec3 refractionColor = texture(refractionMap,refractionTexCoord).rgb;
	
	vec2 inverseRefrTexCoord = texCoord + distorsion*0.1;
	vec3 inverserefractionColor = texture(reflectionMap,inverseRefrTexCoord).rgb;

	float distortedGroundDepth = texture(refractionDepth, refractionTexCoord).x;
	vec3 distortedGroundPosViewspace = reconstructPixelPosition(distortedGroundDepth);
	float distortedWaterDepth = FSIn.positionViewspace.z - distortedGroundPosViewspace.z;
	float tintFactor = clamp(distortedWaterDepth / turbidityDistance, 0.0, 1.0);
	vec3 waterColor = vec3(0.25,0.4,0.6);
	refractionColor = mix(refractionColor, waterColor, tintFactor);

	vec3 FO = vec3(0.1);
	vec3 F = fresnelSchlick(max(0.0, dot(V,N)),FO);
	oColor.rgb = mix(refractionColor,reflectionColor ,F);

	oColor.a = 1.0;
	if(isDeferred == 1)
	{
	vec2 UV = gl_FragCoord.xy/viewportSize;
	float texDepth = texture(currdepthMap,UV).r;

	if(texDepth < gl_FragCoord.z)
		oColor.a = 0.0;

	}
}

#endif
#endif
