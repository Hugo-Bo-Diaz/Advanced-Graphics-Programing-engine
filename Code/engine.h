//
// engine.h: This file contains the types and functions relative to the engine.
//
#pragma once

#define BINDING(b) b

#define MAXTEXTURES 1000

#include "platform.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <list>
#include <vector>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct info
{
	std::string OpenGLversion;
	std::string OpenGLrenderer;
	std::string OpenGLvendor;
	std::string OpenGLSLverion;
	std::list<std::string>OpenGLextensions;
};

struct Buffer
{
	GLuint handle;
	GLenum type;
	u32 size;
	u32 head;
	void* data;

};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct VertexShaderAttribute
{
	u8 location;
	u8 componentCount;
};

struct VertexShaderLayout
{
	std::vector<VertexShaderAttribute>attributes;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
	VertexShaderLayout vertexInputLayout;
};

struct Material
{
	std::string name;
	vec3 albedo;
	vec3 emissive;
	f32 smoothness;
	u32 albedoTextureIdx;
	bool hasalbedo = false;
	u32 emissiveTextureIdx;
	bool hasemissive = false;
	u32 specularTextureIdx;
	bool hasspecular = false;
	u32 normalsTextureIdx;
	bool hasnormals = false;
	u32 bumpTextureIdx;
	bool hasbump = false;

	float bumpStrength = 0.05f;
	float normalsStrength = 1.0f;

	f32 specular;
};

struct Model
{
	u32 meshIdx;
	std::vector<u32> materialIdx;

	u32 localParamsOffset;
	u32 localParamsSize;
	glm::mat4 world = {};

	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;

	std::string name;

	//Buffer localBuffer;
};

//STRUCTS TO ORGANIZE VBO EBO SHADER VAO/////////////////
struct VertexBufferAttribute
{
	u8 location;
	u8 componentCount;
	u8 offset;
};

struct VertexBufferLayout
{
	std::vector<VertexBufferAttribute> attributes;
	u8 stride;
};

struct Vao
{
	GLuint handle;
	GLuint programHandle;
};


//////////////////////////////////////////////////////////

struct Submesh
{
	VertexBufferLayout vertexBufferLayout;
	std::vector<float> vertices;
	std::vector<u32> indices;
	u32 vertexOffset;
	u32 indexOffset;
	std::vector<Vao>vao_list;

	std::string name;
};

struct Mesh
{
	std::vector<Submesh> submeshes;
	GLuint vertexBufferHandle;
	GLuint indexBufferHandle;

	std::string name;
};


enum Mode//WHAT IS SHOWN
{
	Mode_TexturedQuad,

	Mode_AlbedoModel,
	Mode_Normals,
	Mode_Position,
	Mode_Specular,

	Mode_FinalRender,
	Mode_Deferred,

	//these two were used for easy debugging of the water effects
	Mode_ReflectionWater,
	Mode_RefractionWater,

    Mode_Count
};

enum RenderMode//WHAT IS CALCULATED
{
	RenderMode_Forward,
	RenderMode_Deferred,
	RenderMode_Count
};

struct Camera
{

	glm::vec3 position = glm::vec3(0,0,0);
	glm::vec3 target = glm::vec3(0,0,-1);
	glm::vec3 direction;

	float pitch=0;
	float yaw=0;

	bool orbital = false;
	float orbital_distance = 5;

	glm::f32 znear = 0.1f;
	glm::f32 zfar = 1000.0f;
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 ortho;

	glm::vec3 cameraRight;
	glm::vec3 cameraUp;
	glm::vec3 cameraFront;
};

glm::mat4 TransformScale(const vec3& scaleFactors);
glm::mat4 TransformPosition(const vec3& pos);

enum LightType
{
	LightType_Directional,
	LightType_Point,
	LightType_Ambient
};

struct Light
{
	LightType type;
	vec3 color;
	vec3 direction;
	vec3 position;

	float Kconstant = 1.0;
	float Klinear = 0.09;
	float Kquadratic = 0.032;
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    //std::vector<Texture>  textures;
    //std::vector<Program>  programs;

    // program indices
	u32 forwardRenderProgramIdx;

    u32 mapCalculationProgramIdx;
	u32 deferredRenderProgramIdx;

	u32 waterRenderProgramIdx;
	u32 waterPlaneProgramIdx;

	GLint maxUniformBufferSize;
	GLint uniformBlockAlignment;

	GLuint bufferHandle;

	u32 LocalParamsOffset;
	u32 LocalParamsSize;
	Buffer LocalAttBuffer;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

	std::vector<Texture> textures;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<Model> models;
	std::vector<Program> programs;

	int model = 0;
	int mesh = 0;
	u32 texturedMeshProgram_uTexture;
    // Mode
    Mode mode = Mode_FinalRender;
	RenderMode rendermode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;

	u32 globalParamsOffset;
	u32 globalParamsSize;
	Buffer cbuffer;

	u32 LightTransformParamsOffset;
	u32 LightTransformParamsSize;
	Buffer LightTransformBuffer;

	glm::mat4 lightworld;

	//lights
	std::vector<Light> lights;

	//camera
	Camera camera;
	glm::f32 aspectRatio; 
	glm::vec3 upVector = { 0.0,1.0,0.0 };
	float angle = 0;//FOR TESTING

	//worldmatrix
	glm::mat4 world = TransformPosition(vec3(0, 0, -6.0))*TransformScale(vec3(0.45));
	glm::mat4 worldViewProjection;
	
	//framebuffer;
	GLuint frameBufferHandle;//FRAME BUFFER INITIAL

	GLuint colorAttachmentHandle;
	GLuint depthAttachmentHandle;
	GLuint normalAttachmentHandle;
	GLuint positionAttachmentHandle;
	GLuint specularAttachmentHandle;
	GLuint finalAttachmentHandle;

	//
	GLuint deferredBufferHandle;

	GLuint deferredAttachmentHandle;

	//water
	GLuint ReflectionframeBuffer;
	GLuint reflectionAttachmentHandle;
	GLuint reflectiondepthAttachmentHandle;

	GLuint RefractionframeBuffer;
	GLuint refractionAttachmentHandle;
	GLuint refractiondepthAttachmentHandle;

	bool render_water = true;

	GLuint waterviewmatloc;
	GLuint waterprojmatloc;

	u32 waternormalMapIdx;
	u32 waterdudvMapIdx;

	//info
	info* OpenGLinfo;


	//CHECK WINDOW SIZE
	float prevwinx;
	float prevwiny;

	GLint screen[2];

	GLuint spherevao;
	GLuint embeddedsphereVertices;
	GLuint embeddedsphereElements;

	int spherebuffernumindices;

	GLuint waterplanevao;
	GLuint embeddedwaterplaneVertices;
	GLuint embeddedwaterplaneElements;

	bool isrenderonfocus = false;

	u32 LightParamsParamsOffset;
	u32 LightParamsParamsSize;
	Buffer LightParamsBuffer;

	GLuint KlLocdeferred;
	GLuint KqLocdeferred;
};

void Init(App* app);

u32 LoadTexture2D(App* app, const char* filepath);
Image LoadImage(const char* filename);
void AddLight(LightType type, vec3 color, vec3 direction, vec3 position, App* app);

void Gui(App* app);
void OpenGLWindowData(App* app);
void ModelWindowGUI(App* app);
void LightWindowGUI(App* app);

void Update(App* app);
void GenerateBuffers(App* app);
void GenerateWaterBuffers(App* app);

void ChangeWindowSize(float x, float y, App* app);
//glm::mat4 TransformRotation(const vec3& rotation);

void Render(App* app);
GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

void passWaterScene(Camera* cam, GLenum colorAttachment, bool reflection, App* app);

GLuint AddSphere(App* app);

GLuint AddWaterPlane(App* app);

//u32 AddPlane(App* app);
/*

Material* AddMaterial(App* app, const char);
void AssignMaterial(App* app, Submesh* model);
void AddAlbedo(Material* target, const char* texture);
void AddSpecular(Material* target, const char* texture);
void AddNormalMap(Material* target, const char* texture);
void AddDisplacementMap(Material* target, const char* texture);*/

void RecalculateMatrix(Model* model);
void ChangePos(Model* model, float x, float y, float z);
void ChangeScl(Model* model, float x, float y, float z);
void ChangeRot(Model* model, float x, float y, float z);

