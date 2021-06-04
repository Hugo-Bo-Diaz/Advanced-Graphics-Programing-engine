//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include "assimp_model_loading.h"
#include "buffer_management.h"


GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    
	int attributeCount;
	glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

	GLchar attName[256];
	GLsizei attNameLength;
	GLint attSize;
	GLenum attType;

	for (size_t i = 0; i < attributeCount; i++)
	{
		VertexShaderAttribute v = {};
		glGetActiveAttrib(program.handle, i, 256, &attNameLength, &attSize, &attType, attName);
	
		u8 attribute_location = glGetAttribLocation(program.handle, attName);

		program.vertexInputLayout.attributes.push_back(v);
	}

	app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void AddLight(LightType type, vec3 color, vec3 direction, vec3 position,App* app)
{
	Light l;
	l.type = type;
	l.color = color;
	l.direction = direction;
	l.position = position;
	app->lights.push_back(l);

}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

void Init(App* app)
{
	app->OpenGLinfo = new info();

	app->OpenGLinfo->OpenGLversion = (char*)glGetString(GL_VERSION);
	app->OpenGLinfo->OpenGLrenderer = (char*)glGetString(GL_RENDERER);
	app->OpenGLinfo->OpenGLvendor = (char*)glGetString(GL_VENDOR);
	app->OpenGLinfo->OpenGLSLverion = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

	app->camera.cameraFront = glm::normalize(app->camera.position - app->camera.target);
	app->camera.cameraRight = glm::normalize(glm::cross(app->upVector, app->camera.cameraFront));
	app->camera.cameraUp = glm::cross(app->camera.cameraFront, app->camera.cameraRight);

	GLint num_extensions;
	glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
	for (int i = 0; i < num_extensions; ++i)
	{
		app->OpenGLinfo->OpenGLextensions.push_back((char*)glGetStringi(GL_EXTENSIONS, GLuint(i)));
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	struct VertexV3V2 {
		glm::vec3 pos;
		glm::vec2 uv;
	};
	/*
	const VertexV3V2 vertices[] = {
	{ glm::vec3(-0.5,-0.5,0.0), glm::vec2(0.0,0.0) },
	{ glm::vec3(0.5,-0.5,0.0), glm::vec2(1.0,0.0) },
	{ glm::vec3(0.5,0.5,0.0), glm::vec2(1.0,1.0) },
	{ glm::vec3(-0.5,0.5,0.0), glm::vec2(0.0,1.0) }
	};*/

	const VertexV3V2 vertices[] = {
	{ glm::vec3(-1,-1,0.0), glm::vec2(0.0,0.0) },
	{ glm::vec3(1,-1,0.0), glm::vec2(1.0,0.0) },
	{ glm::vec3(1,1,0.0), glm::vec2(1.0,1.0) },
	{ glm::vec3(-1,1,0.0), glm::vec2(0.0,1.0) }
	};

	const u16 indices[] = {
		0,1,2,0,2,3
	};
	
	glGenBuffers(1, &app->embeddedVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &app->embeddedElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &app->vao);
	glBindVertexArray(app->vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBindVertexArray(0);
	
	AddSphere(app);
	AddWaterPlane(app);

	GenerateBuffers(app);

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
	glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);
	/*
	int pat1 = LoadModel(app, "Patrick/Patrick.obj");
	//app->models[pat1].world = TransformPosition(glm::vec3(2, 0,-1))*TransformScale(glm::vec3(0.45));
	ChangePos(&app->models[pat1], 2, 0, -1);
	ChangeScl(&app->models[pat1], 0.45, 0.45, 0.45);
	ChangeRot(&app->models[pat1], 0, -90, 0);
	RecalculateMatrix(&app->models[pat1]);*/
	
	int pat2 = LoadModel(app, "Patrick/Patrick.obj");
	ChangePos(&app->models[pat2], -1, 0, 2);
	ChangeScl(&app->models[pat2], 0.45, 0.45, 0.45);
	ChangeRot(&app->models[pat2], 0, -90, 0);
	RecalculateMatrix(&app->models[pat2]);

	/*int floor = LoadModel(app, "StoneFloor/StoneFloor.obj");
	ChangeScl(&app->models[floor],0.5, 0.5, 0.5);
	RecalculateMatrix(&app->models[floor]);*/

	int toy = LoadModel(app, "TOYBOX/ToyBox.obj");
	ChangePos(&app->models[toy], 0, 1, 0);
	ChangeScl(&app->models[toy], 0.2, 0.2, 0.2);
	RecalculateMatrix(&app->models[toy]);
	/*app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
	
	Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
	*/
	//app->programUniformTexture = glGetUniformLocation(.handle, "uTexture");
	

	glGenBuffers(1, &app->bufferHandle);
	glBindBuffer(GL_UNIFORM_BUFFER, app->bufferHandle);
	glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	app->cbuffer = CreateBuffer(app->maxUniformBufferSize,GL_UNIFORM_BUFFER,GL_STREAM_DRAW);
	app->LocalAttBuffer = CreateBuffer(app->maxUniformBufferSize, GL_UNIFORM_BUFFER, GL_STREAM_DRAW);
	app->LightTransformBuffer = CreateBuffer(sizeof(glm::mat4), GL_UNIFORM_BUFFER, GL_STREAM_DRAW);
	app->LightParamsBuffer = CreateBuffer(app->maxUniformBufferSize, GL_UNIFORM_BUFFER, GL_STREAM_DRAW);


	app->diceTexIdx = LoadTexture2D(app, "dice.png");
	app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
	app->blackTexIdx = LoadTexture2D(app, "color_black.png");
	app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
	app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
	
	app->waternormalMapIdx = LoadTexture2D(app, "watermaps/normalmap.png");
	glBindTexture(GL_TEXTURE_2D, app->textures[app->waternormalMapIdx].handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	app->waterdudvMapIdx = LoadTexture2D(app, "watermaps/dudvmap.png");
	glBindTexture(GL_TEXTURE_2D, app->textures[app->waterdudvMapIdx].handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);

	app->forwardRenderProgramIdx = LoadProgram(app, "forward_shading.glsl", "FORWARD_SHADING_RENDER");
	Program& forwardRenderProgramIdx = app->programs[app->forwardRenderProgramIdx];
	forwardRenderProgramIdx.vertexInputLayout.attributes.push_back({ 0,3 });
	forwardRenderProgramIdx.vertexInputLayout.attributes.push_back({ 2,2 });
	forwardRenderProgramIdx.vertexInputLayout.attributes.push_back({ 1,3 });
	forwardRenderProgramIdx.vertexInputLayout.attributes.push_back({ 3,3 });
	forwardRenderProgramIdx.vertexInputLayout.attributes.push_back({ 4,3 });

	app->mapCalculationProgramIdx = LoadProgram(app, "map_calculation.glsl", "MAP_CALCULATION_SHADER");
	Program& mapCalculationProgramIdx = app->programs[app->mapCalculationProgramIdx];
	mapCalculationProgramIdx.vertexInputLayout.attributes.push_back({ 0,3 });
	mapCalculationProgramIdx.vertexInputLayout.attributes.push_back({ 2,2 });
	mapCalculationProgramIdx.vertexInputLayout.attributes.push_back({ 1,3 });
	mapCalculationProgramIdx.vertexInputLayout.attributes.push_back({ 3,3 });
	mapCalculationProgramIdx.vertexInputLayout.attributes.push_back({ 4,3 });
	//GLuint KlLocforward = glGetUniformLocation(texturedMeshProgramIdx.handle,"Kl");
	//GLuint KqLocforward = glGetUniformLocation(texturedMeshProgramIdx.handle, "Kq");

	app->deferredRenderProgramIdx = LoadProgram(app, "deferred.glsl", "DEFERRED_SHADING_RENDER");
	Program& deferredRenderProgramIdx = app->programs[app->deferredRenderProgramIdx];
	deferredRenderProgramIdx.vertexInputLayout.attributes.push_back({ 0,3 });
	deferredRenderProgramIdx.vertexInputLayout.attributes.push_back({ 1,2 });


	app->waterRenderProgramIdx = LoadProgram(app, "water_render.glsl", "WATER_REFL_REFR_RENDER_SHADER");
	Program& waterRenderProgramIdx = app->programs[app->waterRenderProgramIdx];
	waterRenderProgramIdx.vertexInputLayout.attributes.push_back({ 0,3 });
	waterRenderProgramIdx.vertexInputLayout.attributes.push_back({ 2,2 });
	waterRenderProgramIdx.vertexInputLayout.attributes.push_back({ 1,3 });


	app->waterPlaneProgramIdx = LoadProgram(app, "water_plane.glsl", "WATER_PLANE_RENDER_SHADER");
	Program& waterPlaneProgramIdx = app->programs[app->waterPlaneProgramIdx];
	waterPlaneProgramIdx.vertexInputLayout.attributes.push_back({ 0,3 });
	//waterRenderProgramIdx.vertexInputLayout.attributes.push_back({ 2,2 });
	waterPlaneProgramIdx.vertexInputLayout.attributes.push_back({ 1,3 });

	//GLuint KlLocdeferred = glGetUniformLocation(deferredRenderProgramIdx.handle, "Kl");
	//GLuint KqLocdeferred = glGetUniformLocation(deferredRenderProgramIdx.handle, "Kq");

	AddLight(LightType_Ambient, { 1,1,1 }, { 0,1,0 }, { 0,0,0 }, app);

	AddLight(LightType_Directional, { 1,1,1 }, { 0,1,0 }, { 0,0,-7 }, app);

	AddLight(LightType_Point, { 1,0,1 }, { 0,1,0 }, { -6,0,-5 }, app);
	AddLight(LightType_Point, { 1,1,0 }, { 0,1,0 }, { 6,0,-5 }, app);
	



	app->camera.ortho = glm::ortho(0.f, 400.f, 0.f, 400.f, -1.f, 1.f);
	app->camera.projection = glm::perspective(glm::radians(60.0f), app->aspectRatio, app->camera.znear, app->camera.zfar);
	app->camera.view = glm::lookAt(app->camera.position, app->camera.target, app->upVector);

	// TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    app->mode = Mode_TexturedQuad;
	app->mode = Mode_AlbedoModel;
	app->rendermode = RenderMode_Forward;
}

void LoadMesh(App * app, const char* path)
{

	/*Mesh mesh = {};

	VertexBufferLayout vertexBufferLayout = {};
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0,3,0 });
	vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2,2,3 * sizeof(float) });
	vertexBufferLayout.stride = 5 * sizeof(float);
	*/
}

void Gui(App* app)
{

    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);

    ImGui::End();

	ImGui::Begin("Mode Selection");
	
	static std::string curr_mode;
	curr_mode = "Current Mode: ";
	
	switch (app->mode)
	{
	case Mode_AlbedoModel:
		curr_mode += "Albedo";
		break;
	case Mode_Normals:
		curr_mode += "Normals";
		break;
	case Mode_Position:
		curr_mode += "Position";
		break;
	case Mode_Specular:
		curr_mode += "Specular";
		break;
	case Mode_Deferred:
		curr_mode += "Deferred";
		break;
	case Mode_FinalRender:
		curr_mode += "Final";
		break;
	case Mode_ReflectionWater:
		curr_mode += "Reflection";
		break;
	case Mode_RefractionWater:
		curr_mode += "Refraction";
		break;
	default:
		curr_mode += "unknown";
		break;
	}

	ImGui::Text(curr_mode.c_str());


	if (ImGui::Button("Final"))
	{
		app->mode = Mode_FinalRender;
		app->rendermode = RenderMode_Forward;
	}

	if (ImGui::Button("Deferred"))
	{
		app->mode = Mode_Deferred;
		app->rendermode = RenderMode_Deferred;
	}
	if (app->rendermode == RenderMode_Deferred)
	{
		if (ImGui::CollapsingHeader("gBuffer"))
		{
			if (ImGui::Button("Albedo"))
			{
				app->mode = Mode_AlbedoModel;
			}
			if (ImGui::Button("Normals"))
			{
				app->mode = Mode_Normals;
			}
			if (ImGui::Button("Position"))
			{
				app->mode = Mode_Position;
			}
			if (ImGui::Button("Specular"))
			{
				app->mode = Mode_Specular;
			}
		}
	}




	ImGui::End();


	ImGui::Begin("WaterFX");

	ImGui::Checkbox("active", &app->render_water);

	ImGui::End();

	ImGui::Begin("RENDER");
	glBindFramebuffer(GL_FRAMEBUFFER, app->frameBufferHandle);

	static ImVec2 cach;
	static ImVec2 reg_max;
	static ImVec2 reg_min;

	reg_max = ImGui::GetWindowContentRegionMax();
	reg_min = ImGui::GetWindowContentRegionMin();

	app->isrenderonfocus = ImGui::IsWindowFocused();

	GLuint buffer_to_render= app->colorAttachmentHandle;
	switch (app->mode)
	{
	case Mode_AlbedoModel:
		buffer_to_render = app->colorAttachmentHandle;
		break;
	case Mode_Normals:
		buffer_to_render = app->normalAttachmentHandle;
		break;
	case Mode_Position:
		buffer_to_render = app->positionAttachmentHandle;
		break;
	case Mode_Specular:
		buffer_to_render = app->specularAttachmentHandle;
		break;
	case Mode_Deferred:
		buffer_to_render = app->deferredAttachmentHandle;
		break;
	case Mode_FinalRender:
		buffer_to_render = app->finalAttachmentHandle;
		break;
	default:
		break;
	}

	cach = ImVec2(reg_max.x-reg_min.x,reg_max.y-reg_min.y);
	cach = ImGui::GetWindowSize();
	ImGui::Image((void*)buffer_to_render,cach,ImVec2(0,1),ImVec2(1,0));

	app->displaySize.x = cach.x;
	app->displaySize.y = cach.y;

	if (app->prevwinx != app->displaySize.x || app->prevwiny != app->displaySize.y)
	{
		GenerateBuffers(app);
		app->prevwinx = app->displaySize.x;
		app->prevwiny = app->displaySize.y;
	}
	//ImGui::GetWindowDrawList()->AddImage(
	//	(void*)app->colorAttachmentHandle,
	//	ImGui::GetWindowPos(),
	//	ImGui::GetWindowPos()+Im,
	//		200 + 400), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::End();

	OpenGLWindowData(app);

	ModelWindowGUI(app);

	LightWindowGUI(app);
}

void OpenGLWindowData(App* app)
{
	ImGui::Begin("OpenGL properties");
	ImGui::Text("OpenGL version:");
	ImGui::SameLine();
	ImGui::Text(app->OpenGLinfo->OpenGLversion.c_str());
	ImGui::Text("OpenGL renderer:");
	ImGui::SameLine();
	ImGui::Text(app->OpenGLinfo->OpenGLrenderer.c_str());
	ImGui::Text("OpenGL vendor:");
	ImGui::SameLine();
	ImGui::Text(app->OpenGLinfo->OpenGLvendor.c_str());
	ImGui::Text("OpenGLSL version:");
	ImGui::SameLine();
	ImGui::Text(app->OpenGLinfo->OpenGLSLverion.c_str());

	if (ImGui::CollapsingHeader("OpenGL extensions:"))
	{
		for (std::list<std::string>::iterator it = app->OpenGLinfo->OpenGLextensions.begin(); it != app->OpenGLinfo->OpenGLextensions.end(); it++)
		{
			ImGui::TextWrapped((*it).c_str());
		}

		
	}
	ImGui::End();
}

void ModelWindowGUI(App * app)
{
	ImGui::Begin("Model list");

	for (u32 i = 0; i < app->models.size(); ++i)
	{
		ImGui::PushID(i);
		Model& model = app->models[i];
		Mesh& mesh = app->meshes[model.meshIdx];

		std::string s = model.name + " " + std::to_string(i);

		if (ImGui::CollapsingHeader(s.c_str()))
		{
			glm::vec3 pos = model.position;
			glm::vec3 rot = model.rotation;
			glm::vec3 scl = model.scale;

			//show transform
			ImGui::DragFloat3("position", &pos[0],0.05);
			ImGui::DragFloat3("rotation", &rot[0],0.01,0, PI*2);
			ImGui::DragFloat3("scale", &scl[0],0.01,0.2,5);

			if (pos != model.position || rot != model.rotation || scl != model.scale)
			{
				model.position = pos;
				model.rotation = rot;
				model.scale = scl;
				RecalculateMatrix(&model);
			}

			//show submeshes
			std::string d = "submeshes";// + std::to_string(i)
			if(ImGui::TreeNode(d.c_str()))
			{
				for (u32 j = 0; j < mesh.submeshes.size(); ++j)
				{
					ImGui::PushID(j);

					//Imgui collapsing header  with name :)
					ImGui::Text(mesh.submeshes[j].name.c_str());

					int imagessize = 150;
					ImVec2 cach = ImVec2(imagessize, imagessize);

					if (ImGui::TreeNode("Materials"))
					{
						ImGui::PushID(mesh.submeshes[j].name.c_str());

						u32 submeshMaterialIdx = model.materialIdx[j];
						Material& submeshMaterial = app->materials[submeshMaterialIdx];

						if (submeshMaterial.hasalbedo)
						{

							ImGui::Text("Albedo");
							ImGui::Image((void*)app->textures[submeshMaterial.albedoTextureIdx].handle, cach, ImVec2(0, 1), ImVec2(1, 0));
						
						}

						if (submeshMaterial.hasbump)
						{
							ImGui::PushID("bmp");

							ImGui::Text("Height");
							ImGui::DragFloat("strength", &submeshMaterial.bumpStrength, 0.005, 0.0, 0.05);
							ImGui::Image((void*)app->textures[submeshMaterial.bumpTextureIdx].handle, cach, ImVec2(0, 1), ImVec2(1, 0));
							ImGui::PopID();

						}

						if (submeshMaterial.hasnormals)
						{
							ImGui::PushID("nrm");
							ImGui::Text("Normals");
							ImGui::DragFloat("strength", &submeshMaterial.normalsStrength, 0.005, 0.0, 1.0);
							ImGui::Image((void*)app->textures[submeshMaterial.normalsTextureIdx].handle, cach, ImVec2(0, 1), ImVec2(1, 0));
							ImGui::PopID();

						}

						if (submeshMaterial.hasspecular)
						{
							ImGui::Text("Specular");
							ImGui::Image((void*)app->textures[submeshMaterial.specularTextureIdx].handle, cach, ImVec2(0, 1), ImVec2(1, 0));
						}
						ImGui::PopID();

						ImGui::TreePop();
					}

					ImGui::PopID();

				}
				ImGui::TreePop();
			}
		}
			ImGui::PopID();

	}

	ImGui::End();
}

void LightWindowGUI(App * app)
{
	ImGui::Begin("Lights list");

	for (u32 i = 0; i < app->lights.size(); ++i)
	{
		Light& light = app->lights[i];

		std::string s = "Light: " + std::to_string(i);
		if (ImGui::CollapsingHeader(s.c_str()))
		{
			ImGui::PushID(s.c_str());
			switch (light.type)
			{
				case LightType_Ambient:
				{
					ImGui::Text("Ambient light");
					ImGui::ColorEdit3("color", &light.color[0], ImGuiColorEditFlags_Uint8);

					ImGui::DragFloat("Kl", &light.Klinear, 0.005, 0.0014, 0.7);
					ImGui::DragFloat("Kq", &light.Kquadratic, 0.005, 0.0007, 1.8);
				}
				break;
				case LightType_Directional:
				{
					ImGui::Text("Directional light");
					ImGui::DragFloat3("direction", &light.direction[0], 0.05, 0, 1);
					ImGui::ColorEdit3("color", &light.color[0], ImGuiColorEditFlags_Uint8);

					ImGui::DragFloat("Kl", &light.Klinear, 0.005, 0.0014, 0.7);
					ImGui::DragFloat("Kq", &light.Kquadratic, 0.005, 0.0007, 1.8);
				}
					break;
				case LightType_Point:
				{
					ImGui::Text("Point light");
					ImGui::DragFloat3("position", &light.position[0], 0.05);
					ImGui::ColorEdit3("color", &light.color[0], ImGuiColorEditFlags_Uint8);

					ImGui::DragFloat("Kl", &light.Klinear, 0.005, 0.0014, 0.7);
					ImGui::DragFloat("Kq", &light.Kquadratic, 0.005, 0.0007, 1.8);
					break;
				}
			}
			ImGui::PopID();
		}
	}

	ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
	/*
	glBindBuffer(GL_UNIFORM_BUFFER, app->bufferHandle);

	u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
	u32 bufferHead = 0;
	for (int i = 0; i < app->models.size(); ++i)
	{
		bufferHead = Align(bufferHead, app->uniformBlockAlignment);

		app->models[i].localParamsOffset = bufferHead;

		memcpy(bufferData + bufferHead, glm::value_ptr(app->models[i].world), sizeof(glm::mat4));
		bufferHead += sizeof(glm::mat4);

		memcpy(bufferData + bufferHead, glm::value_ptr(app->worldViewProjection), sizeof(glm::mat4));
		bufferHead += sizeof(glm::mat4);

		app->models[i].localParamsSize = bufferHead - app->models[i].localParamsOffset;

	}
	glUnmapBuffer(GL_UNIFORM_BUFFER);
	glBindBuffer(GL_UNIFORM_BUFFER,0);
	*/
	glBindBuffer(GL_UNIFORM_BUFFER, app->LocalAttBuffer.handle);
	MapBuffer(app->LocalAttBuffer, GL_WRITE_ONLY);
	app->LocalParamsOffset = app->LocalAttBuffer.head;

	for (u32 i = 0; i < app->models.size(); ++i)
	{
		AlignHead(app->LocalAttBuffer, app->uniformBlockAlignment);
		
		Model& model = app->models[i];
		glm::mat4 world = model.world;
		glm::mat4 worldViewProjection = app->camera.projection*app->camera.view*world;

		model.localParamsOffset = app->LocalAttBuffer.head;
		PushMat4(app->LocalAttBuffer, world);
		PushMat4(app->LocalAttBuffer, worldViewProjection);
		model.localParamsSize = app->LocalAttBuffer.head - model.localParamsOffset;
	}

	app->LocalParamsSize = app->LocalAttBuffer.head - app->LocalParamsOffset;
	UnmapBuffer(app->LocalAttBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBuffer(GL_UNIFORM_BUFFER, app->cbuffer.handle);
	MapBuffer(app->cbuffer, GL_WRITE_ONLY);
	//handle lights
	app->globalParamsOffset = app->cbuffer.head;

	PushVec3(app->cbuffer, app->camera.position);

	PushUInt(app->cbuffer, app->lights.size());

	for (u32 i = 0; i < app->lights.size(); ++i)
	{
		AlignHead(app->cbuffer, sizeof(vec4));

		Light& light = app->lights[i];
		PushUInt(app->cbuffer, light.type);
		PushVec3(app->cbuffer, light.color);
		PushVec3(app->cbuffer, light.direction);
		PushVec3(app->cbuffer, light.position);
	}

	app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;
	UnmapBuffer(app->cbuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);



	glBindBuffer(GL_UNIFORM_BUFFER, app->LightTransformBuffer.handle);
	MapBuffer(app->LightTransformBuffer, GL_WRITE_ONLY);
	//handle lights
	app->LightTransformParamsOffset = app->LightTransformBuffer.head;

	AlignHead(app->LightTransformBuffer, app->uniformBlockAlignment);

	PushMat4(app->LightTransformBuffer, app->lightworld);

	app->LightTransformParamsSize = app->LightTransformBuffer.head - app->LightTransformParamsOffset;
	UnmapBuffer(app->LightTransformBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	glBindBuffer(GL_UNIFORM_BUFFER, app->LightParamsBuffer.handle);
	MapBuffer(app->LightParamsBuffer, GL_WRITE_ONLY);
	//handle lights
	app->LightParamsParamsOffset = app->LightParamsBuffer.head;

	for (u32 i = 0; i < app->lights.size(); ++i)
	{
		AlignHead(app->LightParamsBuffer,sizeof(float)*4);
		glm::vec4 vec;
		vec.x = app->lights[i].Klinear;
		vec.y = app->lights[i].Kquadratic;
		PushVec4(app->LightParamsBuffer, vec);
	}
	app->LightParamsParamsSize = app->LightParamsBuffer.head - app->LightParamsParamsOffset;
	UnmapBuffer(app->LightParamsBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);


	//app->camera.position = vec3(0, 0, 0);
	//app->camera.target = vec3(0, 0, -1);
	//app->camera.target.x = glm::cos(glm::radians(app->angle));
	//app->camera.target.z = glm::sin(glm::radians(app->angle));

	if (app->input.keys[K_SPACE] == BUTTON_PRESS)
	{
		app->camera.orbital = !app->camera.orbital;
	}

	if (!app->camera.orbital)
	{
		if (app->input.keys[K_W] == BUTTON_PRESSED)
		{
			app->camera.position += 0.1f * app->camera.cameraFront;
		}

		if (app->input.keys[K_S] == BUTTON_PRESSED)
		{
			app->camera.position -= 0.1f * app->camera.cameraFront;
		}

		if (app->input.keys[K_A] == BUTTON_PRESSED)
		{
			app->camera.position -= 0.1f * glm::normalize(glm::cross(app->camera.cameraFront, app->camera.cameraUp));
		}

		if (app->input.keys[K_D] == BUTTON_PRESSED)
		{
			app->camera.position += 0.1f * glm::normalize(glm::cross(app->camera.cameraFront, app->camera.cameraUp));
		}

		if (app->input.mouseButtons[LEFT] == BUTTON_PRESSED && app->isrenderonfocus)
		{
			app->camera.yaw += app->input.mouseDelta.x / 3;
			app->camera.pitch -= app->input.mouseDelta.y / 5;
		}
		app->camera.direction.x = glm::cos(glm::radians(app->camera.yaw)) * glm::cos(glm::radians(app->camera.pitch));
		app->camera.direction.y = glm::sin(glm::radians(app->camera.pitch));
		app->camera.direction.z = glm::sin(glm::radians(app->camera.yaw)) * glm::cos(glm::radians(app->camera.pitch));

		app->camera.cameraFront = glm::normalize(app->camera.direction);
		app->camera.target = app->camera.position + app->camera.cameraFront;
	}
	else
	{
		if (app->input.keys[K_W] == BUTTON_PRESSED)
		{
			app->camera.orbital_distance -= 0.1f;
		}

		if (app->input.keys[K_S] == BUTTON_PRESSED)
		{
			app->camera.orbital_distance += 0.1f;
		}

		if (app->input.mouseButtons[LEFT] == BUTTON_PRESSED && app->isrenderonfocus)
		{
			app->camera.yaw -= app->input.mouseDelta.x / 3;
			app->camera.pitch += app->input.mouseDelta.y / 5;
		}
		app->camera.target = vec3(0);

		app->camera.position.x = app->camera.orbital_distance * glm::cos(glm::radians(app->camera.yaw)) * glm::cos(glm::radians(app->camera.pitch));
		app->camera.position.y = app->camera.orbital_distance *glm::sin(glm::radians(app->camera.pitch));
		app->camera.position.z = app->camera.orbital_distance *glm::sin(glm::radians(app->camera.yaw)) * glm::cos(glm::radians(app->camera.pitch));


		app->camera.direction = app->camera.target-app->camera.position;

		app->camera.cameraFront = glm::normalize(app->camera.direction);
	}

	app->aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
	app->camera.ortho = glm::ortho(glm::radians(60.0f), app->aspectRatio, app->camera.znear, app->camera.zfar);
	app->camera.projection = glm::perspective(glm::radians(60.0f), app->aspectRatio, app->camera.znear, app->camera.zfar);
	
	app->camera.view = glm::lookAt(app->camera.position, app->camera.target, app->upVector);
	
	app->worldViewProjection = app->camera.projection* app->world*app->camera.view;
}

void GenerateBuffers(App* app)
{
	//Framebuffer
	//ALBEDO
	glGenTextures(1, &app->colorAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//NORMALS
	glGenTextures(1, &app->normalAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->normalAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//POSITION
	glGenTextures(1, &app->positionAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//SPECULAR
	glGenTextures(1, &app->specularAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->specularAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//FINAL
	glGenTextures(1, &app->finalAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->finalAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//DEPTH
	glGenTextures(1, &app->depthAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenFramebuffers(1, &app->frameBufferHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, app->frameBufferHandle);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->colorAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->positionAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->specularAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, app->finalAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

	GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (framebufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		ELOG("FRAMEBUFFER ERROR");
	}

	glDrawBuffers(1, &app->colorAttachmentHandle);
	glDrawBuffers(1, &app->normalAttachmentHandle);
	glDrawBuffers(1, &app->positionAttachmentHandle);
	glDrawBuffers(1, &app->specularAttachmentHandle);
	glDrawBuffers(1, &app->finalAttachmentHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//DEFERRED
	glGenTextures(1, &app->deferredAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->deferredAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &app->deferredBufferHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, app->deferredBufferHandle);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->deferredAttachmentHandle, 0);

	GLenum defframebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (defframebufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		ELOG("FRAMEBUFFER ERROR");
	}

	glDrawBuffers(1, &app->deferredAttachmentHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	GenerateWaterBuffers(app);

}

void GenerateWaterBuffers(App * app)
{
	//REFLECTION
	
	//COLOR
	glGenTextures(1, &app->reflectionAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->reflectionAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//DEPTH
	glGenTextures(1, &app->reflectiondepthAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->reflectiondepthAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenFramebuffers(1, &app->ReflectionframeBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, app->ReflectionframeBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->reflectionAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->reflectiondepthAttachmentHandle, 0);

	GLenum defframebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (defframebufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("FRAMEBUFFER ERROR");
	}

	glDrawBuffers(1, &app->reflectionAttachmentHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//REFRACTION

	//COLOR
	glGenTextures(1, &app->refractionAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->refractionAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, app->displaySize.x, app->displaySize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	//DEPTH
	glGenTextures(1, &app->refractiondepthAttachmentHandle);
	glBindTexture(GL_TEXTURE_2D, app->refractiondepthAttachmentHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenFramebuffers(1, &app->RefractionframeBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, app->RefractionframeBuffer);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->refractionAttachmentHandle, 0);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->refractiondepthAttachmentHandle, 0);

	GLenum refractionframebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (refractionframebufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("FRAMEBUFFER ERROR");
	}

	glDrawBuffers(1, &app->refractionAttachmentHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void ChangeWindowSize(float x, float y, App * app)
{
	app->displaySize.x = x;
	app->displaySize.y = y;
	GenerateBuffers(app);
}

void ChangeWindowSize(float x, float y)
{
}

glm::mat4 TransformScale(const vec3 & scaleFactors)
{
	glm::mat4 transform = glm::scale(scaleFactors);
	return transform;
}

glm::mat4 TransformPosition(const vec3 & pos)
{
	glm::mat4 transform = glm::translate(pos);
	return transform;
}



void Render(App* app)
{
 
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	if (app->render_water)
	{
		// :)

		glCullFace(GL_BACK);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(true);

		//fill buffer?
		float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;


		//reflection///////////////////////////////////////////
		glBindFramebuffer(GL_FRAMEBUFFER, app->ReflectionframeBuffer);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Camera reflectionCamera = app->camera;
		reflectionCamera.position.y = -reflectionCamera.position.y;
		reflectionCamera.pitch = -reflectionCamera.pitch;

		reflectionCamera.ortho = glm::ortho(glm::radians(60.0f), aspectRatio, reflectionCamera.znear, reflectionCamera.zfar);
		reflectionCamera.projection = glm::perspective(glm::radians(60.0f), aspectRatio, reflectionCamera.znear, reflectionCamera.zfar);


		reflectionCamera.direction.x = glm::cos(glm::radians(reflectionCamera.yaw)) * glm::cos(glm::radians(reflectionCamera.pitch));
		reflectionCamera.direction.y = glm::sin(glm::radians(reflectionCamera.pitch));
		reflectionCamera.direction.z = glm::sin(glm::radians(reflectionCamera.yaw)) * glm::cos(glm::radians(reflectionCamera.pitch));

		reflectionCamera.cameraFront = glm::normalize(reflectionCamera.direction);
		reflectionCamera.target = reflectionCamera.position + reflectionCamera.cameraFront;

		//reflectionCamera.direction.y = glm::sin(glm::radians(app->camera.pitch));
		//reflectionCamera.target = glm::normalize(reflectionCamera.direction) + reflectionCamera.position;

		reflectionCamera.view = glm::lookAt(reflectionCamera.position, reflectionCamera.target, app->upVector);

		glm::mat4 refl = reflectionCamera.projection* app->world*reflectionCamera.view;

		//passwater :D
		passWaterScene(&reflectionCamera, GL_COLOR_ATTACHMENT0, true, app);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//refraction////////////////////////////////////////
		glBindFramebuffer(GL_FRAMEBUFFER, app->RefractionframeBuffer);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Camera refractionCamera = app->camera;

		refractionCamera.ortho = glm::ortho(glm::radians(60.0f), aspectRatio, refractionCamera.znear, refractionCamera.zfar);
		refractionCamera.projection = glm::perspective(glm::radians(60.0f), aspectRatio, refractionCamera.znear, refractionCamera.zfar);

		refractionCamera.view = glm::lookAt(refractionCamera.position, refractionCamera.target, app->upVector);

		glm::mat4 refr = refractionCamera.projection* app->world*refractionCamera.view;

		//passwater :D


		refractionCamera.projection = glm::perspective(glm::radians(60.0f), app->aspectRatio, refractionCamera.znear, refractionCamera.zfar);
		refractionCamera.view = glm::lookAt(refractionCamera.position, refractionCamera.target, app->upVector);

		passWaterScene(&refractionCamera, GL_COLOR_ATTACHMENT0, false, app);


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}




	GLuint drawBuffersforward[] = { GL_COLOR_ATTACHMENT4 };
	Program& forwardRenderProgram = app->programs[app->forwardRenderProgramIdx];


	GLuint drawBuffersdeferred[] = { GL_COLOR_ATTACHMENT0 };
	Program& deferredRenderProgramIdx = app->programs[app->deferredRenderProgramIdx];

	switch (app->rendermode)
	{
		case RenderMode_Forward:
			//use normal render
		glBindFramebuffer(GL_FRAMEBUFFER, app->frameBufferHandle);

		//glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, app->displaySize.x, app->displaySize.y);

		//GLuint drawBuffers[] = { app->colorAttachmentHandle,app->normalAttachmentHandle,app->finalAttachmentHandle };
				
		glDrawBuffers(ARRAY_COUNT(drawBuffersforward), drawBuffersforward);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(forwardRenderProgram.handle);

		for (int i = 0; i<app->models.size();++i)
		{
			Model& model = app->models[i];
			Mesh& mesh = app->meshes[model.meshIdx];

			for (u32 j = 0; j < mesh.submeshes.size(); ++j)
			{
				GLuint vao = FindVAO(mesh, j, forwardRenderProgram);
				glBindVertexArray(vao);

				u32 submeshMaterialIdx = model.materialIdx[j];
				Material& submeshMaterial = app->materials[submeshMaterialIdx];

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
				glUniform1i(app->texturedMeshProgram_uTexture, 0);
				
				//DO SAME FOR SPECULAR
				if (submeshMaterial.normalsTextureIdx == 0)
				{
					GLint loc = glGetUniformLocation(forwardRenderProgram.handle, "normalMapExists");
					glUniform1i(loc, 0);
				}
				else
				{
					GLint loc = glGetUniformLocation(forwardRenderProgram.handle, "normalMapExists");
					glUniform1i(loc, 1);
					
					GLint loc1 = glGetUniformLocation(forwardRenderProgram.handle, "normalStrength");
					glUniform1f(loc1, submeshMaterial.normalsStrength);

					glActiveTexture(GL_TEXTURE1);
					GLint locnormals = glGetUniformLocation(forwardRenderProgram.handle, "uNormalMap");
					glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.normalsTextureIdx].handle);
					glUniform1i(locnormals, 1);
				}

				if (submeshMaterial.bumpTextureIdx == 0)
				{
					GLint loc = glGetUniformLocation(forwardRenderProgram.handle, "depthMapExists");
					glUniform1i(loc, 0);
				}
				else
				{
					GLint loc = glGetUniformLocation(forwardRenderProgram.handle, "depthMapExists");
					glUniform1i(loc, 1);

					GLint loc1 = glGetUniformLocation(forwardRenderProgram.handle, "depthStrength");
					glUniform1f(loc1, submeshMaterial.bumpStrength);

					glActiveTexture(GL_TEXTURE2);
					GLint locdepth = glGetUniformLocation(forwardRenderProgram.handle, "uDepthMap");
					glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.bumpTextureIdx].handle);
					glUniform1i(locdepth, 2);
				}

				if (submeshMaterial.specularTextureIdx == 0)
				{
					GLint loc = glGetUniformLocation(forwardRenderProgram.handle, "specularMapExists");
					glUniform1i(loc, 0);
				}
				else
				{
					GLint loc = glGetUniformLocation(forwardRenderProgram.handle, "specularMapExists");
					glUniform1i(loc, 1);

					glActiveTexture(GL_TEXTURE2);
					GLint locspec = glGetUniformLocation(forwardRenderProgram.handle, "uSpecularMap");
					glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.specularTextureIdx].handle);
					glUniform1i(locspec, 2);
				}


				GLint locproj = glGetUniformLocation(forwardRenderProgram.handle, "cameraProj");
				glUniformMatrix4fv(locproj,1,GL_FALSE, glm::value_ptr(app->camera.projection));

				GLint loc = glGetUniformLocation(forwardRenderProgram.handle, "specular");
				glUniform1f(loc, submeshMaterial.specular);

				//send texture for specular as well with a bool to know if the shader should use it :)

				u32 blockOffset = app->LocalParamsOffset +model.localParamsOffset;
				u32 blockSize = app->LocalAttBuffer.size;
				glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->LocalAttBuffer.handle, blockOffset, blockSize);
						
				u32 globalblockOffset = app->globalParamsOffset;
				u32 globalblockSize = app->cbuffer.size;
				glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, globalblockOffset, globalblockSize);

				u32 lightparblockOffset = app->LightParamsParamsOffset;
				u32 lightparblockSize = app->LightParamsBuffer.size;
				glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(3), app->LightParamsBuffer.handle, lightparblockOffset, lightparblockSize);


				Submesh& submesh = mesh.submeshes[j];
				glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
			}
		}
		break;
	case RenderMode_Deferred:
	{		//use deferred render


		glBindFramebuffer(GL_FRAMEBUFFER, app->frameBufferHandle);

		//glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glViewport(0, 0, app->displaySize.x, app->displaySize.y);

		//GLuint drawBuffers[] = { app->colorAttachmentHandle,app->normalAttachmentHandle,app->finalAttachmentHandle };

		GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2 ,GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);



		Program& texturedMeshProgram = app->programs[app->mapCalculationProgramIdx];
		glUseProgram(texturedMeshProgram.handle);

		for (int i = 0; i < app->models.size(); ++i)
		{
			Model& model = app->models[i];
			Mesh& mesh = app->meshes[model.meshIdx];

			for (u32 j = 0; j < mesh.submeshes.size(); ++j)
			{
				GLuint vao = FindVAO(mesh, j, texturedMeshProgram);
				glBindVertexArray(vao);

				u32 submeshMaterialIdx = model.materialIdx[j];
				Material& submeshMaterial = app->materials[submeshMaterialIdx];

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
				glUniform1i(app->texturedMeshProgram_uTexture, 0);
				
				if (submeshMaterial.normalsTextureIdx == 0)
				{
					GLint loc = glGetUniformLocation(texturedMeshProgram.handle, "normalMapExists");
					glUniform1i(loc, 0);
				}
				else
				{
					GLint loc = glGetUniformLocation(texturedMeshProgram.handle, "normalMapExists");
					glUniform1i(loc, 1);

					glActiveTexture(GL_TEXTURE1);
					GLint locnormals = glGetUniformLocation(texturedMeshProgram.handle, "uNormalMap");
					glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.normalsTextureIdx].handle);
					glUniform1i(locnormals, 1);
				}

				if (submeshMaterial.bumpTextureIdx == 0)
				{
					GLint loc = glGetUniformLocation(texturedMeshProgram.handle, "depthMapExists");
					glUniform1i(loc, 0);
				}
				else
				{
					GLint loc = glGetUniformLocation(texturedMeshProgram.handle, "depthMapExists");
					glUniform1i(loc, 1);

					GLint loc1 = glGetUniformLocation(texturedMeshProgram.handle, "depthStrength");
					glUniform1f(loc1, submeshMaterial.bumpStrength);

					glActiveTexture(GL_TEXTURE2);
					GLint locdepth = glGetUniformLocation(texturedMeshProgram.handle, "uDepthMap");
					glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.bumpTextureIdx].handle);
					glUniform1i(locdepth, 2);

				}


				if (submeshMaterial.specularTextureIdx == 0)
				{
					GLint loc = glGetUniformLocation(texturedMeshProgram.handle, "specularMapExists");
					glUniform1i(loc, 0);
				}
				else
				{
					GLint loc = glGetUniformLocation(texturedMeshProgram.handle, "specularMapExists");
					glUniform1i(loc, 1);

					glActiveTexture(GL_TEXTURE2);
					GLint locspec = glGetUniformLocation(texturedMeshProgram.handle, "uSpecularMap");
					glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.specularTextureIdx].handle);
					glUniform1i(locspec, 2);
				}

				GLint loc = glGetUniformLocation(texturedMeshProgram.handle, "specular");
				glUniform1f(loc, submeshMaterial.specular);

				//send texture for specular as well with a bool to know if the shader should use it :)

				u32 blockOffset = app->LocalParamsOffset + model.localParamsOffset;
				u32 blockSize = app->LocalAttBuffer.size;
				glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->LocalAttBuffer.handle, blockOffset, blockSize);

				u32 globalblockOffset = app->globalParamsOffset;
				u32 globalblockSize = app->cbuffer.size;
				glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, globalblockOffset, globalblockSize);

				u32 lightparblockOffset = app->LightParamsParamsOffset;
				u32 lightparblockSize = app->LightParamsBuffer.size;
				glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(3), app->LightParamsBuffer.handle, lightparblockOffset, lightparblockSize);


				Submesh& submesh = mesh.submeshes[j];
				glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
			}
		}



		//DEFERRED :D
		glBindFramebuffer(GL_FRAMEBUFFER, app->deferredBufferHandle);

		//glClearColor(1.0f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, app->displaySize.x, app->displaySize.y);
		glEnable(GL_DEPTH_TEST);

		glDrawBuffers(ARRAY_COUNT(drawBuffersdeferred), drawBuffersdeferred);

		glUseProgram(deferredRenderProgramIdx.handle);

		// - bind the program 
		glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glBlendEquation(GL_FUNC_ADD);
		//glDepthFunc(GL_EQUAL);

		//   (...and make its texture sample from unit 0)
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, app->colorAttachmentHandle);
		GLint loc0 = glGetUniformLocation(deferredRenderProgramIdx.handle, "uAlbedo");
		glUniform1i(loc0, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, app->normalAttachmentHandle);
		GLint loc1 = glGetUniformLocation(deferredRenderProgramIdx.handle, "uNormal");
		glUniform1i(loc1, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, app->positionAttachmentHandle);
		GLint loc2 = glGetUniformLocation(deferredRenderProgramIdx.handle, "uPosition");
		glUniform1i(loc2, 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, app->specularAttachmentHandle);
		GLint loc3 = glGetUniformLocation(deferredRenderProgramIdx.handle, "uSpecular");
		glUniform1i(loc3, 3);




		for (int i = 0; i < app->lights.size(); ++i)
		{
			glm::mat4 world = TransformPosition(vec3(0));
			app->lightworld = TransformPosition(vec3(0));

			int vertextodraw = 6;

			switch (app->lights[i].type)
			{
				case LightType_Directional:
				{
					//bind mainsquare :D
					//world = TransformPosition(vec3(0, 0.5, -3));
					//app->lightworld = app->camera.projection * app->camera.view * world;

					glBindVertexArray(app->vao);
					glBlendFunc(GL_ONE, GL_ONE);


				}
				break;
				case LightType_Point:
				{

					Light& light = app->lights[i];
					const float maxBrightness = std::fmaxf(std::fmaxf(light.color.r, light.color.g), light.color.b);
					float spheresize = (-light.Klinear + std::sqrt(light.Klinear * light.Klinear - 4 * light.Kquadratic * (light.Kconstant - (256.0f / 5.0f) * maxBrightness))) / (2.0f * light.Kquadratic);

					//calculate radius
					world = TransformPosition(app->lights[i].position) * TransformScale(vec3(spheresize));
					app->lightworld = app->camera.projection * app->camera.view *world;

					vertextodraw = app->spherebuffernumindices;

					glDisable(GL_DEPTH_TEST);
					glDisable(GL_CULL_FACE);
					//glFrontFace(GL_CW);
					glBlendFunc(GL_ONE, GL_ONE);

					glBindVertexArray(app->spherevao);

				}
				break;
				case LightType_Ambient:
				{
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glBindVertexArray(app->vao);
				}
				break;
				default:
					break;
			}

			glBindBuffer(GL_UNIFORM_BUFFER, app->LightTransformBuffer.handle);
			MapBuffer(app->LightTransformBuffer, GL_WRITE_ONLY);
			//handle lights
			app->LightTransformParamsOffset = app->LightTransformBuffer.head;

			AlignHead(app->LightTransformBuffer, app->uniformBlockAlignment);

			PushMat4(app->LightTransformBuffer, app->lightworld);

			app->LightTransformParamsSize = app->LightTransformBuffer.head - app->LightTransformParamsOffset;
			UnmapBuffer(app->LightTransformBuffer);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);


			u32 lightblockOffset = app->LightTransformParamsOffset;
			u32 lightblockSize = app->LightTransformBuffer.size;
			glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(2), app->LightTransformBuffer.handle, lightblockOffset, lightblockSize);


			u32 globalblockOffset = app->globalParamsOffset;
			u32 globalblockSize = app->cbuffer.size;
			glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, globalblockOffset, globalblockSize);

			u32 lightparblockOffset = app->LightParamsParamsOffset;
			u32 lightparblockSize = app->LightParamsBuffer.size;
			glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(3), app->LightParamsBuffer.handle, lightparblockOffset, lightparblockSize);


			//GLuint modelLoc = glGetUniformLocation(deferredRenderProgramIdx.handle, "model");
			//glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &worldViewProjection[0][0]);

			GLint loc4 = glGetUniformLocation(deferredRenderProgramIdx.handle, "current_light");
			glUniform1i(loc4, i);

			//glUniform1f(app->KlLocdeferred, app->lights[i].Klinear);
			//glUniform1f(app->KqLocdeferred, app->lights[i].Kquadratic);

			glDrawElements(GL_TRIANGLES, vertextodraw, GL_UNSIGNED_SHORT, 0);

		}
	}
	break;
	}
	glBindFramebuffer(GL_FRAMEBUFFER,0);

if(app->render_water)
{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		//glBlendEquation(GL_FUNC_ADD);

		if (app->rendermode == RenderMode_Deferred)
		{



			glBindFramebuffer(GL_FRAMEBUFFER, app->deferredBufferHandle);

			glDrawBuffers(ARRAY_COUNT(drawBuffersdeferred), drawBuffersdeferred);
		}
		else
		{
			glBindFramebuffer(GL_FRAMEBUFFER, app->frameBufferHandle);

			glDrawBuffers(ARRAY_COUNT(drawBuffersforward), drawBuffersforward);
		}


		Program& programWaterPlaneRender = app->programs[app->waterPlaneProgramIdx];
		glUseProgram(programWaterPlaneRender.handle);

		GLint locn1 = glGetUniformLocation(programWaterPlaneRender.handle, "uProjectionMatrix");
		glUniformMatrix4fv(locn1, 1,GL_FALSE, glm::value_ptr(app->camera.projection));

		GLint locn2 = glGetUniformLocation(programWaterPlaneRender.handle, "uWorldViewMatrix");
		glUniformMatrix4fv(locn2, 1,GL_FALSE, glm::value_ptr(app->camera.view/*add water transform matrix*/));

		GLint locn3 = glGetUniformLocation(programWaterPlaneRender.handle, "viewportSize");
		glUniform2f(locn3, app->displaySize.x, app->displaySize.y);

		GLint locn4 = glGetUniformLocation(programWaterPlaneRender.handle, "modelViewMatrix");
		glUniformMatrix4fv(locn4, 1, GL_FALSE, glm::value_ptr(app->camera.view/*add water transform matrix*/));

		GLint locn5 = glGetUniformLocation(programWaterPlaneRender.handle, "viewMatrixInv");
		glUniformMatrix4fv(locn5, 1, GL_FALSE, glm::value_ptr(glm::inverse(app->camera.view)));

		GLint locn6 = glGetUniformLocation(programWaterPlaneRender.handle, "projectionMatrixInv");
		glUniformMatrix4fv(locn6, 1, GL_FALSE, glm::value_ptr(glm::inverse(app->camera.projection)));


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, app->reflectionAttachmentHandle);
		GLint locn7 = glGetUniformLocation(programWaterPlaneRender.handle, "reflectionMap");
		glUniform1i(locn7, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, app->refractionAttachmentHandle);
		GLint locn8 = glGetUniformLocation(programWaterPlaneRender.handle, "refractionMap");
		glUniform1i(locn8, 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, app->reflectiondepthAttachmentHandle);
		GLint locn9 = glGetUniformLocation(programWaterPlaneRender.handle, "reflectionDepth");
		glUniform1i(locn9, 2);

		glActiveTexture(GL_TEXTURE3);
		GLint locn10 = glGetUniformLocation(programWaterPlaneRender.handle, "refractionDepth");
		glBindTexture(GL_TEXTURE_2D, app->refractiondepthAttachmentHandle);
		glUniform1i(locn10, 3);

		glActiveTexture(GL_TEXTURE4);
		GLint locn11 = glGetUniformLocation(programWaterPlaneRender.handle, "normalMap");
		glBindTexture(GL_TEXTURE_2D,app->textures[ app->waternormalMapIdx].handle);
		glUniform1i(locn11, 4);

		glActiveTexture(GL_TEXTURE5);
		GLint locn12 = glGetUniformLocation(programWaterPlaneRender.handle, "dudvMap");
		glBindTexture(GL_TEXTURE_2D, app->textures[app->waterdudvMapIdx].handle);//diceTexIdx
		glUniform1i(locn12, 5);

		int isDeferred = 0;

		if (app->rendermode == RenderMode_Deferred)
		{
			glActiveTexture(GL_TEXTURE6);
			GLint locn13 = glGetUniformLocation(programWaterPlaneRender.handle, "currdepthMap");
			glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);//diceTexIdx
			glUniform1i(locn13, 6);

			isDeferred = 1;

		}			
		
		GLint locb = glGetUniformLocation(programWaterPlaneRender.handle, "isDeferred");
		glUniform1i(locb, isDeferred);

		glBindVertexArray(app->waterplanevao);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	/*1
}
break;
        default:;
    }*/
}



void passWaterScene(Camera * cam, GLenum colorAttachment, bool reflection, App* app)
{
	glDrawBuffer(colorAttachment);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CLIP_DISTANCE0);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Program& programWaterRender = app->programs[app->waterRenderProgramIdx];
	glUseProgram(programWaterRender.handle);

	//glm::mat4 view = cam->view;
	//set uniforms of camera matrices

	if (reflection)
	{
		GLint loc4 = glGetUniformLocation(programWaterRender.handle, "clippingPlane");
		glm::vec4 ClippingPlane = {0, 1, 0, 0};
		glUniform4fv(loc4,1, glm::value_ptr(ClippingPlane));
	}
	else
	{
		GLint loc4 = glGetUniformLocation(programWaterRender.handle, "clippingPlane");
		glm::vec4 ClippingPlane = { 0, -1, 0, 0 };
		glUniform4fv(loc4, 1, glm::value_ptr(ClippingPlane));
	}

	GLint locworld = glGetUniformLocation(programWaterRender.handle, "uWorldMatrix");
	GLint locworldprojview = glGetUniformLocation(programWaterRender.handle, "uWorldViewProjectionMatrix");

	for (int i = 0; i<app->models.size(); ++i)
	{
		Model& model = app->models[i];
		Mesh& mesh = app->meshes[model.meshIdx];

		for (u32 j = 0; j < mesh.submeshes.size(); ++j)
		{
			GLuint vao = FindVAO(mesh, j, programWaterRender);
			glBindVertexArray(vao);

			u32 submeshMaterialIdx = model.materialIdx[j];
			Material& submeshMaterial = app->materials[submeshMaterialIdx];

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
			glUniform1i(app->texturedMeshProgram_uTexture, 0);

			//send texture for specular as well with a bool to know if the shader should use it :)

			//calculate matrices and send them as uniform

			glm::mat4 world = model.world;
			glm::mat4 worldViewProjection = cam->projection*cam->view* world;

			glUniformMatrix4fv(locworld,1,GL_FALSE,glm::value_ptr(world));
			glUniformMatrix4fv(locworldprojview, 1, GL_FALSE, glm::value_ptr(worldViewProjection));

			//u32 blockOffset = app->LocalParamsOffset + model.localParamsOffset;
			//u32 blockSize = app->LocalAttBuffer.size;
			//glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->LocalAttBuffer.handle, blockOffset, blockSize);

			Submesh& submesh = mesh.submeshes[j];
			glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
		}
	}



}

GLuint FindVAO(Mesh & mesh, u32 submeshIndex, const Program & program)
{
	Submesh& submesh = mesh.submeshes[submeshIndex];

	for (u32 i = 0; i < (u32)submesh.vao_list.size(); i++)
	{
		if (submesh.vao_list[i].programHandle == program.handle)
			return submesh.vao_list[i].handle;
	}

	GLuint vaoHandle = 0;

	glGenVertexArrays(1, &vaoHandle);
	glBindVertexArray(vaoHandle);

	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

	for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
	{
		bool attributeWasLinked = false;

		for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
		{
			if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
			{
				const u32 index = submesh.vertexBufferLayout.attributes[j].location;
				const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
				const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset;
				const u32 stride = submesh.vertexBufferLayout.stride;

				glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
				glEnableVertexAttribArray(index);

				attributeWasLinked = true;
				break;
			}
		}

		assert(attributeWasLinked);
	}

	glBindVertexArray(0);

	Vao vao = { vaoHandle, program.handle };
	submesh.vao_list.push_back(vao);

	return vaoHandle;
}

GLuint AddSphere(App * app)
{
	#define H 32
	#define V 8

	static const float pi = 3.1416f;
	struct Vertex { glm::vec3 pos; };

	Vertex sphere[H][V + 1];
	for (int h = 0; h < H; ++h) {
		for (int v = 0; v < V + 1; ++v)
		{
			float nh = float(h) / H;
			float nv = float(v) / V - 0.5;
			float angleh = 2 * pi * nh;
			float anglev = -pi * nv;

			sphere[h][v].pos.x = sinf(angleh)*cosf(anglev);
			sphere[h][v].pos.y = -sinf(anglev);
			sphere[h][v].pos.z = cosf(angleh)*cosf(anglev);

			glm::vec3 n = glm::normalize(sphere[h][v].pos);

			/*sphere[h][v].uv.y = glm::atan(n.x, n.z) / (2.0*pi) + 0.5;
			sphere[h][v].uv.x = n.y * 0.5 + 0.5;*/

		}
	}
	app->spherebuffernumindices = H * V * 6;
	u16 sphereIndices[H][V][6];
	for (int h = 0; h < H; ++h) {
		for (int v = 0; v < V; ++v)
		{
			sphereIndices[h][v][0] = (h + 0) * (V + 1) + v;
			sphereIndices[h][v][1] = ((h + 1)%H) * (V + 1) + v;
			sphereIndices[h][v][2] = ((h + 1)%H) * (V + 1) + v+1;
			sphereIndices[h][v][3] = (h + 0) * (V + 1) + v;
			sphereIndices[h][v][4] = ((h + 1)%H) * (V + 1) + v+1;
			sphereIndices[h][v][5] = (h + 0) * (V + 1) + v+1;

		}
	}

	glGenBuffers(1, &app->embeddedsphereVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedsphereVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sphere), sphere, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//Vertex)*H*(V+1
	glGenBuffers(1, &app->embeddedsphereElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedsphereElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(sphereIndices), sphereIndices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &app->spherevao);
	glBindVertexArray(app->spherevao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedsphereVertices);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedsphereElements);
	glBindVertexArray(0);

	return app->spherevao;
}

GLuint AddWaterPlane(App * app)
{
	struct VertexV3V3 {
		glm::vec3 pos;
		glm::vec3 uv;
	};

	const VertexV3V3 vertices[] = {
		{ glm::vec3(-20,0,-20), glm::vec3(0,1,0) },
	{ glm::vec3(20,0,-20), glm::vec3(0,1,0) },
	{ glm::vec3(20,0,20), glm::vec3(0,1,0) },
	{ glm::vec3(-20,0,20), glm::vec3(0,1,0) }
	};

	const u16 indices[] = {
		0,1,2,0,2,3
	};
	/*const u16 indices[] = {
		2,1,0,3,2,0
	};*/

	glGenBuffers(1, &app->embeddedwaterplaneVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedwaterplaneVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &app->embeddedwaterplaneElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedwaterplaneElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &app->waterplanevao);
	glBindVertexArray(app->waterplanevao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedwaterplaneVertices);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V3), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V3), (void*)12);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedwaterplaneElements);
	glBindVertexArray(0);

	return app->waterplanevao;
}
/*
u32 AddPlane(App * app)
{
	Model newModel = {};
	app->models.push_back(newModel);
	u32 modelidx = app->models.size() - 1;

	Mesh newMesh = {};
	app->meshes.push_back(newMesh);
	u32 meshidx = app->meshes.size() - 1;

	newModel.meshIdx = meshidx;

	Submesh newsubmesh = {};
	newMesh.submeshes.push_back(newsubmesh);

	newsubmesh.indexOffset = 0;
	newsubmesh.indices = ;
	newsubmesh.vao_list

	struct VertexV3V3 {
		glm::vec3 pos;
		glm::vec3 uv;
	};

	const VertexV3V3 vertices[] = {
		{ glm::vec3(-20,0,-20), glm::vec3(0,1,0) },
	{ glm::vec3(20,0,-20), glm::vec3(0,1,0) },
	{ glm::vec3(20,0,20), glm::vec3(0,1,0) },
	{ glm::vec3(-20,0,20), glm::vec3(0,1,0) }
	};

	const u16 indices[] = {
		0,1,2,0,2,3
	};
	/*const u16 indices[] = {
	2,1,0,3,2,0
	};*/
/*
	glGenBuffers(1, &app->embeddedwaterplaneVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedwaterplaneVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &app->embeddedwaterplaneElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedwaterplaneElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &app->waterplanevao);
	glBindVertexArray(app->waterplanevao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedwaterplaneVertices);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V3), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V3), (void*)12);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedwaterplaneElements);
	glBindVertexArray(0);

	return modelidx;
}*/

void RecalculateMatrix(Model * model)
{
	glm::quat q = glm::quat(model->rotation);
	glm::mat4 rotm = glm::mat4(q);
	
	//model->world = rotm * TransformScale(model->scale)* TransformPosition(model->position);

	model->world = TransformPosition(model->position)* rotm * TransformScale(model->scale);
}

void ChangePos(Model * model, float x, float y, float z)
{
	model->position = glm::vec3(x,y,z);
}

void ChangeScl(Model * model, float x, float y, float z)
{
	model->scale = glm::vec3(x, y, z);
}

void ChangeRot(Model * model, float x, float y, float z)
{
	model->rotation = glm::vec3(glm::radians(x) , glm::radians(y), glm::radians(z));
}
