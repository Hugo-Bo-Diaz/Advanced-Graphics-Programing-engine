#ifdef MAP_CALCULATION_SHADER

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

layout(location=3) in vec3 tangent;
layout(location=4) in vec3 bitangent;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 aPos;

out Data
{
	vec3 tangentLocalspace;
	vec3 bitangentLocalspace;
	vec3 normalLocalspace;
} VSOut;

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3( uWorldMatrix * vec4(aPosition,1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal,0.0));

	/*float clippingScale = 5.0;
	gl_Position = vec4(aPosition, clippingScale);
	gl_Position.z = -gl_Position.z;*/

	aPos = aPosition;


	VSOut.tangentLocalspace = tangent;
	VSOut.bitangentLocalspace = bitangent;
	VSOut.normalLocalspace = aNormal;

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

struct Light
{
unsigned int type;
vec3 color;
vec3 direction;
vec3 position;
};

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform sampler2D uDepthMap;
uniform sampler2D uSpecularMap;

uniform float specular;
uniform int normalMapExists;
uniform int depthMapExists;
uniform int specularMapExists;

uniform float depthStrength;

in Data{
	vec3 tangentLocalspace;
	vec3 bitangentLocalspace;
	vec3 normalLocalspace;
} FSIn;

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oPosition;
layout(location = 3) out vec4 oSpecular;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

vec3 newpos = vec3(0.0);

in vec3 aPos;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    /*float height =  texture(uDepthMap, texCoords).r;
    vec2 p = viewDir.xy/viewDir.z * (height*0.1);
	//return vec2(height);
    return texCoords - p;    */

	// number of depth layers
    const float numLayers = 20;
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy/viewDir.z * depthStrength; 
    vec2 deltaTexCoords = P / numLayers;

	vec2  currentTexCoords     = texCoords;
	float currentDepthMapValue = texture(uDepthMap, currentTexCoords).r;
  
	while(currentLayerDepth < currentDepthMapValue)
	{
		// shift texture coordinates along direction of P
		currentTexCoords -= deltaTexCoords;
		// get depthmap value at current texture coordinates
		currentDepthMapValue = texture(uDepthMap, currentTexCoords).r;  
		// get depth of next layer
		currentLayerDepth += layerDepth;  
	}

	  
	// get texture coordinates before collision (reverse operations)
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

	// get depth after and before collision for linear interpolation
	float afterDepth  = currentDepthMapValue - currentLayerDepth;
	float beforeDepth = texture(uDepthMap, prevTexCoords).r - currentLayerDepth + layerDepth;
 
	// interpolation of texture coordinates
	float weight = afterDepth / (afterDepth - beforeDepth);
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	//texture(uDepthMap, finalTexCoords).r

	vec3 normview = normalize(uCameraPosition - vPosition);

	float d = (texture(uDepthMap, finalTexCoords).r) / (normview.y);

	//newpos = normview*d*2.0;
	newpos = normview*d*depthStrength*40;

//	depthmodifier = length(newpos);

	return finalTexCoords;

	//return P;
} 


void main()
{

	vec3 T = normalize(FSIn.tangentLocalspace);
	vec3 B = normalize(FSIn.bitangentLocalspace);
	vec3 N = normalize(FSIn.normalLocalspace);
	mat3 TBN = mat3(T,B,N);
	
	vec3 tanvposition = vPosition*TBN;
	vec3 tanviewposition = uCameraPosition*TBN;
	vec3 depthViewDir = normalize(tanviewposition-tanvposition);
	vec2 newtexCoords = vec2(0.0);
	if(depthMapExists == 1)
	{
		newtexCoords = ParallaxMapping(vTexCoord,depthViewDir);
	}
	else
	{
		newtexCoords = vTexCoord;
	}	

	vec3 tangentSpaceNormal = texture(uNormalMap, newtexCoords).xyz * 2.0 - vec3(1.0);
	vec3 localSpaceNormal = TBN*tangentSpaceNormal;
	vec3 viewSpaceNormal = normalize(uWorldViewProjectionMatrix* vec4(localSpaceNormal,0.0)).xyz;
	

	oColor = texture(uTexture, newtexCoords);
	//oColor = vec4(1.0f,1.0f,1.0f,1.0f);


	vec3 norm = vec3(0.0);
	if(normalMapExists == 1)
		norm = normalize(viewSpaceNormal);
	else
		norm = normalize(vNormal);

	vec4 v_clip_coord = uWorldViewProjectionMatrix * vec4(aPos-newpos, 1.0);
	float f_ndc_depth = v_clip_coord.z / v_clip_coord.w;
	gl_FragDepth = (1.0 - 0.0) * 0.5 * f_ndc_depth + (1.0 + 0.0) * 0.5;

	oPosition = vec4(vPosition+newpos,1.0f);
	oNormals = vec4(norm,1.0f);

	float realspecular = 0.0;
	if(specularMapExists == 1)
	{
		realspecular = texture(uSpecularMap,newtexCoords).r;
	}
	else
	{
		realspecular = specular;
	}

	oSpecular = vec4(vec3(realspecular),1.0f);
}

#endif
#endif
