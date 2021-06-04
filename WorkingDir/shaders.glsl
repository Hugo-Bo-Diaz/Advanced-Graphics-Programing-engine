///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

/*
#ifdef TEXTURED_GEOMETRY

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vTexCoord = aTexCoord;
	gl_Position = vec4(aPosition, 1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
}

#endif
#endif
*/

// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.


#ifdef SHOW_TEXTURED_MESH

#if defined(VERTEX) ///////////////////////////////////////////////////

layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoord;

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;

layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;
};

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3( uWorldMatrix*vec4(aPosition,1.0));
	vNormal = vec3(uWorldMatrix* vec4(aNormal,0.0));

	float clippingScale = 5.0;
	gl_Position = vec4(aPosition, clippingScale);
	gl_Position.z = -gl_Position.z;

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition,1.0);
}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location = 0) out vec4 oColor;

void main()
{
	oColor = texture(uTexture, vTexCoord);
	//oColor = vec4(1.0f,1.0f,1.0f,1.0f);
}

#endif
#endif

///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////

#ifdef SHOW_ILUMINATED_MESH

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

void main()
{
	vTexCoord = aTexCoord;

	vPosition = vec3( uWorldMatrix * vec4(aPosition,1.0));
	vNormal = vec3(uWorldMatrix * vec4(aNormal,0.0));
	vViewDir = uCameraPosition - vPosition;

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

uniform float specular;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 3, std140) uniform LightParamsSecond
{
	LightConstants uConstants[16];
};

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oNormals;
layout(location = 2) out vec4 oPosition;
layout(location = 3) out vec4 oSpecular;
layout(location = 4) out vec4 oLightResult;

void main()
{
	oColor = texture(uTexture, vTexCoord);
	//oColor = vec4(1.0f,1.0f,1.0f,1.0f);

	vec3 norm = normalize(vNormal);
	oPosition = vec4(vPosition,1.0f);
	oNormals = vec4(norm,1.0f);
	oSpecular = vec4(vec3(specular),1.0f);

	vec3 viewDir = normalize(uCameraPosition-vPosition);

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
			color+= texture(uTexture, vTexCoord).xyz*0.1*uLight[i].color;
		}


		float Kconstant = 1.0;
		float Klinear = uConstants[i].Klinear;// 0.7;
		float Kquadratic = uConstants[i].Kquadratic;// 1.8;
		float distance = length(uLight[i].position - vPosition);

		
		// diffuse
		vec3 diffuse = max(dot(norm, lightDir), 0.0) * texture(uTexture, vTexCoord).xyz * uLight[i].color;
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
	
	oLightResult =vec4(color,1.0f);
	
}

#endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEFERRED_SHADING_RENDER

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
layout(location=1) in vec2 aTexCoord;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};
layout(binding = 2, std140) uniform LightParams
{
	mat4 model;
};
out vec2 vTexCoord;

//uniform mat4 model;

uniform int current_light;

void main()
{
	vTexCoord = aTexCoord;
	if(uLight[current_light].type ==0)
		gl_Position = vec4(aPosition, 1.0);
	else if(uLight[current_light].type ==1)
		gl_Position =  model * vec4(aPosition,1.0);
	else if( uLight[current_light].type ==2)
		gl_Position = vec4(aPosition, 1.0);

}

#elif defined(FRAGMENT) ///////////////////////////////////////////////

uniform sampler2D uAlbedo;
uniform sampler2D uNormal;
uniform sampler2D uPosition;
uniform sampler2D uSpecular;

layout(binding = 0, std140) uniform GlobalParams
{
	vec3 uCameraPosition;
	unsigned int uLightCount;
	Light uLight[16];
};

layout(binding = 3, std140) uniform LightParamsSecond
{
	LightConstants uConstants[16];
};

layout(location = 0) out vec4 oColor;

in vec2 vTexCoord;

uniform int current_light;

void main()
{
	
	vec2 uv = gl_FragCoord.xy/vec2(textureSize(uAlbedo,0));

	vec3 FragPos = texture(uPosition, uv).rgb;
	vec3 Normal = texture(uNormal, uv).rgb;
	vec3 Albedo = texture(uAlbedo, uv).rgb;
	float Specular = texture(uSpecular, uv).r;

	vec3 lighting = vec3(0.0);
	vec3 viewDir = normalize(uCameraPosition-FragPos);

	//for(int i=0;i<uLightCount; ++i)

	int i = current_light;

	float Kconstant = 1.0;
	//float Klinear = uConstants[i].Klinear;// 0.7;
	//float Kquadratic = uConstants[i].Kquadratic;// 1.8;
	float Klinear = uConstants[i].Klinear;// 0.7;
	float Kquadratic = uConstants[i].Kquadratic;// 1.8;
	float distance = length(uLight[i].position - FragPos);

	vec3 lightDir = vec3(0.0);

	if(uLight[i].type == 0)
	{
		lightDir = normalize(uLight[i].direction);
	}
	else
	{
		lightDir = normalize(uLight[i].position - FragPos);
	}

    // diffuse
    vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Albedo * uLight[i].color;
    // specular
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
    vec3 specular = uLight[i].color * spec * Specular;
    // attenuation
	float attenuation = 1.0;

	if(uLight[i].type != 0)
	    attenuation = 1.0 / (1.0 + Klinear * distance + Kquadratic * distance * distance);

    diffuse *= attenuation;
    specular *= attenuation;
    lighting += diffuse + specular;

	if(texture(uAlbedo, uv).a != 0)
	{
		oColor = vec4(lighting,1.0);
		if(uLight[i].type == 2)
		{
			oColor = vec4(Albedo*0.1*uLight[i].color, 1.0);
		}
	}

	//oColor = vec4(uv.x,uv.y,0.0,1.0);
	//oColor = texture(uNormal, vTexCoord);
	//
}

#endif
#endif