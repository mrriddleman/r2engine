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
#include "r2/Core/Math/MathUtils.h"
#include <map>
#include <random>

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
        LIGHTBOX_SHADER,
        BLUR_SHADER,
        SSAO_SHADER,
        PBR_SHADER,
        CUBEMAP_SHADER,
        BACKGROUND_SHADER,
        CONVOLUTION_SHADER,
        PREFILTER_SHADER,
        BRDF_SHADER,
        TWO_D_SHADER,
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
    glm::vec3 lightPos(2.0, 4.0, -2.0);
    glm::vec3 lightColor = glm::vec3(0.2, 0.2, 0.7);
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
    r2::draw::opengl::VertexArrayBuffer g_boxVAO, g_planeVAO, g_transparentVAO, g_quadVAO, g_skyboxVAO, g_normalMappedQuadVAO, g_sphereVAO;
    u32 marbelTex, metalTex, windowTex, grassTex, skyboxTex;
    std::vector<glm::vec3> vegetation;
    r2::draw::Model g_planetModel;
    r2::draw::Model g_rockModel;
    r2::draw::Model g_nanosuitModel;
    std::vector<r2::draw::opengl::OpenGLMesh> s_planetMeshes;
    std::vector<r2::draw::opengl::OpenGLMesh> s_rockMeshes;
    r2::draw::opengl::FrameBuffer g_frameBuffer;
    r2::draw::opengl::FrameBuffer g_intermediateFrameBuffer;
    r2::draw::opengl::RenderBuffer g_renderBuffer;
    r2::draw::opengl::FrameBuffer g_depthBuffer;
    r2::draw::opengl::FrameBuffer g_depthMapFBO;
    
    r2::draw::opengl::FrameBuffer g_hdrFBO;
    r2::draw::opengl::FrameBuffer g_pingPongFBO[2];
    
    r2::draw::opengl::FrameBuffer g_captureFBO;
    r2::draw::opengl::RenderBuffer g_captureRBO;
    
    r2::draw::opengl::FrameBuffer g_prefiltedFBO;
    r2::draw::opengl::RenderBuffer g_prefilteredRBO;
    
    enum GBufferBuffers
    {
        GBUFFER_POSITION = 0,
        GBUFFER_NORMAL,
        GBUFFER_ALBEDO_SPEC,
        NUM_GBUFFER_BUFFERS
    };
    r2::draw::opengl::FrameBuffer g_gBuffer;
    u32 g_gBufferTextures[NUM_GBUFFER_BUFFERS];
    
    r2::draw::opengl::FrameBuffer g_ssaoFBO;
    r2::draw::opengl::FrameBuffer g_ssaoBlurFBO;
    u32 g_ssaoColorBuffer;
    u32 g_ssaoBlurColorBuffer;
    u32 g_ssaoNoiseTexture;
    std::vector<glm::vec3> ssaoKernel;
    
    std::vector<glm::vec3> g_objectPositions;
    u32 textureColorBuffer;
    u32 textureColorBuffer2;
    u32 pingPongTexture[2];
    u32 screenTexture;
    u32 woodTexture;
    u32 depthTexture;
    u32 depthCubeMap;
    u32 bricksTexture;
    u32 normalMappedBricksTexture;
    u32 bricksHeightMapTexture;
    u32 containerTexture;
    u32 hdrTexture;
    u32 envCubemap;
    u32 albedoMapTexture, normalMapTexture, metallicMapTexture, roughnessMapTexture, aoMapTexture;
    u32 irradianceMapTexture;
    u32 prefilteredMap;
    u32 brdfLUTTexture;
    
    r2::draw::opengl::FrameBuffer g_pointLightDepthMapFBO[NUM_POINT_LIGHTS];
    u32 g_pointLightDepthCubeMaps[NUM_POINT_LIGHTS];
    
    std::vector<glm::mat4> g_rockModelMatrices;

    
    const u32 g_instanceAmount = 5000;
    const float g_radius = 75.0f;
    const float g_offset = 10.0f;
    
    const int g_numRows = 7;
    const int g_numColumns = 7;
    const float spacing = 2.5f;
    
    // lighting info
    // -------------
    // positions
    std::vector<glm::vec3> g_lightPositions;

    // colors
    std::vector<glm::vec3> g_lightColors;
    
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

    void CalculateTangentAndBiTangent(const glm::vec3& edge1, const glm::vec3& edge2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2, glm::vec3& tangent, glm::vec3& bitangent);
    void SetupLighting(const opengl::Shader& shader);
    void SetupMVP(const opengl::Shader& shader, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj);
    void SetupVP(const opengl::Shader& shader, const glm::mat4& view, const glm::mat4& proj);
    void SetupModelMat(const opengl::Shader& shader, const glm::mat4& model);
    
    void SetupArrayMats(const opengl::Shader& shader, const std::string& arrayName, const std::vector<glm::mat4>& mats);

    void MakeSphere(opengl::VertexArrayBuffer& vao, u32 segments);
    void RenderSphere(const opengl::Shader& shader, const glm::mat4& model);
    
    void DrawOpenGLMesh(const opengl::Shader& shader, const opengl::OpenGLMesh& mesh);
    void DrawOpenGLMeshes(const opengl::Shader& shader, const std::vector<opengl::OpenGLMesh>& meshes);
    void DrawOpenGLMeshesInstanced(const opengl::Shader& shader, const std::vector<opengl::OpenGLMesh>& meshes, u32 numInstances);
    void DrawOpenGLMeshInstanced(const opengl::Shader& shader, const opengl::OpenGLMesh& mesh, u32 numInstances);
    
    void DrawCube(const opengl::Shader& shader, const glm::mat4& model);
    void DrawQuad(const opengl::Shader& shader, const glm::mat4& model);

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
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "LightBox.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "LightBox.fs", fragmentPath);
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[LIGHTBOX_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[LIGHTBOX_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "Blur.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "Blur.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[BLUR_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[BLUR_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "SSAO.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "SSAO.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[SSAO_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[SSAO_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "PBR.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "PBR.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[PBR_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[PBR_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "CubeMap.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "CubeMap.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[CUBEMAP_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[CUBEMAP_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "Background.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "Background.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[BACKGROUND_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[BACKGROUND_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "Convolution.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "Convolution.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[CONVOLUTION_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[CONVOLUTION_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "SpecularConvolution.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "SpecularConvolution.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[PREFILTER_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[PREFILTER_SHADER].manifest = shaderManifest;
            
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "BRDF.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "BRDF.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[BRDF_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[BRDF_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "2D.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "2D.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[TWO_D_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath, "");
            s_shaders[TWO_D_SHADER].manifest = shaderManifest;
        }
#endif

        //SetupSkinnedModelDemo();
        SetupLearnOpenGLDemo();
    }
    
    void OpenGLDraw(float alpha)
    {
        //DrawSkinnedModelDemo();
        DrawLearnOpenGLDemo();
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
    
    void DrawCube(const opengl::Shader& shader, const glm::mat4& model)
    {
        opengl::Bind(g_boxVAO);
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        opengl::UnBind(g_boxVAO);
    }
    
    void DrawQuad(const opengl::Shader& shader, const glm::mat4& model)
    {
        opengl::Bind(g_quadVAO);
        SetupModelMat(shader, model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        opengl::UnBind(g_quadVAO);
    }
    
    void SetupLearnOpenGLDemo()
    {
//        char modelPath[r2::fs::FILE_PATH_LENGTH];
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::MODELS, "nanosuit/nanosuit.obj", modelPath);
//        LoadModel(g_nanosuitModel, modelPath);
//        s_openglMeshes = opengl::CreateOpenGLMeshesFromModel(g_nanosuitModel);
//
//        g_objectPositions.push_back(glm::vec3(-3.0,  -3.0, -3.0));
//        g_objectPositions.push_back(glm::vec3( 0.0,  -3.0, -3.0));
//        g_objectPositions.push_back(glm::vec3( 3.0,  -3.0, -3.0));
//        g_objectPositions.push_back(glm::vec3(-3.0,  -3.0,  0.0));
//        g_objectPositions.push_back(glm::vec3( 0.0,  -3.0,  0.0));
//        g_objectPositions.push_back(glm::vec3( 3.0,  -3.0,  0.0));
//        g_objectPositions.push_back(glm::vec3(-3.0,  -3.0,  3.0));
//        g_objectPositions.push_back(glm::vec3( 0.0,  -3.0,  3.0));
//        g_objectPositions.push_back(glm::vec3( 3.0,  -3.0,  3.0));
        
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
////
//        opengl::Create(g_planeVAO);
//        opengl::VertexBuffer planeVBO;
//        opengl::Create(planeVBO, {
//            {ShaderDataType::Float3, "aPos"},
//            {ShaderDataType::Float3, "aNormal"},
//            {ShaderDataType::Float2, "aTexCoord"}
//        }, planeVertices, COUNT_OF(planeVertices), GL_STATIC_DRAW);
//
//        opengl::AddBuffer(g_planeVAO, planeVBO);
//
//
        
        //calculate the tangent and bitanget
        // positions
//        glm::vec3 pos1(-1.0,  1.0, 0.0);
//        glm::vec3 pos2(-1.0, -1.0, 0.0);
//        glm::vec3 pos3( 1.0, -1.0, 0.0);
//        glm::vec3 pos4( 1.0,  1.0, 0.0);
//        // texture coordinates
//        glm::vec2 uv1(0.0, 1.0);
//        glm::vec2 uv2(0.0, 0.0);
//        glm::vec2 uv3(1.0, 0.0);
//        glm::vec2 uv4(1.0, 1.0);
//        // normal vector
//        glm::vec3 nm(0.0, 0.0, 1.0);
//
//        glm::vec3 tri1_edge1 = pos2 - pos1;
//        glm::vec3 tri1_edge2 = pos3 - pos1;
//        glm::vec2 tri1_deltaUV1 = uv2 - uv1;
//        glm::vec2 tri1_deltaUV2 = uv3 - uv1;
//
//        glm::vec3 tri2_edge1 = pos3 - pos1;
//        glm::vec3 tri2_edge2 = pos4 - pos1;
//        glm::vec2 tri2_deltaUV1 = uv3 - uv1;
//        glm::vec2 tri2_deltaUV2 = uv4 - uv1;
//
//        glm::vec3 tangent1, bitangent1, tangent2, bitangent2;
//
//        CalculateTangentAndBiTangent(tri1_edge1, tri1_edge2, tri1_deltaUV1, tri1_deltaUV2, tangent1, bitangent1);
//        CalculateTangentAndBiTangent(tri2_edge1, tri2_edge2, tri2_deltaUV1, tri2_deltaUV2, tangent2, bitangent2);
//
//        float newQuadVerts[] = {
//            // positions            // normal         // texcoords  // tangent                          // bitangent
//            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
//            pos2.x, pos2.y, pos2.z, nm.x, nm.y, nm.z, uv2.x, uv2.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
//            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent1.x, tangent1.y, tangent1.z, bitangent1.x, bitangent1.y, bitangent1.z,
//
//            pos1.x, pos1.y, pos1.z, nm.x, nm.y, nm.z, uv1.x, uv1.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
//            pos3.x, pos3.y, pos3.z, nm.x, nm.y, nm.z, uv3.x, uv3.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z,
//            pos4.x, pos4.y, pos4.z, nm.x, nm.y, nm.z, uv4.x, uv4.y, tangent2.x, tangent2.y, tangent2.z, bitangent2.x, bitangent2.y, bitangent2.z
//        };
//
//        opengl::Create(g_normalMappedQuadVAO);
//        opengl::VertexBuffer normalMappedQuadVBO;
//        opengl::Create(normalMappedQuadVBO, {
//            {ShaderDataType::Float3, "aPos"},
//            {ShaderDataType::Float3, "aNormal"},
//            {ShaderDataType::Float2, "aTexCoord"},
//            {ShaderDataType::Float3, "aTangent"},
//            {ShaderDataType::Float3, "aBiTangent"}
//        }, newQuadVerts, COUNT_OF(newQuadVerts), GL_STATIC_DRAW);
//
//        opengl::AddBuffer(g_normalMappedQuadVAO, normalMappedQuadVBO);
        
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
//        char path[r2::fs::FILE_PATH_LENGTH];
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "container2.png", path);
//
//        containerTexture = opengl::LoadImageTexture(path);
        
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
        
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "wood.png", path);
//        woodTexture = opengl::LoadImageTexture(path);
        
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "bricks2.jpg", path);
//        bricksTexture = opengl::LoadImageTexture(path);
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "bricks2_normal.jpg", path);
//        normalMappedBricksTexture = opengl::LoadImageTexture(path);
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "bricks2_disp.jpg", path);
//        bricksHeightMapTexture = opengl::LoadImageTexture(path);
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
      //  glEnable(GL_BLEND);
       // glEnable(GL_CULL_FACE);
       // glFrontFace(GL_CCW);
        
        //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        
        //setup framebuffers
        
//        opengl::Create(g_gBuffer, CENG.DisplaySize());
//        g_gBufferTextures[GBUFFER_POSITION] = opengl::AttachHDRTextureToFrameBuffer(g_gBuffer, GL_RGB16F, GL_NEAREST, GL_CLAMP_TO_EDGE);
//        g_gBufferTextures[GBUFFER_NORMAL] = opengl::AttachHDRTextureToFrameBuffer(g_gBuffer, GL_RGB16F, GL_NEAREST);
//        g_gBufferTextures[GBUFFER_ALBEDO_SPEC] = opengl::AttachTextureToFrameBuffer(g_gBuffer, true, GL_NEAREST);
//
//        opengl::Create(g_renderBuffer, CENG.DisplaySize().width, CENG.DisplaySize().height);
//        opengl::AttachDepthBufferForRenderBufferToFrameBuffer(g_gBuffer, g_renderBuffer);
//
//        unsigned int attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
//        opengl::Bind(g_gBuffer);
//        glDrawBuffers(3, attachments);
//        opengl::UnBind(g_gBuffer);
//
//        opengl::Create(g_ssaoFBO, CENG.DisplaySize());
//        g_ssaoColorBuffer = opengl::AttachHDRTextureToFrameBuffer(g_ssaoFBO, GL_RGB, GL_NEAREST);
//
//        opengl::Create(g_ssaoBlurFBO, CENG.DisplaySize());
//        g_ssaoBlurColorBuffer = opengl::AttachHDRTextureToFrameBuffer(g_ssaoBlurFBO, GL_RGB, GL_NEAREST);
//
//        std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
//        std::default_random_engine generator;
//
//        for (u32 i = 0; i < 64; ++i)
//        {
//            glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0,
//                             randomFloats(generator) * 2.0 - 1.0,
//                             randomFloats(generator));
//            sample  = glm::normalize(sample);
//            sample *= randomFloats(generator);
//            float scale = (float)i / 64.0;
//            float t = scale * scale;
//            scale = (1.0f - t)*0.1 + 1.0 * t;
//
//            ssaoKernel.push_back(sample);
//        }
//
//        std::vector<glm::vec3> ssaoNoise;
//        for (u32 i = 0; i < 16; ++i)
//        {
//            glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
//            ssaoNoise.push_back(noise);
//        }
//
//        g_ssaoNoiseTexture = opengl::CreateImageTexture(4, 4, GL_RGB32F, &ssaoNoise[0]);
        
        
       // opengl::Create(g_depthBuffer, 1024, 1024);
       // depthTexture = opengl::AttachDepthToFrameBuffer(g_depthBuffer);
        
      //  opengl::Create(g_frameBuffer, CENG.DisplaySize().width, CENG.DisplaySize().height);
        //textureColorBuffer = opengl::AttachTextureToFrameBuffer(g_frameBuffer);
//        opengl::Create(g_hdrFBO, CENG.DisplaySize().width, CENG.DisplaySize().height);
//        textureColorBuffer = opengl::AttachHDRTextureToFrameBuffer(g_hdrFBO);
//        textureColorBuffer2 = opengl::AttachHDRTextureToFrameBuffer(g_hdrFBO);
//
//        opengl::Create(g_renderBuffer, CENG.DisplaySize().width, CENG.DisplaySize().height);
//        opengl::AttachDepthAndStencilForRenderBufferToFrameBuffer(g_hdrFBO, g_renderBuffer);
//        opengl::Bind(g_hdrFBO);
//        unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
//        glDrawBuffers(2, attachments);
//        opengl::UnBind(g_hdrFBO);
//
//
//        opengl::Create(g_pingPongFBO[0], CENG.DisplaySize().width, CENG.DisplaySize().height);
//        pingPongTexture[0] = opengl::AttachHDRTextureToFrameBuffer(g_pingPongFBO[0]);
//
//        opengl::Create(g_pingPongFBO[1], CENG.DisplaySize().width, CENG.DisplaySize().height);
//        pingPongTexture[1] = opengl::AttachHDRTextureToFrameBuffer(g_pingPongFBO[1]);
        
//        opengl::Create(g_depthMapFBO, 1024, 1024);
//        depthCubeMap = opengl::CreateDepthCubeMap(g_depthMapFBO.width, g_depthMapFBO.height);
//        opengl::AttachDepthCubeMapToFrameBuffer(g_depthMapFBO, depthCubeMap);
        
        

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
        
//        const u32 NUM_LIGHTS = 32;
//        srand(13);
//        for (u32 i = 0; i < NUM_LIGHTS; ++i)
//        {
//            float xPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
//            float yPos = ((rand() % 100) / 100.0) * 6.0 - 4.0;
//            float zPos = ((rand() % 100) / 100.0) * 6.0 - 3.0;
//            g_lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
//
//            float rColor = ((rand() % 100) / 200.0f) + 0.5;
//            float gColor = ((rand() % 100) / 200.0f) + 0.5;
//            float bColor = ((rand() % 100) / 200.0f) + 0.5;
//            g_lightColors.push_back(glm::vec3(rColor, gColor, bColor));
//        }
        
//        s_shaders[LEARN_OPENGL_SHADER].UseShader();
//        s_shaders[LEARN_OPENGL_SHADER].SetUInt("diffuseTexture", 0);
//
//        s_shaders[BLUR_SHADER].UseShader();
//        s_shaders[BLUR_SHADER].SetUInt("image", 0);
        
//        s_shaders[LEARN_OPENGL_SHADER2].UseShader();
//        s_shaders[LEARN_OPENGL_SHADER2].SetUInt("gPosition", 0);
//        s_shaders[LEARN_OPENGL_SHADER2].SetUInt("gNormal", 1);
//        s_shaders[LEARN_OPENGL_SHADER2].SetUInt("gAlbedoSpec", 2);
//        s_shaders[LEARN_OPENGL_SHADER2].SetUInt("ssao", 3);
//
//        s_shaders[SSAO_SHADER].UseShader();
//        s_shaders[SSAO_SHADER].SetUInt("gPosition", 0);
//        s_shaders[SSAO_SHADER].SetUInt("gNormal", 1);
//        s_shaders[SSAO_SHADER].SetUInt("texNoise", 2);
//
//        s_shaders[BLUR_SHADER].UseShader();
//        s_shaders[BLUR_SHADER].SetUInt("ssaoInput", 0);
        
        //PBR demo
        
        MakeSphere(g_sphereVAO, 64);
        
        glDepthFunc(GL_LEQUAL);
        s_shaders[BACKGROUND_SHADER].UseShader();
        s_shaders[BACKGROUND_SHADER].SetUInt("environmentMap", 0);
        
        
        g_lightPositions.clear();
        g_lightPositions.push_back(glm::vec3(-10.0f, 10.0f, 10.0f));
        g_lightPositions.push_back(glm::vec3(10.0f, 10.0f, 10.0f));
        g_lightPositions.push_back(glm::vec3(-10.0f, -10.0f, 10.0f));
        g_lightPositions.push_back(glm::vec3(10.0f, -10.0f, 10.0f));
        
        g_lightColors.clear();
        g_lightColors.push_back(glm::vec3(300.0f, 300.f, 300.f));
        g_lightColors.push_back(glm::vec3(300.0f, 300.f, 300.f));
        g_lightColors.push_back(glm::vec3(300.0f, 300.f, 300.f));
        g_lightColors.push_back(glm::vec3(300.0f, 300.f, 300.f));
        
        
        
        char path[r2::fs::FILE_PATH_LENGTH];
        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "hdr/newport_loft.hdr", path);
        
        opengl::Create(g_captureFBO, 512, 512);
        opengl::Create(g_captureRBO, 512, 512);
        opengl::AttachDepthBufferForRenderBufferToFrameBuffer(g_captureFBO, g_captureRBO);
        
        hdrTexture = opengl::LoadHDRImage(path);
        
        envCubemap = opengl::CreateHDRCubeMap(g_captureRBO.width, g_captureRBO.height);
        
        
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };
        
        s_shaders[CUBEMAP_SHADER].UseShader();
        s_shaders[CUBEMAP_SHADER].SetUInt("equirectangularMap", 0);
        s_shaders[CUBEMAP_SHADER].SetUMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);

        glViewport(0, 0, g_captureRBO.width, g_captureRBO.height);
        opengl::Bind(g_captureFBO);
        for (u32 i = 0; i < 6; ++i)
        {
            s_shaders[CUBEMAP_SHADER].SetUMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            DrawCube(s_shaders[CUBEMAP_SHADER], glm::mat4(1.0));
        }
        opengl::UnBind(g_captureFBO);

        irradianceMapTexture = opengl::CreateHDRCubeMap(32, 32);

        g_captureFBO.width = 32;
        g_captureFBO.height = 32;
        g_captureRBO.width = 32;
        g_captureRBO.height = 32;

        opengl::Bind(g_captureFBO);
        opengl::Bind(g_captureRBO);

        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

        s_shaders[CONVOLUTION_SHADER].UseShader();
        s_shaders[CONVOLUTION_SHADER].SetUInt("environmentMap", 0);
        s_shaders[CONVOLUTION_SHADER].SetUMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);


        glViewport(0, 0, 32, 32);
        opengl::Bind(g_captureFBO);

        for (u32 i = 0; i < 6; ++i)
        {
            s_shaders[CONVOLUTION_SHADER].SetUMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMapTexture, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            DrawCube(s_shaders[CONVOLUTION_SHADER], glm::mat4(1.0));
        }
        opengl::UnBind(g_captureFBO);

        
        
        prefilteredMap = opengl::CreateHDRCubeMap(128, 128, true);
        
        s_shaders[PREFILTER_SHADER].UseShader();
        s_shaders[PREFILTER_SHADER].SetUInt("environmentMap", 0);
        s_shaders[PREFILTER_SHADER].SetUMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        
        const u32 prefilteredSize = 128;
        
        opengl::Create(g_prefiltedFBO, prefilteredSize, prefilteredSize);
        opengl::Create(g_prefilteredRBO, prefilteredSize, prefilteredSize);
        
        opengl::Bind(g_prefiltedFBO);
        
        u32 maxMipLevels = 5;
        for (u32 mip = 0; mip < maxMipLevels; ++mip)
        {
            u32 mipWidth = prefilteredSize * pow(0.5, mip);
            u32 mipHeight = prefilteredSize * pow(0.5, mip);
            
            opengl::Bind(g_prefilteredRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);
            
            float roughness = (float)mip / (float)(maxMipLevels - 1);
            s_shaders[PREFILTER_SHADER].SetUFloat("roughness", roughness);
            for (u32 i = 0; i < 6; ++i)
            {
                s_shaders[PREFILTER_SHADER].SetUMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilteredMap, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                DrawCube(s_shaders[PREFILTER_SHADER], glm::mat4(1.0f));
            }
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        brdfLUTTexture = opengl::CreateBRDFTexture(512, 512);
        g_captureFBO.width = 512;
        g_captureFBO.height = 512;
        g_captureRBO.width = 512;
        g_captureRBO.height = 512;
        opengl::Bind(g_captureFBO);
        opengl::Bind(g_captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, g_captureFBO.width, g_captureFBO.height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);
        glViewport(0, 0, g_captureFBO.width, g_captureFBO.height);
        
        s_shaders[BRDF_SHADER].UseShader();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        DrawQuad(s_shaders[BRDF_SHADER], glm::mat4(1.0));
        
        opengl::UnBind(g_captureFBO);
        

        glViewport(0, 0, g_width, g_height);
        
        s_shaders[BACKGROUND_SHADER].UseShader();
        s_shaders[BACKGROUND_SHADER].SetUInt("environmentMap", 0);
        
        s_shaders[PBR_SHADER].UseShader();
        s_shaders[PBR_SHADER].SetUInt("irradianceMap", 0);
        s_shaders[PBR_SHADER].SetUInt("prefilteredMap", 1);
        s_shaders[PBR_SHADER].SetUInt("brdfLUT", 2);
        s_shaders[PBR_SHADER].SetUVec3("albedo", glm::vec3(0.5f, 0.0f, 0.0f ));
        s_shaders[PBR_SHADER].SetUFloat("ao", 1.0f);
        
       // glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "pbr/rusted_iron/albedo.png", path);
//        albedoMapTexture = opengl::LoadImageTexture(path);
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "pbr/rusted_iron/normal.png", path);
//        normalMapTexture = opengl::LoadImageTexture(path);
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "pbr/rusted_iron/metallic.png", path);
//        metallicMapTexture = opengl::LoadImageTexture(path);
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "pbr/rusted_iron/roughness.png", path);
//        roughnessMapTexture = opengl::LoadImageTexture(path);
//
//        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "pbr/rusted_iron/ao.png", path);
//        aoMapTexture = opengl::LoadImageTexture(path);
        
       // s_shaders[PBR_SHADER].UseShader();

        
//        s_shaders[PBR_SHADER].SetUInt("albedoMap", 0);
//        s_shaders[PBR_SHADER].SetUInt("normalMap", 1);
//        s_shaders[PBR_SHADER].SetUInt("metallicMap", 2);
//        s_shaders[PBR_SHADER].SetUInt("roughnessMap", 3);
//        s_shaders[PBR_SHADER].SetUInt("aoMap", 4);
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
      //  lightPos.z = sin(timeVal * 0.5) * 3.0;
        
       // float aspect = (float)g_depthMapFBO.width / (float)g_depthMapFBO.height;
        float near = 1.0f;
        float far = 25.0f;
        
        //PBR Demo
        {
            s_shaders[PBR_SHADER].UseShader();
            
            glClearColor(0.5f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            
            SetupVP(s_shaders[PBR_SHADER], g_View, g_Proj);
            s_shaders[PBR_SHADER].SetUVec3("camPos", g_CameraPos);

            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMapTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, prefilteredMap);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, albedoMapTexture);
//            glActiveTexture(GL_TEXTURE1);
//            glBindTexture(GL_TEXTURE_2D, normalMapTexture);
//            glActiveTexture(GL_TEXTURE2);
//            glBindTexture(GL_TEXTURE_2D, metallicMapTexture);
//            glActiveTexture(GL_TEXTURE3);
//            glBindTexture(GL_TEXTURE_2D, roughnessMapTexture);
//            glActiveTexture(GL_TEXTURE4);
//            glBindTexture(GL_TEXTURE_2D, aoMapTexture);
            
            //render rows*column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
            glm::mat4 model = glm::mat4(1.0f);

            for (int row = 0; row < g_numRows; ++row)
            {
                float metallic = (float)row / (float)g_numRows;
                
                s_shaders[PBR_SHADER].SetUFloat("metallic", metallic);
                
              //  printf("row: %i, metallic: %f\n", row, metallic);
                
                for (int col = 0; col < g_numColumns; ++col)
                {
                    float roughness =  glm::clamp((float)col / (float)g_numColumns, 0.05f, 1.0f);
                    
                   s_shaders[PBR_SHADER].SetUFloat("roughness",roughness);
                    // printf("col: %i, roughness: %f\n", row, roughness);
                    
                    model = glm::mat4(1.0f);
                    model = glm::translate(model, glm::vec3(
                                           ((float)(col - g_numColumns/2.0f))*spacing,
                                           ((float)(row - g_numRows/2.0f))*spacing,
                                            -2.0f));

                    RenderSphere(s_shaders[PBR_SHADER], model);
                }
            }

            for (u32 i = 0; i < g_lightPositions.size(); ++i)
            {
                glm::vec3 newPos = g_lightPositions[i] + glm::vec3(sin(timeVal * 5.0) * 5.0, 0.0, 0.0);
                newPos = g_lightPositions[i];
                char uniformName[r2::fs::FILE_PATH_LENGTH];
                sprintf(uniformName, "lightPositions[%u]", i);
                s_shaders[PBR_SHADER].SetUVec3(uniformName, newPos);
                sprintf(uniformName, "lightColors[%u]", i);
                s_shaders[PBR_SHADER].SetUVec3(uniformName, g_lightColors[i]);

                model = glm::mat4(1.0f);
                model = glm::translate(model, newPos);
                model = glm::scale(model, glm::vec3(0.5f));

                RenderSphere(s_shaders[PBR_SHADER], model);
            }
            
            
            s_shaders[BACKGROUND_SHADER].UseShader();
            SetupVP(s_shaders[BACKGROUND_SHADER], g_View, g_Proj);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
            DrawCube(s_shaders[BACKGROUND_SHADER], glm::mat4(1.0));
            
        }
        
        
//        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, near, far);
//        std::vector<glm::mat4> shadowTransforms;
//
//        shadowTransforms.push_back(shadowProj *
//                                     glm::lookAt(lightPos, lightPos + glm::vec3( 1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
//        shadowTransforms.push_back(shadowProj *
//                                     glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0,-1.0, 0.0)));
//        shadowTransforms.push_back(shadowProj *
//                                     glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
//        shadowTransforms.push_back(shadowProj *
//                                     glm::lookAt(lightPos, lightPos + glm::vec3( 0.0,-1.0, 0.0), glm::vec3(0.0, 0.0,-1.0)));
//        shadowTransforms.push_back(shadowProj *
//                                     glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0, 1.0), glm::vec3(0.0,-1.0, 0.0)));
//        shadowTransforms.push_back(shadowProj *
//                                     glm::lookAt(lightPos, lightPos + glm::vec3( 0.0, 0.0,-1.0), glm::vec3(0.0,-1.0, 0.0)));
        {
//            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//            opengl::Bind(g_gBuffer);
//
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//            s_shaders[LEARN_OPENGL_SHADER].UseShader();
//            SetupVP(s_shaders[LEARN_OPENGL_SHADER], g_View, g_Proj);
//
//            //room cube
//            glm::mat4 model = glm::mat4(1.0);
//            model = glm::translate(model, glm::vec3(0.0, 7.0f, 0.0f));
//            model = glm::scale(model, glm::vec3(7.5f));
//            s_shaders[LEARN_OPENGL_SHADER].SetUBool("inverseNormals", true);
//
//            SetupModelMat(s_shaders[LEARN_OPENGL_SHADER], model);
//
//          //  glActiveTexture(GL_TEXTURE0);
//         //   glBindTexture(GL_TEXTURE_2D, woodTexture);
//
//            opengl::Bind(g_boxVAO);
//            DrawCube(s_shaders[LEARN_OPENGL_SHADER], model);
//
//            s_shaders[LEARN_OPENGL_SHADER].SetUBool("inverseNormals", false);
//
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 5.0f));
//            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
//            model = glm::scale(model, glm::vec3(0.5f));
//            SetupModelMat(s_shaders[LEARN_OPENGL_SHADER], model);
//            DrawOpenGLMeshes(s_shaders[LEARN_OPENGL_SHADER], s_openglMeshes);
//
//
//            opengl::UnBind(g_gBuffer);
//
//            //ssao texture
//            opengl::Bind(g_ssaoFBO);
//            glClear(GL_COLOR_BUFFER_BIT);
//            s_shaders[SSAO_SHADER].UseShader();
//            for (u32 i = 0; i < 64; ++i)
//            {
//                char uniformName[r2::fs::FILE_PATH_LENGTH];
//                sprintf(uniformName, "samples[%i]", i);
//                s_shaders[SSAO_SHADER].SetUVec3(uniformName, ssaoKernel[i]);
//            }
//
//            s_shaders[SSAO_SHADER].SetUMat4("projection", g_Proj);
//
//            for (u32 i = 0; i <= GBUFFER_NORMAL; ++i)
//            {
//                glActiveTexture(GL_TEXTURE0 + i);
//                glBindTexture(GL_TEXTURE_2D, g_gBufferTextures[i]);
//            }
//            glActiveTexture(GL_TEXTURE2);
//            glBindTexture(GL_TEXTURE_2D, g_ssaoNoiseTexture);
//            opengl::Bind(g_quadVAO);
//            glDrawArrays(GL_TRIANGLES, 0, 6);
//
//            opengl::UnBind(g_ssaoFBO);
//
//            //ssao blur
//            opengl::Bind(g_ssaoBlurFBO);
//            glClear(GL_COLOR_BUFFER_BIT);
//            s_shaders[BLUR_SHADER].UseShader();
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, g_ssaoColorBuffer);
//            opengl::Bind(g_quadVAO);
//            glDrawArrays(GL_TRIANGLES, 0, 6);
//            opengl::UnBind(g_ssaoBlurFBO);
//
//            //lighting pass for deferred shading
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//            s_shaders[LEARN_OPENGL_SHADER2].UseShader();
//
//            const float linear = 0.09;
//            const float quadratic = 0.032;
//            const float constant = 1.0f;
//
//            glm::vec3 lightPosView = glm::vec3(g_View * glm::vec4(lightPos, 1.0));
//            s_shaders[LEARN_OPENGL_SHADER2].SetUVec3("light.Position", lightPosView);
//            s_shaders[LEARN_OPENGL_SHADER2].SetUVec3("light.Color", lightColor);
//            s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("light.Linear", linear);
//            s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("light.Quadratic", quadratic);
//            for (u32 i = 0; i < NUM_GBUFFER_BUFFERS; ++i)
//            {
//                glActiveTexture(GL_TEXTURE0 + i);
//                glBindTexture(GL_TEXTURE_2D, g_gBufferTextures[i]);
//            }
//            glActiveTexture(GL_TEXTURE0 + NUM_GBUFFER_BUFFERS);
//            glBindTexture(GL_TEXTURE_2D, g_ssaoColorBuffer);
//            opengl::Bind(g_quadVAO);
//            glDrawArrays(GL_TRIANGLES, 0, 6);
            
//            for (u32 i = 0; i < g_lightPositions.size(); ++i)
//            {
//                float lightMax = std::fmaxf(std::fmaxf(g_lightColors[i].r, g_lightColors[i].g), g_lightColors[i].b);
//                float radius = (-linear + std::sqrtf(linear * linear - 4 * quadratic * (constant - (256.0/5.0) * lightMax))) / (2 * quadratic);
//
//                char uniformName[r2::fs::FILE_PATH_LENGTH];
//                sprintf(uniformName, "lights[%i].Position", i);
//
//                s_shaders[LEARN_OPENGL_SHADER2].SetUVec3(uniformName, g_lightPositions[i]);
//
//                sprintf(uniformName, "lights[%i].Color", i);
//
//                s_shaders[LEARN_OPENGL_SHADER2].SetUVec3(uniformName, g_lightColors[i]);
//
//                sprintf(uniformName, "lights[%i].Linear", i);
//
//                s_shaders[LEARN_OPENGL_SHADER2].SetUFloat(uniformName, linear);
//
//                sprintf(uniformName, "lights[%i].Quadtric", i);
//
//                s_shaders[LEARN_OPENGL_SHADER2].SetUFloat(uniformName, quadratic);
//
//                sprintf(uniformName, "lights[%i].Radius", i);
//
//                s_shaders[LEARN_OPENGL_SHADER2].SetUFloat(uniformName, radius);
//
//            }
//            s_shaders[LEARN_OPENGL_SHADER2].SetUVec3("viewPos", g_CameraPos);
//
//            opengl::Bind(g_quadVAO);
//            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            
//            glBindFramebuffer(GL_READ_FRAMEBUFFER, g_gBuffer.FBO);
//            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
//            glBlitFramebuffer(
//                              0, 0, CENG.DisplaySize().width, CENG.DisplaySize().height, 0, 0, CENG.DisplaySize().width, CENG.DisplaySize().height, GL_DEPTH_BUFFER_BIT, GL_NEAREST
//                              );
//            glBindFramebuffer(GL_FRAMEBUFFER, 0);
//
//            s_shaders[LIGHTBOX_SHADER].UseShader();
//            SetupVP(s_shaders[LIGHTBOX_SHADER], g_View, g_Proj);
//
//            opengl::Bind(g_boxVAO);
//
//            for (u32 i = 0; i < g_lightPositions.size(); ++i)
//            {
//                model = glm::mat4(1.0f);
//                model = glm::translate(model, g_lightPositions[i]);
//                model = glm::scale(model, glm::vec3(0.25));
//                s_shaders[LIGHTBOX_SHADER].SetUVec3("lightColor", g_lightColors[i]);
//                DrawCube(s_shaders[LIGHTBOX_SHADER], model);
//            }
            //glViewport(0, 0, g_frameBuffer.width, g_frameBuffer.height);
            
//
//            s_shaders[LEARN_OPENGL_SHADER].UseShader();
//            SetupVP(s_shaders[LEARN_OPENGL_SHADER], g_View, g_Proj);
//            s_shaders[LEARN_OPENGL_SHADER].SetUVec3("viewPos", g_CameraPos);
//
//            for (u32 i = 0; i < g_lightPositions.size(); ++i)
//            {
//                char paramStr[512];
//                sprintf(paramStr, (std::string("lights") + std::string("[%u]") + std::string(".Position")).c_str(), i);
//
//                s_shaders[LEARN_OPENGL_SHADER].SetUVec3(paramStr, g_lightPositions[i]);
//                sprintf(paramStr, (std::string("lights") + std::string("[%u]") + std::string(".Color")).c_str(), i);
//
//                s_shaders[LEARN_OPENGL_SHADER].SetUVec3(paramStr, g_lightColors[i]);
//            }
//
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, woodTexture);
//            opengl::Bind(g_boxVAO);
//
//            glm::mat4 model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
//            model = glm::scale(model, glm::vec3(12.5f, 0.5f, 12.5f));
//            DrawCube(s_shaders[LEARN_OPENGL_SHADER], model);
//
//            glBindTexture(GL_TEXTURE_2D, containerTexture);
//
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0));
//            model = glm::scale(model, glm::vec3(0.5f));
//            DrawCube(s_shaders[LEARN_OPENGL_SHADER], model);
//
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(2.0f, 0.0f, 1.0f));
//            model = glm::scale(model, glm::vec3(0.5f));
//            DrawCube(s_shaders[LEARN_OPENGL_SHADER], model);
//
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(-1.0f, -1.0f, 2.0f));
//            model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)));
//            DrawCube(s_shaders[LEARN_OPENGL_SHADER], model);
//
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(0.0f, 2.7f, 4.0f));
//            model = glm::rotate(model, glm::radians(23.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)));
//            model = glm::scale(model, glm::vec3(1.25));
//            DrawCube(s_shaders[LEARN_OPENGL_SHADER], model);
//
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(-2.0f, 1.0f, -3.0f));
//            model = glm::rotate(model, glm::radians(124.0f), glm::normalize(glm::vec3(1.0f, 0.0, 1.0f)));
//            DrawCube(s_shaders[LEARN_OPENGL_SHADER], model);
//
//            model = glm::mat4(1.0f);
//            model = glm::translate(model, glm::vec3(-3.0f, 0.0f, 0.0f));
//            model = glm::scale(model, glm::vec3(0.5f));
//            DrawCube(s_shaders[LEARN_OPENGL_SHADER], model);
//
//            s_shaders[LIGHTBOX_SHADER].UseShader();
//
//            SetupVP(s_shaders[LIGHTBOX_SHADER], g_View, g_Proj);
//
//            for (u32 i = 0; i < g_lightPositions.size(); ++i)
//            {
//                model = glm::mat4(1.0f);
//                model = glm::translate(model, glm::vec3(g_lightPositions[i]));
//                model = glm::scale(model, glm::vec3(0.25f));
//                SetupModelMat(s_shaders[LIGHTBOX_SHADER], model);
//                s_shaders[LIGHTBOX_SHADER].SetUVec3("lightColor", g_lightColors[i]);
//                DrawCube(s_shaders[LIGHTBOX_SHADER], model);
//            }
//
//            opengl::UnBind(g_hdrFBO);
//
//            //blur bright fragments with two-pass guassian blur
//            bool horizontal = true, firstIteration = true;
//
//            u32 amount = 10;
//            s_shaders[BLUR_SHADER].UseShader();
//            opengl::Bind(g_quadVAO);
//            for (u32 i = 0; i < amount; ++i)
//            {
//                opengl::Bind(g_pingPongFBO[horizontal]);
//                s_shaders[BLUR_SHADER].SetUInt("horizontal", horizontal);
//                glBindTexture(GL_TEXTURE_2D, firstIteration ? textureColorBuffer2 : pingPongTexture[!horizontal]);
//                glDrawArrays(GL_TRIANGLES, 0, 6);
//                horizontal = !horizontal;
//                if (firstIteration)
//                {
//                    firstIteration = false;
//                }
//            }
//            opengl::UnBind(g_pingPongFBO[0]);
//
//            glDisable(GL_DEPTH_TEST);
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//            s_shaders[LEARN_OPENGL_SHADER2].UseShader();
//
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
//            glActiveTexture(GL_TEXTURE1);
//            glBindTexture(GL_TEXTURE_2D, pingPongTexture[!horizontal]);
//            s_shaders[LEARN_OPENGL_SHADER2].SetUInt("bloom", true);
//            s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("exposure", 0.5f);
//            glDrawArrays(GL_TRIANGLES, 0, 6);
//            glEnable(GL_DEPTH_TEST);
//            s_shaders[DEPTH_CUBE_MAP_SHADER].UseShader();
//            //glm::mat4 lightProjection, lightView;
//
//            //lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
//            //lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
//            //SetupVP(s_shaders[DEPTH_SHADER], lightView, lightProjection);
//
//            glViewport(0, 0, g_depthMapFBO.width, g_depthMapFBO.height);
//            opengl::Bind(g_depthMapFBO);
//            glClear(GL_DEPTH_BUFFER_BIT);
//            SetupArrayMats(s_shaders[DEPTH_CUBE_MAP_SHADER], "shadowMatrices", shadowTransforms);
//            s_shaders[DEPTH_CUBE_MAP_SHADER].SetUVec3("lightPos", lightPos);
//            s_shaders[DEPTH_CUBE_MAP_SHADER].SetUFloat("far_plane", far);
//            DrawScene(s_shaders[DEPTH_CUBE_MAP_SHADER]);
//
//            //opengl::UnBind(g_depthBuffer);
//
//            opengl::Bind(g_frameBuffer);
//
//            glViewport(0, 0, g_width, g_height);
//            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//            s_shaders[LEARN_OPENGL_SHADER].UseShader();
//
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, woodTexture);
//            glActiveTexture(GL_TEXTURE1);
//            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
//            SetupVP(s_shaders[LEARN_OPENGL_SHADER], g_View, g_Proj);
//            //s_shaders[LEARN_OPENGL_SHADER].SetUMat4("lightSpaceMatrix", lightProjection * lightView);
//            s_shaders[LEARN_OPENGL_SHADER].SetUVec3("lightPos", lightPos);
//            s_shaders[LEARN_OPENGL_SHADER].SetUVec3("viewPos", g_CameraPos);
//            s_shaders[LEARN_OPENGL_SHADER].SetUFloat("far_plane", far);
//            DrawScene(s_shaders[LEARN_OPENGL_SHADER]);
//            opengl::UnBind(g_frameBuffer);
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
//            glDisable(GL_DEPTH_TEST);
//            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
//            glClear(GL_COLOR_BUFFER_BIT);
//
//            s_shaders[LEARN_OPENGL_SHADER2].UseShader();
//            glActiveTexture(GL_TEXTURE0);
//            glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
//           // s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("near_plane", near);
//           // s_shaders[LEARN_OPENGL_SHADER2].SetUFloat("far_plane", far);
//            opengl::Bind(g_quadVAO);
//            //Use the texture that was drawn to in the previous pass
//
//            glDrawArrays(GL_TRIANGLES, 0, 6);
//            glEnable(GL_DEPTH_TEST);
        }
        
    }
    
    void CalculateTangentAndBiTangent(const glm::vec3& edge1, const glm::vec3& edge2, const glm::vec2& deltaUV1, const glm::vec2& deltaUV2, glm::vec3& tangent, glm::vec3& bitangent)
    {
        float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        R2_CHECK(!r2::math::NearEq( denom, 0.0), "denominator is 0!");
        
        float f = 1.0f / denom;
        
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent = glm::normalize(tangent);
        
        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent = glm::normalize(bitangent);
    }
    
    void RenderSphere(const opengl::Shader& shader, const glm::mat4& model)
    {
        SetupModelMat(shader, model);
        opengl::Bind(g_sphereVAO);
        glDrawElements(GL_TRIANGLE_STRIP, g_sphereVAO.indexBuffer.size, GL_UNSIGNED_INT, 0);
        opengl::UnBind(g_sphereVAO);
    }
    
    void MakeSphere(opengl::VertexArrayBuffer& vao, u32 segments)
    {
        opengl::Create(vao);
        opengl::VertexBuffer vbo;
        opengl::IndexBuffer ibo;
        
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<u32> indices;
        const float PI = 3.14159265359;
        
        for (u32 y = 0; y <= segments; ++y)
        {
            for (u32 x = 0; x <= segments; ++x)
            {
                float xSegment = (float)x / (float)segments;
                float ySegment = (float)y / (float)segments;
                float xPos = cos(xSegment * 2.0f * PI) * sin(ySegment * PI);
                float yPos = cos(ySegment * PI);
                float zPos = sin(xSegment * 2.0f * PI) * sin(ySegment * PI);
                
                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }
        
        bool oddRow = false;
        for (int y = 0; y < segments; ++y)
        {
            if(!oddRow)
            {
                for (int x = 0; x <= segments; ++x)
                {
                    indices.push_back(y * (segments + 1) + x);
                    indices.push_back((y + 1) * (segments + 1) + x);
                }
            }
            else
            {
                for (int x = segments; x >= 0; --x)
                {
                    indices.push_back((y+1) * (segments + 1) + x);
                    indices.push_back(y * (segments + 1) + x);
                }
            }
            oddRow = !oddRow;
        }

        std::vector<float> data;
        for (int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            
            if(normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if(uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        
        opengl::Create(vbo, {
            {ShaderDataType::Float3, "aPos"},
            {ShaderDataType::Float3, "aNormal"},
            {ShaderDataType::Float2, "aTexCoord"}
        }, &data[0], data.size(), GL_STATIC_DRAW);
        
        opengl::Create(ibo, &indices[0], indices.size(), GL_STATIC_DRAW);
        
        opengl::AddBuffer(vao, vbo);
        opengl::SetIndexBuffer(vao, ibo);
        
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
