//
//  RendererBackend.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-11-04.
//

#include "RendererBackend.h"
#include "glad/glad.h"
#include "r2/Platform/Platform.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Camera/Camera.h"
#include "r2/Core/Math/Ray.h"

#include "r2/Render/Backends/OpenGLImage.h"

#include "r2/Render/Backends/OpenGLMesh.h"
#include "r2/Render/Renderer/Model.h"
#include "r2/Render/Renderer/SkinnedModel.h"
#include "r2/Render/Renderer/LoadHelpers.h"

#include "r2/Render/Renderer/AnimationPlayer.h"

#include "r2/Render/Backends/OpenGLShader.h"

#include <map>

namespace
{
    enum
    {
        LIGHTING_SHADER = 0,
        LAMP_SHADER,
        DEBUG_SHADER,
        OUTLINE_SHADER,
        LEARN_OPENGL_SHADER,
        LEARN_OPENGL_SHADER2,
        LEARN_OPENGL_SHADER3,
        SKYBOX_SHADER,
        DEPTH_SHADER,
        DEPTH_CUBE_MAP_SHADER,
        NUM_SHADERS
    };
    
    //@Temp
    std::vector<r2::draw::opengl::Shader> s_shaders;
    
    float cubeVerts[] = {
        //front face
        0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,  //top right
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, //bottom right
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //bottom left
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // top left
        
        //right face
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, //top left 4
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, //bottom left 5
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f, //bottom right 6
        0.5f, 0.5f, -0.5f,  1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top right 7
        
        //left face
        -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, //bottom  left 8
        -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, //top left 9
        -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, //bottom right 10
        -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, //top right 11
        
        //back face
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, //bottom left 12
        0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, //top left 13
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, //bottom right 14
        -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f, //top right 15
        
        //top face
        -0.5f, 0.5f, 0.5f, 0.0, 1.0f, 0.0f, 0.0f, 0.0f, //bottom left 16
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, //top left 17
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //bottom right 18
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, //top right 19
        
        //bottom face
        -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, //bottom left 20
        -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, //top left 21
        0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, //bottom right 22
        0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f //top right 23
        
    };
    
    glm::vec3 cubePositions[] = {
        glm::vec3( 0.0f,  0.0f,  0.0f),
        glm::vec3( 2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3( 2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3( 1.3f, -2.0f, -2.5f),
        glm::vec3( 1.5f,  2.0f, -2.5f),
        glm::vec3( 1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };
    
    unsigned int indices[] = {  // note that we start from 0!
        0, 1, 3,  // first Triangle
        1, 2, 3,   // second Triangle
        4, 6, 5,
        6, 4, 7,
        8, 9, 10,
        9, 11, 10,
        12, 13, 14,
        13, 15, 14,
        16, 17, 18,
        17, 19, 18,
        20, 21, 22,
        21, 23, 22
    };
    
    const u32 NUM_POINT_LIGHTS = 2;
    glm::vec3 pointLightPositions[] = {
        glm::vec3( -1.5f,  1.f,  2.5f),
        glm::vec3( 1.5f,  1.f,  2.f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };
    
    glm::mat4 g_View = glm::mat4(1.0f);
    glm::mat4 g_Proj = glm::mat4(1.0f);
    glm::vec3 g_CameraPos = glm::vec3(0);
    glm::vec3 g_CameraDir = glm::vec3(0);
    glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
    glm::vec3 g_lightDir(-2.0f, -1.0f, 4.0f);
    
    u32 g_width, g_height;
    struct DebugVertex
    {
        glm::vec3 position;
    };
    
    struct LampVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
    };
    
    std::vector<DebugVertex> g_debugVerts;
    std::vector<r2::draw::opengl::OpenGLMesh> s_openglMeshes;
    r2::draw::SkinnedModel g_Model;
    s32 g_ModelAnimation = 0;
    
   // u32 g_VBO, g_EBO, g_DebugVAO, g_DebugVBO, defaultTexture, g_lightVAO;
    u32 defaultTexture;
    
    r2::draw::opengl::VertexArrayBuffer g_LampVAO, g_DebugVAO;
    
    float planeVertices[] = {
        // positions            // normals         // texcoords
        25.0f, -0.5f,  25.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
        25.0f, -0.5f,  -25.0f,  0.0f, 1.0f, 0.0f,   25.0f,  25.0f,
        -25.0f, -0.5f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
        
        -25.0f, -0.5f,  -25.0f,  0.0f, 1.0f, 0.0f,  0.0f,  25.0f,
        -25.0f, -0.5f, 25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
        25.0f, -0.5f, 25.0f,  0.0f, 1.0f, 0.0f,  25.0f, 0.0f
    };
    
    float cubeVertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
        1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        // front face
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
        1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        // right face
        1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
        1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
        1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
        1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
        1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
        1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
        // bottom face
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
        1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        // top face
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
        1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
    };

    float transparentVertices[] = {
        // positions         // texture Coords
        0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
        0.0f, -0.5f,  0.0f,  0.0f,  0.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  0.0f,
        
        0.0f,  0.5f,  0.0f,  0.0f,  1.0f,
        1.0f, -0.5f,  0.0f,  1.0f,  0.0f,
        1.0f,  0.5f,  0.0f,  1.0f,  1.0f
    };
    
    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,  1.0f, 0.0f,
        1.0f,  1.0f,  1.0f, 1.0f
    };
    
    //Learn OpenGL
    r2::draw::opengl::VertexArrayBuffer g_boxVAO, g_planeVAO, g_transparentVAO, g_quadVAO, g_skyboxVAO;
    u32 marbelTex, metalTex, windowTex, grassTex, skyboxTex;
    std::vector<glm::vec3> vegetation;
    r2::draw::Model g_planetModel;
    r2::draw::Model g_rockModel;
    std::vector<r2::draw::opengl::OpenGLMesh> s_planetMeshes;
    std::vector<r2::draw::opengl::OpenGLMesh> s_rockMeshes;
    r2::draw::opengl::FrameBuffer g_frameBuffer;
    r2::draw::opengl::FrameBuffer g_intermediateFrameBuffer;
    r2::draw::opengl::RenderBuffer g_renderBuffer;
    r2::draw::opengl::FrameBuffer g_depthBuffer;
    r2::draw::opengl::FrameBuffer g_depthMapFBO;
    u32 textureColorBuffer;
    u32 screenTexture;
    u32 woodTexture;
    u32 depthTexture;
    u32 depthCubeMap;
    
    r2::draw::opengl::FrameBuffer g_pointLightDepthMapFBO[NUM_POINT_LIGHTS];
    u32 g_pointLightDepthCubeMaps[NUM_POINT_LIGHTS];
    
    std::vector<glm::mat4> g_rockModelMatrices;

    
    const u32 g_instanceAmount = 5000;
    const float g_radius = 75.0f;
    const float g_offset = 10.0f;
    
    std::vector<std::string> g_cubeMapFaces
    {
        "skybox/right.jpg",
        "skybox/left.jpg",
        "skybox/top.jpg",
        "skybox/bottom.jpg",
        "skybox/front.jpg",
        "skybox/back.jpg"
    };
    
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,
        
        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,
        
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };
    
}

namespace r2::draw
{

    
    void SetupLighting(const opengl::Shader& shader);
    void SetupMVP(const opengl::Shader& shader, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj);
    void SetupVP(const opengl::Shader& shader, const glm::mat4& view, const glm::mat4& proj);
    void SetupModelMat(const opengl::Shader& shader, const glm::mat4& model);
    
    void SetupArrayMats(const opengl::Shader& shader, const std::string& arrayName, const std::vector<glm::mat4>& mats);
    
    void DrawOpenGLMesh(const opengl::Shader& shader, const opengl::OpenGLMesh& mesh);
    void DrawOpenGLMeshes(const opengl::Shader& shader, const std::vector<opengl::OpenGLMesh>& meshes);
    void DrawOpenGLMeshesInstanced(const opengl::Shader& shader, const std::vector<opengl::OpenGLMesh>& meshes, u32 numInstances);
    void DrawOpenGLMeshInstanced(const opengl::Shader& shader, const opengl::OpenGLMesh& mesh, u32 numInstances);
    
    void DrawSkinnedModel(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats);
    void DrawSkinnedModelOutline(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats);
    
    void DrawSkinnedModelDemo();
    void SetupSkinnedModelDemo();
    
    void SetupLearnOpenGLDemo();
    void DrawLearnOpenGLDemo();
    
    void OpenGLInit()
    {
        g_width = CENG.DisplaySize().width;
        g_height = CENG.DisplaySize().height;
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
       // glEnable(GL_FRAMEBUFFER_SRGB);
        
        char vertexPath[r2::fs::FILE_PATH_LENGTH];
        char fragmentPath[r2::fs::FILE_PATH_LENGTH];
        char geometryPath[r2::fs::FILE_PATH_LENGTH];
        
        s_shaders.resize(NUM_SHADERS, opengl::Shader());
        
#ifdef R2_ASSET_PIPELINE
        //load the shader pipeline assets
        {
            r2::asset::pln::ShaderManifest shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lighting.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lighting.fs", fragmentPath);

            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);

            s_shaders[LIGHTING_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[LIGHTING_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lamp.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lamp.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[LAMP_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[LAMP_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "debug.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "debug.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[DEBUG_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[DEBUG_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "SkeletonOutline.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "SkeletonOutline.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[OUTLINE_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[OUTLINE_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "LearnOpenGL.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "LearnOpenGL.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[LEARN_OPENGL_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[LEARN_OPENGL_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "LearnOpenGL2.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "LearnOpenGL2.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[LEARN_OPENGL_SHADER2].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[LEARN_OPENGL_SHADER2].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "LearnOpenGL3.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "LearnOpenGL3.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[LEARN_OPENGL_SHADER3].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[LEARN_OPENGL_SHADER3].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "Skybox.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "Skybox.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[SKYBOX_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[SKYBOX_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "DepthShader.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "DepthShader.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[DEPTH_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[DEPTH_SHADER].manifest = shaderManifest;
            
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "DepthCubeMap.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "DepthCubeMap.fs", fragmentPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "DepthCubeMap.gs", geometryPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            shaderManifest.geometryShaderPath = std::string(geometryPath);
            
            s_shaders[DEPTH_CUBE_MAP_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, geometryPath);
            s_shaders[DEPTH_CUBE_MAP_SHADER].manifest = shaderManifest;
            
        }
#endif

        SetupSkinnedModelDemo();
       // SetupLearnOpenGLDemo();
    }
    
    void OpenGLDraw(float alpha)
    {
        DrawSkinnedModelDemo();
        //DrawLearnOpenGLDemo();
    }
    
    void OpenGLShutdown()
    {
        opengl::Destroy(g_LampVAO);
        opengl::Destroy(g_DebugVAO);
        
        for (u32 i = 0; i < NUM_SHADERS; ++i)
        {
            glDeleteProgram(s_shaders[i].shaderProg);
        }
    }
    
    void OpenGLResizeWindow(u32 width, u32 height)
    {
        glViewport(0, 0, width, height);
        g_width = width;
        g_height = height;
    }
    
    void OpenGLSetCamera(const r2::Camera& cam)
    {
        g_View = cam.view;
        g_Proj = cam.proj;
        g_CameraPos = cam.position;
        g_CameraDir = cam.facing;
    }
    
    void OpenGLDrawRay(const r2::math::Ray& ray)
    {
        DebugVertex v0;
        
        v0.position = ray.origin + ray.direction;
        DebugVertex v1;
        v1.position = v0.position + ray.direction * 100.0f;
        
        g_debugVerts.push_back(v0);
        g_debugVerts.push_back(v1);
    }
    
    void DrawOpenGLMeshes(const opengl::Shader& shader, const std::vector<opengl::OpenGLMesh>& meshes)
    {
        for (u32 i = 0; i < meshes.size(); ++i)
        {
            DrawOpenGLMesh(shader, meshes[i]);
        }
    }
    
    void DrawOpenGLMeshesInstanced(const opengl::Shader& shader, const std::vector<opengl::OpenGLMesh>& meshes, u32 numInstances)
    {
        for (u32 i = 0; i < meshes.size(); ++i)
        {
            DrawOpenGLMeshInstanced(shader, meshes[i], numInstances);
        }
    }
    
    void DrawOpenGLMesh(const opengl::Shader& shader, const opengl::OpenGLMesh& mesh)
    {
        u32 textureNum[TextureType::NUM_TEXTURE_TYPES];
        for(u32 i = 0; i < TextureType::NUM_TEXTURE_TYPES; ++i)
        {
            textureNum[i] = 1;
        }

        for (u32 i = 0; i < mesh.numTextures; ++i)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string number;
            std::string name = TextureTypeToString(mesh.types[i]);
            number = std::to_string(textureNum[mesh.types[i]]++);
            shader.SetUInt(("material." + name + number).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, mesh.texIDs[i]);
        }

        //draw mesh
        opengl::Bind(mesh.vertexArray);
        glDrawElements(GL_TRIANGLES, mesh.numIndices, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
    
    void DrawOpenGLMeshInstanced(const opengl::Shader& shader, const opengl::OpenGLMesh& mesh, u32 numInstances)
    {
        u32 textureNum[TextureType::NUM_TEXTURE_TYPES];
        for(u32 i = 0; i < TextureType::NUM_TEXTURE_TYPES; ++i)
        {
            textureNum[i] = 1;
        }
        
        for (u32 i = 0; i < mesh.numTextures; ++i)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string number;
            std::string name = TextureTypeToString(mesh.types[i]);
            number = std::to_string(textureNum[mesh.types[i]]++);
            shader.SetUInt(("material." + name + number).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, mesh.texIDs[i]);
        }
        
        //draw mesh
        opengl::Bind(mesh.vertexArray);
        glDrawElementsInstanced(GL_TRIANGLES, (int)mesh.numIndices, GL_UNSIGNED_INT, 0, numInstances);
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }
    
    void DrawSkinnedModel(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats)
    {
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.01f));
        
        SetupModelMat(shader, modelMat);
        
        SetupArrayMats(shader, "boneTransformations", boneMats);
        
        DrawOpenGLMeshes(shader, s_openglMeshes);
    }
    
    void DrawSkinnedModelOutline(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats)
    {
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.01f));
        
        SetupMVP(shader, modelMat, g_View, g_Proj);
        SetupArrayMats(shader, "boneTransformations", boneMats);
        
        shader.SetUVec4("outlineColor", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

        DrawOpenGLMeshes(shader, s_openglMeshes);
    }

    void SetupArrayMats(const opengl::Shader& shader, const std::string& uniformName, const std::vector<glm::mat4>& boneMats)
    {
        for (u32 i = 0; i < boneMats.size(); ++i)
        {
            char boneTransformsStr[512];
            sprintf(boneTransformsStr, (uniformName + std::string("[%u]")).c_str(), i);
            shader.SetUMat4(boneTransformsStr, boneMats[i]);
        }
    }
    
    void SetupMVP(const opengl::Shader& shader, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj)
    {
        SetupModelMat(shader, model);
        SetupVP(shader, view, proj);
    }
    
    void SetupVP(const opengl::Shader& shader, const glm::mat4& view, const glm::mat4& proj)
    {
        shader.SetUMat4("view", view);
        shader.SetUMat4("projection", proj);
    }
    
    void SetupModelMat(const opengl::Shader& shader, const glm::mat4& model)
    {
        shader.SetUMat4("model", model);
    }
    
    void SetupLighting(const opengl::Shader& shader)
    {
        shader.SetUFloat("material.shininess", 64.0f);
        
        glm::vec3 diffuseColor = glm::vec3(0.3f);
        glm::vec3 ambientColor = glm::vec3(0.3f);
        glm::vec3 specularColor = glm::vec3(0.3f);
        
        float attenConst = 1.0f;
        float attenLinear = 0.09f;
        float attenQuad = 0.032f;
        //directional light setup
        {
            shader.SetUVec3("dirLight.direction", g_lightDir);
            shader.SetUVec3("dirLight.light.ambient", glm::vec3(0.2f));
            shader.SetUVec3("dirLight.light.diffuse", glm::vec3(0.2f));
            shader.SetUVec3("dirLight.light.specular", glm::vec3(0.2f));
        }
        
        //point light setup
        {
            for (u32 i = 0; i < NUM_POINT_LIGHTS; ++i)
            {
                char pointLightStr[512];
                sprintf(pointLightStr, "pointLights[%i].position", i);

                shader.SetUVec3(pointLightStr, pointLightPositions[i]);
                
                sprintf(pointLightStr, "pointLights[%i].attenuationState.constant", i);

                shader.SetUFloat(pointLightStr, attenConst);
                
                sprintf(pointLightStr, "pointLights[%i].attenuationState.linear", i);

                shader.SetUFloat(pointLightStr, attenLinear);
                
                
                sprintf(pointLightStr, "pointLights[%i].attenuationState.quadratic", i);

                shader.SetUFloat(pointLightStr, attenQuad);
                
                sprintf(pointLightStr, "pointLights[%i].light.ambient", i);

                shader.SetUVec3(pointLightStr, ambientColor);
                
                sprintf(pointLightStr, "pointLights[%i].light.diffuse", i);

                shader.SetUVec3(pointLightStr, diffuseColor);
                
                sprintf(pointLightStr, "pointLights[%i].light.specular", i);

                shader.SetUVec3(pointLightStr, specularColor);
            }
        }
        
        //spotlight setup
        {

            shader.SetUVec3("spotLight.position", g_CameraPos);
            
            shader.SetUVec3("spotLight.direction", g_CameraDir);
            
            shader.SetUVec3("spotLight.light.ambient", glm::vec3(0.0f));
            
            shader.SetUVec3("spotLight.light.diffuse", glm::vec3(1.0f));
            
            shader.SetUVec3("spotLight.light.specular", specularColor);
            
            shader.SetUVec3("spotLight.light.emission", specularColor);
            
            shader.SetUFloat("spotLight.attenuationState.constant", attenConst);
            
            shader.SetUFloat("spotLight.attenuationState.linear", attenLinear);
            
            shader.SetUFloat("spotLight.attenuationState.quadratic", attenQuad);

            shader.SetUFloat("spotLight.cutoff", glm::cos(glm::radians(12.5f)));
            
            shader.SetUFloat("spotLight.outerCutoff", glm::cos(glm::radians(15.f)));
        }
    }
    
    void OpenGLNextAnimation()
    {
        size_t numAnimations = g_Model.animations.size();
        g_ModelAnimation = (g_ModelAnimation + 1) % numAnimations;
        
    }
    
    void OpenGLPrevAnimation()
    {
        size_t numAnimations = g_Model.animations.size();
        if ((g_ModelAnimation - 1) < 0)
        {
            g_ModelAnimation = (s32)numAnimations-1;
        }
        else
            --g_ModelAnimation;
            
    }
    
    //DEMOS
    
    void SetupSkinnedModelDemo()
    {
        //load the model
        {
            //
            char modelPath[r2::fs::FILE_PATH_LENGTH];
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "micro_bat_lp/models/micro_bat.fbx", modelPath);
            char animationsPath[r2::fs::FILE_PATH_LENGTH];
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "micro_bat_lp/animations", animationsPath);
            LoadSkinnedModel(g_Model, modelPath);
            AddAnimations(g_Model, animationsPath);
            s_openglMeshes = opengl::CreateOpenGLMeshesFromSkinnedModel(g_Model);
        }
        
        opengl::Create(g_planeVAO);
        opengl::VertexBuffer planeVBO;
        opengl::Create(planeVBO, {
            {ShaderDataType::Float3, "aPos"},
            {ShaderDataType::Float3, "aNormal"},
            {ShaderDataType::Float2, "aTexCoord"}
        }, planeVertices, COUNT_OF(planeVertices), GL_STATIC_DRAW);
        
        opengl::AddBuffer(g_planeVAO, planeVBO);
        
        opengl::Create(g_boxVAO);
        
        opengl::VertexBuffer cubeVBO;
        opengl::Create(cubeVBO, {
            {ShaderDataType::Float3, "aPos"},
            {ShaderDataType::Float3, "aNormal"},
            {ShaderDataType::Float2, "aTexCoord"}
        }, cubeVertices, COUNT_OF(cubeVertices), GL_STATIC_DRAW);
        
        opengl::AddBuffer(g_boxVAO, cubeVBO);
        
        //lamp setup
        {
            opengl::Create(g_LampVAO);
            
            opengl::VertexBuffer verts;
            opengl::Create(verts, {
                {ShaderDataType::Float3, "aPos"},
                {ShaderDataType::Float3, "aNormal"},
                {ShaderDataType::Float2, "aTexCoord"}
            }, cubeVerts, COUNT_OF(cubeVerts), GL_STATIC_DRAW);
            
            opengl::IndexBuffer indexBuf;
            opengl::Create(indexBuf, indices, COUNT_OF(indices), GL_STATIC_DRAW);
            
            opengl::AddBuffer(g_LampVAO, verts);
            opengl::SetIndexBuffer(g_LampVAO, indexBuf);
            
            opengl::UnBind(g_LampVAO);
        }
        
        //debug setup
        {
            opengl::Create(g_DebugVAO);
            
            opengl::VertexBuffer vBuf;
            
            opengl::Create(vBuf, {
                {ShaderDataType::Float3, "aPos"}
            }, (float*)nullptr, 100 * 3 * 2, GL_STREAM_DRAW);
            
            opengl::AddBuffer(g_DebugVAO, vBuf);
            opengl::UnBind(g_DebugVAO);
        }
        
        opengl::Create(g_quadVAO);
        opengl::VertexBuffer quadVBO;
        opengl::Create(quadVBO, {
            {ShaderDataType::Float2, "aPos"},
            {ShaderDataType::Float2, "aTexCoord"}
        }, quadVertices, COUNT_OF(quadVertices), GL_STATIC_DRAW);
        
        opengl::AddBuffer(g_quadVAO, quadVBO);
        
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        
        opengl::Create(g_depthBuffer, 1024, 1024);
        depthTexture = opengl::AttachDepthToFrameBuffer(g_depthBuffer);
        
        for (u32 i = 0; i < NUM_POINT_LIGHTS; ++i)
        {
            opengl::Create(g_pointLightDepthMapFBO[i], 1024, 1024);
            g_pointLightDepthCubeMaps[i] = opengl::CreateDepthCubeMap(g_pointLightDepthMapFBO[i].width, g_pointLightDepthMapFBO[i].height);
            opengl::AttachDepthCubeMapToFrameBuffer(g_pointLightDepthMapFBO[i], g_pointLightDepthCubeMaps[i]);
        }

        u32 whiteColor = 0xffffffff;
        defaultTexture = opengl::CreateImageTexture(1, 1, &whiteColor);
        
        char path[r2::fs::FILE_PATH_LENGTH];
        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "wood.png", path);
        woodTexture = opengl::LoadImageTexture(path);
        
        
        
        
        s_shaders[LIGHTING_SHADER].UseShader();

        s_shaders[LIGHTING_SHADER].SetUInt("material.texture_diffuse1", 0);
        s_shaders[LIGHTING_SHADER].SetUInt("material.texture_specular1", 1);
        s_shaders[LIGHTING_SHADER].SetUInt("material.texture_ambient1", 2);
        s_shaders[LIGHTING_SHADER].SetUInt("material.emission", 3);
        s_shaders[LIGHTING_SHADER].SetUInt("skybox", 4);
        s_shaders[LIGHTING_SHADER].SetUInt("shadowMap", 5);
        
        for (u32 i = 0; i < NUM_POINT_LIGHTS; ++i)
        {
            char name[r2::fs::FILE_PATH_LENGTH];
            sprintf(name, "pointLightShadowMaps[%i]", i);
            s_shaders[LIGHTING_SHADER].SetUInt(name, 6+i);
        }
    }
    
    
    void DrawSkinnedMeshScene(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats)
    {
        
        
        
//        opengl::Bind(g_boxVAO);
//        glm::mat4 model = glm::mat4(1.0f);
//        model = glm::scale(model, glm::vec3(5.0f));
//        SetupModelMat(shader, model);
//        glDisable(GL_CULL_FACE);
//        shader.SetUBool("reverseNormals", true);
//        glDrawArrays(GL_TRIANGLES, 0, 36);
//        shader.SetUBool("reverseNormals", false);
//        glEnable(GL_CULL_FACE);
        
        //  glStencilFunc(GL_ALWAYS, 0, 0xFF);
        shader.SetUBool("animated", false);
        opengl::Bind(g_planeVAO);
//
//        //bottom
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

//        //back
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0, 10, 0));
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-10, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 0.0f, -1.0f));
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
//
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(10, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 0.0f, 1.0f));
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //            glStencilFunc(GL_ALWAYS, 1, 0xFF);
        //            glStencilMask(0xFF);
        
        shader.SetUBool("animated", true);
        DrawSkinnedModel(shader, boneMats);
        
        
        
    }
    
    void DrawSkinnedModelDemo()
    {
       // glStencilMask(0xFF);
        //render to depth map
        std::vector<glm::mat4> boneMats = r2::draw::PlayAnimationForSkinnedModel(CENG.GetTicks(),g_Model, g_ModelAnimation);
        
        //do the directional light shadow pass
        glm::mat4 lightProjection, lightView;
        
        {
            opengl::Bind(g_depthBuffer);
            glViewport(0, 0, g_depthBuffer.width, g_depthBuffer.height);
            glClear(GL_DEPTH_BUFFER_BIT );//| GL_STENCIL_BUFFER_BIT);
            
            float near_plane = 1.0f, far_plane = 100.0f;
            lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
            lightView = glm::lookAt(g_lightDir, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            
            s_shaders[DEPTH_SHADER].UseShader();
            
            SetupVP(s_shaders[DEPTH_SHADER], lightView, lightProjection);
            
            DrawSkinnedMeshScene(s_shaders[DEPTH_SHADER], boneMats);
            opengl::UnBind(g_depthBuffer);
        }
        
        float near = 1.0f;
        float far = 25.0f;
        {
            s_shaders[DEPTH_CUBE_MAP_SHADER].UseShader();
            
            float aspect = (float)g_pointLightDepthMapFBO[0].width / (float)g_pointLightDepthMapFBO[0].height;
            
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
            glActiveTexture(GL_TEXTURE0);
            
            for(u32 i = 0; i < NUM_POINT_LIGHTS; ++i)
            {
                std::vector<glm::mat4> shadowTransforms;

                shadowTransforms.push_back(shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
                shadowTransforms.push_back(shadowProj * glm::lookAt(pointLightPositions[i], pointLightPositions[i] + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)));
                
                glViewport(0, 0, g_pointLightDepthMapFBO[i].width, g_pointLightDepthMapFBO[i].height);
                opengl::Bind(g_pointLightDepthMapFBO[i]);
                glClear(GL_DEPTH_BUFFER_BIT);
                SetupArrayMats(s_shaders[DEPTH_CUBE_MAP_SHADER], "shadowMatrices", shadowTransforms);
                s_shaders[DEPTH_CUBE_MAP_SHADER].SetUVec3("lightPos", pointLightPositions[i]);
                s_shaders[DEPTH_CUBE_MAP_SHADER].SetUFloat("far_plane", far);
                DrawSkinnedMeshScene(s_shaders[DEPTH_CUBE_MAP_SHADER], boneMats);
            }
            
            opengl::UnBind(g_pointLightDepthMapFBO[NUM_POINT_LIGHTS-1]);
            
        }
        
        //Draw the scene for reals
        {
            s_shaders[LIGHTING_SHADER].UseShader();
            glViewport(0, 0, g_width, g_height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
            
            
 
            SetupVP(s_shaders[LIGHTING_SHADER], g_View, g_Proj);
            SetupLighting(s_shaders[LIGHTING_SHADER]);
            float timeVal = static_cast<float>(CENG.GetTicks()) / 1000.f;
            
            s_shaders[LIGHTING_SHADER].SetUFloat("time", timeVal);
            s_shaders[LIGHTING_SHADER].SetUVec3("viewPos", g_CameraPos);
            s_shaders[LIGHTING_SHADER].SetUMat4("lightDirSpaceMatrix", lightProjection * lightView);
            s_shaders[LIGHTING_SHADER].SetUFloat("far_plane", far);
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, depthTexture);
            
            for (u32 i =0; i < NUM_POINT_LIGHTS; ++i)
            {
                glActiveTexture(GL_TEXTURE6 + i);
                glBindTexture(GL_TEXTURE_CUBE_MAP, g_pointLightDepthCubeMaps[i]);
            }
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            DrawSkinnedMeshScene(s_shaders[LIGHTING_SHADER], boneMats);
        }
        
        
        //draw object if it's behind
        {
            
//            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
//            glStencilMask(0x00);
//            glDisable(GL_DEPTH_TEST);
//
//            glUseProgram(s_shaders[OUTLINE_SHADER].shaderProg);
//
//            DrawSkinnedModelOutline(s_shaders[OUTLINE_SHADER], boneMats);
//            glEnable(GL_DEPTH_TEST);
//            glStencilFunc(GL_ALWAYS, 1, 0xFF);
        }
        
        //Draw lamp
        {
            
            s_shaders[LAMP_SHADER].UseShader();

            SetupVP(s_shaders[LAMP_SHADER], g_View, g_Proj);
            
            opengl::Bind(g_LampVAO);
            
            
            for (u32 i = 0; i < NUM_POINT_LIGHTS; ++i)
            {
                glm::mat4 lightModel = glm::mat4(1.0f);
                
                lightModel = glm::translate(lightModel, pointLightPositions[i]);
                lightModel = glm::scale(lightModel, glm::vec3(0.2f));
                
                SetupModelMat(s_shaders[LAMP_SHADER], lightModel);
                
                glDrawElements(GL_TRIANGLES, COUNT_OF(indices), GL_UNSIGNED_INT, 0);
            }
        }
        
        //draw debug stuff
        if (g_debugVerts.size() > 0)
        {
            s_shaders[DEBUG_SHADER].UseShader();
            
            SetupMVP(s_shaders[DEBUG_SHADER], glm::mat4(1.0f), g_View, g_Proj);
            
            s_shaders[DEBUG_SHADER].SetUVec4("debugColor", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
            
            opengl::Bind(g_DebugVAO);
            
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DebugVertex)*g_debugVerts.size(), &g_debugVerts[0]);
            
            glDrawArrays(GL_LINES, 0, g_debugVerts.size());
        }
//
//        glDisable(GL_DEPTH_TEST);
//        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
//        glClear(GL_COLOR_BUFFER_BIT);
//
//        s_shaders[LEARN_OPENGL_SHADER2].UseShader();
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, depthTexture);
//      //  s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("near_plane", near);
//      //  s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("far_plane", far);
//        opengl::Bind(g_quadVAO);
//        //Use the texture that was drawn to in the previous pass
//
//        glDrawArrays(GL_TRIANGLES, 0, 6);
//        glEnable(GL_DEPTH_TEST);
    }
    
    void GenerateRockMatrices(u32 amount, float radius, float offset)
    {
        srand(CENG.GetTicks());
        g_rockModelMatrices.clear();
        g_rockModelMatrices.reserve(amount);
        for (u32 i = 0; i < amount; ++i)
        {
            glm::mat4 model = glm::mat4(1.0f);
            
            float angle = (float)i / (float)amount * 360.0f;
            
            float displacement = (rand() % (int)(2 * offset * 100))/ 100.0f - offset;
            float x = sin(angle) * radius + displacement;
            displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float y = displacement * 0.4f;
            displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
            float z = cos(angle) * radius + displacement;
            model = glm::translate(model, glm::vec3(x, y, z));
            
            float scale = (rand() % 20 )/ 100.0f + 0.05;
            model = glm::scale(model, glm::vec3(scale));
            
            float rotAngle = (rand() %360);
            model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));
            
            g_rockModelMatrices.push_back(model);
        }
    }
    
    void SetupLearnOpenGLDemo()
    {
        
        //
//        char modelPath[r2::fs::FILE_PATH_LENGTH];
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "planet/planet.obj", modelPath);
//        LoadModel(g_planetModel, modelPath);
//        s_planetMeshes = opengl::CreateOpenGLMeshesFromModel(g_planetModel);
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "rock/rock.obj", modelPath);
//        LoadModel(g_rockModel, modelPath);
//        s_rockMeshes = opengl::CreateOpenGLMeshesFromModel(g_rockModel);
//        GenerateRockMatrices(g_instanceAmount, g_radius, g_offset);
//
//        opengl::VertexBuffer rockModelMatrixVBO;
//        opengl::Create(rockModelMatrixVBO, {{
//            {ShaderDataType::Float4, ""},
//            {ShaderDataType::Float4, ""},
//            {ShaderDataType::Float4, ""},
//            {ShaderDataType::Float4, ""}
//        }, VertexType::Instanced}, &g_rockModelMatrices[0], g_instanceAmount * sizeof(glm::mat4), GL_STATIC_DRAW);
//
//        for (u32 i = 0; i < s_rockMeshes.size(); ++i)
//        {
//            opengl::AddBuffer( s_rockMeshes[i].vertexArray, rockModelMatrixVBO);
//        }
        
//        LoadSkinnedModel(g_Model, modelPath);
//        s_openglMeshes = opengl::CreateOpenGLMeshesFromSkinnedModel(g_Model);
        
        opengl::Create(g_boxVAO);

        opengl::VertexBuffer cubeVBO;
        opengl::Create(cubeVBO, {
            {ShaderDataType::Float3, "aPos"},
            {ShaderDataType::Float3, "aNormal"},
            {ShaderDataType::Float2, "aTexCoord"}
        }, cubeVertices, COUNT_OF(cubeVertices), GL_STATIC_DRAW);

        opengl::AddBuffer(g_boxVAO, cubeVBO);

        opengl::Create(g_planeVAO);
        opengl::VertexBuffer planeVBO;
        opengl::Create(planeVBO, {
            {ShaderDataType::Float3, "aPos"},
            {ShaderDataType::Float3, "aNormal"},
            {ShaderDataType::Float2, "aTexCoord"}
        }, planeVertices, COUNT_OF(planeVertices), GL_STATIC_DRAW);

        opengl::AddBuffer(g_planeVAO, planeVBO);
//
//
//        opengl::Create(g_transparentVAO);
//
//        opengl::VertexBuffer transparentVBO;
//
//        opengl::Create(transparentVBO, {
//            {ShaderDataType::Float3, "aPos"},
//            {ShaderDataType::Float2, "aTexCoord"}
//        }, transparentVertices, COUNT_OF(transparentVertices), GL_STATIC_DRAW);
//
//        opengl::AddBuffer(g_transparentVAO, transparentVBO);
//
        opengl::Create(g_quadVAO);
        opengl::VertexBuffer quadVBO;
        opengl::Create(quadVBO, {
            {ShaderDataType::Float2, "aPos"},
            {ShaderDataType::Float2, "aTexCoord"}
        }, quadVertices, COUNT_OF(quadVertices), GL_STATIC_DRAW);

        opengl::AddBuffer(g_quadVAO, quadVBO);

//
//        opengl::Create(g_skyboxVAO);
//        opengl::VertexBuffer skyboxVBO;
//        opengl::Create(skyboxVBO, {
//            {ShaderDataType::Float3, "aPos"}
//        }, skyboxVertices, COUNT_OF(skyboxVertices), GL_STATIC_DRAW);
//
//        opengl::AddBuffer(g_skyboxVAO, skyboxVBO);
//
//        opengl::UnBind(g_skyboxVAO);
//
//        vegetation.push_back(glm::vec3(-1.5f,  0.0f, -0.48f));
//        vegetation.push_back(glm::vec3( 1.5f,  0.0f,  0.51f));
//        vegetation.push_back(glm::vec3( 0.0f,  0.0f,  0.7f));
//        vegetation.push_back(glm::vec3(-0.3f,  0.0f, -2.3f));
//        vegetation.push_back(glm::vec3( 0.5f,  0.0f, -0.6f));
//
        char path[r2::fs::FILE_PATH_LENGTH];
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "marble.jpg", path);
//
//        marbelTex = opengl::LoadImageTexture(path);
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "metal.png", path);
//        metalTex = opengl::LoadImageTexture(path);
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "grass.png", path);
//
//        grassTex = opengl::LoadImageTexture(path);
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "blending_transparent_window.png", path);
//
//        windowTex = opengl::LoadImageTexture(path);
//
        
        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "wood.png", path);
        woodTexture = opengl::LoadImageTexture(path);
        
        
        
//        skyboxTex = opengl::CreateCubeMap(g_cubeMapFaces);
//
//        s_shaders[LEARN_OPENGL_SHADER].UseShader();
//        s_shaders[LEARN_OPENGL_SHADER].SetUInt("texture1", 0);
//

//
        
        
        
//        s_shaders[SKYBOX_SHADER].UseShader();
//        s_shaders[SKYBOX_SHADER].SetUInt("skybox", 0);
//
//        glActiveTexture(GL_TEXTURE0);
//        glEnable(GL_DEPTH_TEST);
//        glEnable(GL_BLEND);
//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        
        //setup framebuffers
        
        opengl::Create(g_depthBuffer, 1024, 1024);
        depthTexture = opengl::AttachDepthToFrameBuffer(g_depthBuffer);
        
        opengl::Create(g_frameBuffer, CENG.DisplaySize().width, CENG.DisplaySize().height);
        textureColorBuffer = opengl::AttachTextureToFrameBuffer(g_frameBuffer);
        
        opengl::Create(g_renderBuffer, CENG.DisplaySize().width, CENG.DisplaySize().height);
        opengl::AttachDepthAndStencilForRenderBufferToFrameBuffer(g_frameBuffer, g_renderBuffer);
        
        opengl::Create(g_depthMapFBO, 1024, 1024);
        depthCubeMap = opengl::CreateDepthCubeMap(g_depthMapFBO.width, g_depthMapFBO.height);
        opengl::AttachDepthCubeMapToFrameBuffer(g_depthMapFBO, depthCubeMap);
        
        

//        opengl::Create(g_frameBuffer, CENG.DisplaySize().width, CENG.DisplaySize().height);
//        textureColorBuffer = opengl::AttachMultisampleTextureToFrameBuffer(g_frameBuffer, 8);
//        opengl::Create(g_renderBuffer, CENG.DisplaySize().width, CENG.DisplaySize().height);
//
//        opengl::AttachDepthAndStencilMultisampleForRenderBufferToFrameBuffer(g_frameBuffer, g_renderBuffer, 8);
//        opengl::UnBind(g_frameBuffer);
//
//        //We want this so we can use post proc effects
//        opengl::Create(g_intermediateFrameBuffer, CENG.DisplaySize().width, CENG.DisplaySize().height);
//        screenTexture = opengl::AttachTextureToFrameBuffer(g_intermediateFrameBuffer);
//
//        opengl::UnBind(g_intermediateFrameBuffer);
        
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        s_shaders[LEARN_OPENGL_SHADER2].UseShader();
        s_shaders[LEARN_OPENGL_SHADER2].SetUInt("screenTexture", 0);
        
        s_shaders[LEARN_OPENGL_SHADER].UseShader();
        s_shaders[LEARN_OPENGL_SHADER].SetUInt("diffuseTexture", 0);
        s_shaders[LEARN_OPENGL_SHADER].SetUInt("shadowMap", 1);
    }
    
    void DrawScene(const r2::draw::opengl::Shader& shader)
    {
        
        //render the scene
        opengl::Bind(g_boxVAO);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(5.0f));
        SetupModelMat(shader, model);
        glDisable(GL_CULL_FACE);
        shader.SetUBool("reverseNormals", true);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        shader.SetUBool("reverseNormals", false);
        glEnable(GL_CULL_FACE);
        
        //draw the floor
        
//        SetupModelMat(shader, model);
//        opengl::Bind(g_planeVAO);
//        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        //draw the cubes
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(4.0f, -3.5, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(2.0f, 3.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.75f));
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5f));
        model = glm::scale(model, glm::vec3(0.5f));
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0f));
        model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
        model = glm::scale(model, glm::vec3(0.75f));
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    void DrawLearnOpenGLDemo()
    {
        //Pass 1
        float timeVal = static_cast<float>(CENG.GetTicks()) / 1000.f;
        lightPos.z = sin(timeVal * 0.5) * 3.0;
        
        float aspect = (float)g_depthMapFBO.width / (float)g_depthMapFBO.height;
        float near = 1.0f;
        float far = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
        std::vector<glm::mat4> shadowTransforms;
        
        shadowTransforms.push_back(shadowProj *
                                     glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj *
                                     glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj *
                                     glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj *
                                     glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
        shadowTransforms.push_back(shadowProj *
                                     glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
        shadowTransforms.push_back(shadowProj *
                                     glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));
        {
            
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            
            s_shaders[DEPTH_CUBE_MAP_SHADER].UseShader();
            //glm::mat4 lightProjection, lightView;
            
            //lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
            //lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            //SetupVP(s_shaders[DEPTH_SHADER], lightView, lightProjection);
            
            glViewport(0, 0, g_depthMapFBO.width, g_depthMapFBO.height);
            opengl::Bind(g_depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            SetupArrayMats(s_shaders[DEPTH_CUBE_MAP_SHADER], "shadowMatrices", shadowTransforms);
            s_shaders[DEPTH_CUBE_MAP_SHADER].SetUVec3("lightPos", lightPos);
            s_shaders[DEPTH_CUBE_MAP_SHADER].SetUFloat("far_plane", far);
            DrawScene(s_shaders[DEPTH_CUBE_MAP_SHADER]);
            
            //opengl::UnBind(g_depthBuffer);
            
            opengl::Bind(g_frameBuffer);
            
            glViewport(0, 0, g_width, g_height);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            s_shaders[LEARN_OPENGL_SHADER].UseShader();
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
            SetupVP(s_shaders[LEARN_OPENGL_SHADER], g_View, g_Proj);
            //s_shaders[LEARN_OPENGL_SHADER].SetUMat4("lightSpaceMatrix", lightProjection * lightView);
            s_shaders[LEARN_OPENGL_SHADER].SetUVec3("lightPos", lightPos);
            s_shaders[LEARN_OPENGL_SHADER].SetUVec3("viewPos", g_CameraPos);
            s_shaders[LEARN_OPENGL_SHADER].SetUFloat("far_plane", far);
            DrawScene(s_shaders[LEARN_OPENGL_SHADER]);
            opengl::UnBind(g_frameBuffer);
            //draw the planet
            
//            glm::mat4 model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
//            model = glm::scale(model, glm::vec3(4.0f));
//            s_shaders[LEARN_OPENGL_SHADER].SetUMat4("model", model);
//            DrawOpenGLMeshes(s_shaders[LEARN_OPENGL_SHADER], s_planetMeshes);
//
//
//            s_shaders[LEARN_OPENGL_SHADER3].UseShader();
//            SetupVP(s_shaders[LEARN_OPENGL_SHADER3], g_View, g_Proj);
//            DrawOpenGLMeshesInstanced(s_shaders[LEARN_OPENGL_SHADER3], s_rockMeshes, g_instanceAmount);
            
           // for (u32 i = 0; i < 1000; ++i)
           // {
             //   s_shaders[LEARN_OPENGL_SHADER].SetUMat4("model", g_rockModelMatrices[i]);
            //    DrawOpenGLMeshes(s_shaders[LEARN_OPENGL_SHADER3], s_rockMeshes);
           // }
            
//            s_shaders[LIGHTING_SHADER].UseShader();
//
//            s_shaders[LIGHTING_SHADER].SetUInt("skybox", 3);
//            SetupLighting(s_shaders[LIGHTING_SHADER]);
//
//            float timeVal = static_cast<float>(CENG.GetTicks()) / 1000.f;
//
//            s_shaders[LIGHTING_SHADER].SetUFloat("time", timeVal);
//            s_shaders[LIGHTING_SHADER].SetUVec3("viewPos", g_CameraPos);
//
//            glm::mat4 modelMat = glm::mat4(1.0f);
//            //modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//            modelMat = glm::scale(modelMat, glm::vec3(0.1f));
//
//            glActiveTexture(GL_TEXTURE3);
//            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
//            SetupMVP(s_shaders[LIGHTING_SHADER], modelMat, g_View, g_Proj);
//
//
//            DrawOpenGLMeshes(s_shaders[LIGHTING_SHADER], s_openglMeshes);
            
//            s_shaders[LEARN_OPENGL_SHADER].UseShader();
//            SetupVP(s_shaders[LEARN_OPENGL_SHADER], g_View, g_Proj);
//            s_shaders[LEARN_OPENGL_SHADER].SetUVec3("cameraPos", g_CameraPos);
//            //Draw boxes
//            {
//                opengl::Bind(g_boxVAO);
//                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
//
//                glm::mat4 model = glm::mat4(1.0f);
//
//                model = glm::translate(model, glm::vec3(-1.0f, 0.0f, -1.0f));
//                s_shaders[LEARN_OPENGL_SHADER].SetUMat4("model", model);
//                glDrawArrays(GL_TRIANGLES, 0, 36);
//
//                model = glm::mat4(1.0f);
//
//                model = glm::translate(model, glm::vec3(2.0f, 0.0f, 0.0f));
//                s_shaders[LEARN_OPENGL_SHADER].SetUMat4("model", model);
//                glDrawArrays(GL_TRIANGLES, 0, 36);
//            }
            
            //draw floor
//            {
//                opengl::Bind(g_planeVAO);
//                glBindTexture(GL_TEXTURE_2D, metalTex);
//
//                glm::mat4 model = glm::mat4(1.0f);
//                s_shaders[LEARN_OPENGL_SHADER].SetUMat4("model", model);
//
//                glDrawArrays(GL_TRIANGLES, 0, 6);
//            }
            
            //draw skybox
            {
//                glDepthFunc(GL_LEQUAL);
//                s_shaders[SKYBOX_SHADER].UseShader();
//
//                //change the view to not have any translation
//                glm::mat4 view = glm::mat4(glm::mat3(g_View));
//
//                SetupVP(s_shaders[SKYBOX_SHADER], view, g_Proj);
//                opengl::Bind(g_skyboxVAO);
//                glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
//                glDrawArrays(GL_TRIANGLES, 0, 36);
//
//                glDepthFunc(GL_LESS);
            }
            
//            {
//                //sort the windows
//                s_shaders[LEARN_OPENGL_SHADER].UseShader();
//                //SetupVP(s_shaders[LEARN_OPENGL_SHADER], g_View, g_Proj);
//
//                std::sort(vegetation.begin(), vegetation.end(), [](const glm::vec3& v1, const glm::vec3& v2){
//
//                    float distance1 = glm::length(g_CameraPos - v1);
//                    float distance2 = glm::length(g_CameraPos - v2);
//
//                    //we want to draw the bigger one first
//                    return distance1 > distance2;
//                });
//
//                opengl::Bind(g_transparentVAO);
//                glBindTexture(GL_TEXTURE_2D, windowTex);
//                glm::mat4 model = glm::mat4(1.0f);
//
//
//                for (auto& vPos : vegetation)
//                {
//                    model = glm::mat4(1.0f);
//                    model = glm::translate(model, vPos);
//                    s_shaders[LEARN_OPENGL_SHADER].SetUMat4("model", model);
//                    glDrawArrays(GL_TRIANGLES, 0, 6);
//                }
//            }
            
            
        }
        
        //Pass 2
        {
            //glBindFramebuffer(GL_READ_FRAMEBUFFER, g_frameBuffer.FBO);
//            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_intermediateFrameBuffer.FBO);
//            glBlitFramebuffer(0, 0, CENG.DisplaySize().width, CENG.DisplaySize().height, 0, 0, CENG.DisplaySize().width, CENG.DisplaySize().height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            
            //Render to texture code
            glDisable(GL_DEPTH_TEST);
            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            s_shaders[LEARN_OPENGL_SHADER2].UseShader();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
            s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("near_plane", near);
            s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("far_plane", far);
            opengl::Bind(g_quadVAO);
            //Use the texture that was drawn to in the previous pass
            
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glEnable(GL_DEPTH_TEST);
        }
        
    }
    
#ifdef R2_ASSET_PIPELINE
    void ReloadShader(const r2::asset::pln::ShaderManifest& manifest)
    {
        for (auto& shaderAsset : s_shaders)
        {
            if (shaderAsset.manifest.vertexShaderPath == manifest.vertexShaderPath &&
                shaderAsset.manifest.fragmentShaderPath == manifest.fragmentShaderPath)
            {
                opengl::ReloadShaderProgramFromRawFiles(&shaderAsset.shaderProg, shaderAsset.manifest.vertexShaderPath.c_str(), shaderAsset.manifest.fragmentShaderPath.c_str(),
                    shaderAsset.manifest.geometryShaderPath.c_str());
            }
        }
    }
#endif
}
