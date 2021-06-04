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