#ifdef FORWARD_SHADING_RENDER

struct Light
{
unsigned int type;
vec3 color;
vec3 direction;
vec3 position;
};

struct LightConstants
{
float Klinear;
float Kquadratic;
float useless;
float uselesss;
};

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

layout(location=3) in vec3 tangent;
layout(location=4) in vec3 bitangent;

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

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;

out Data
{
	vec3 tangentLocalspace;
	vec3 bitangentLocalspace;
	vec3 normalLocalspace;
} VSOut;
out vec3 aPos;

void main()
{
	
	VSOut.tangentLocalspace = tangent;
	VSOut.bitangentLocalspace = bitangent;
	VSOut.normalLocalspace = aNormal;

	vTexCoord = aTexCoord;

	vPosition = vec3( uWorldMatrix * vec4(aPosition,1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal,0.0));
	vViewDir = uCameraPosition - vPosition;

	aPos = aPosition;

	/*float clippingScale = 5.0;
	gl_Position = vec4(aPosition, clippingScale);
	gl_Position.z = -gl_Position.z;*/

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform sampler2D uDepthMap;

uniform float specular;

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

layout(binding = 3, std140) uniform LightParamsSecond
{
	LightConstants uConstants[16];
};

float depthmodifier = 0.0;
uniform float depthStrength;
vec3 newpos;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 

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


	depthmodifier = length(newpos);
	depthmodifier = 0.01;

	return finalTexCoords;

	//return P;
} 

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oPosition;
layout(location = 3) out vec4 oSpecular;
layout(location = 4) out vec4 oLightResult;

in Data{
	vec3 tangentLocalspace;
	vec3 bitangentLocalspace;
	vec3 normalLocalspace;
} FSIn;


uniform int normalMapExists;
uniform int depthMapExists;

uniform mat4 cameraProj;

in vec3 aPos;

layout (depth_any) out float gl_FragDepth;

void main()
{


	vec3 T = normalize(FSIn.tangentLocalspace);
	vec3 B = normalize(FSIn.bitangentLocalspace);
	vec3 N = normalize(FSIn.normalLocalspace);
	mat3 TBN = mat3(T,B,N);

	vec3 viewDir = normalize(uCameraPosition-vPosition);
	
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
	

	//oColor = vec4(FSIn.tangentLocalspace,1.0f);

	vec3 norm = vec3(0.0);
	if(normalMapExists == 1)
		norm = normalize(viewSpaceNormal);
	else
		norm = normalize(vNormal);

	oPosition = vec4(vPosition,1.0f);
	oNormals = vec4(norm,1.0f);
	oSpecular = vec4(vec3(specular),1.0f);



	vec3 color = vec3(0.0);
	
	for ( int i = 0; i< uLightCount; ++i)
	{

		vec3 lightDir = vec3(0.0);
	
		if(uLight[i].type == 0)
		{
			lightDir = normalize(uLight[i].direction);
		}
		else if(uLight[i].type == 1)
		{
			lightDir = normalize(uLight[i].position - vPosition);
		}
		else if(uLight[i].type == 2)
		{
			color+= texture(uTexture, newtexCoords).xyz*0.1*uLight[i].color;
		}


		float Kconstant = 1.0;
		float Klinear = uConstants[i].Klinear;// 0.7;
		float Kquadratic = uConstants[i].Kquadratic;// 1.8;
		float distance = length(uLight[i].position - vPosition);

		
		// diffuse
		vec3 diffuse = max(dot(norm, lightDir), 0.0) * texture(uTexture, newtexCoords).xyz * uLight[i].color;
		// specular
		vec3 halfwayDir = normalize(lightDir + viewDir);  
		float spec = pow(max(dot(norm, halfwayDir), 0.0), 16.0);
		vec3 specular = uLight[i].color * spec * specular;
		// attenuation
		float attenuation = 1.0;

		if(uLight[i].type != 0)
			attenuation = 1.0 / (1.0 + Klinear * distance + Kquadratic * distance * distance);

		diffuse *= attenuation;
		specular *= attenuation;
		
		if(uLight[i].type != 2)
			color += diffuse + specular;

	};
	
	//float d = uCameraPosition-vPosition;
	//gl_FragDepth = (pow(2,24) -1.0 ) * ((1000.0 + 0.1)/(2.0 * (1000.0 - 0.1) + (1.0 / gl_FragCoord.z + depthmodifier) * (-1000.0 * 0.1) / (1000.0 - 0.1) + 1/2));
	
	//float ndc = (gl_FragCoord.z + depthmodifier);
	//gl_FragDepth =((1/ndc - 1/0.1) / (1/1000.0 - 1/0.1));


	vec4 v_clip_coord = uWorldViewProjectionMatrix * vec4(aPos-newpos, 1.0);
	float f_ndc_depth = v_clip_coord.z / v_clip_coord.w;
	gl_FragDepth = (1.0 - 0.0) * 0.5 * f_ndc_depth + (1.0 + 0.0) * 0.5;

	/*vec4 ndc = (cameraProj * vec4(vViewDir,0.0) * vec4(newpos, 0.));
	float newdepth =ndc.z/ndc.w;
	if(newdepth != 0.0)
		gl_FragDepth += newdepth;*/

    oColor = vec4(newpos, 1.0);

	//gl_FragDepth =gl_FragCoord.z + depthmodifier;

	//gl_FragDepth =(0.1*(gl_FragCoord.z + depthmodifier)+1000.0 )/ (gl_FragCoord.z + depthmodifier);

	//gl_FragDepth =((gl_FragCoord.z + depthmodifier)*(far+near)+2*far*near)/(gl_FragCoord.z + depthmodifier)*(far-near);

	oColor =vec4(color,1.0f);
	//oColor =vec4(depthmodifier,0.0,0.0,1.0f);
	//oColor =vec4(vec3(d),1.0f);

}

#endif
#endif
