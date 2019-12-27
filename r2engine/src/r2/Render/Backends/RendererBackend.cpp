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

namespace
{

    
    enum
    {
        LIGHTING_SHADER = 0,
        LAMP_SHADER,
        DEBUG_SHADER,
        OUTLINE_SHADER,
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
    
    glm::vec3 pointLightPositions[] = {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };
    
    glm::mat4 g_View = glm::mat4(1.0f);
    glm::mat4 g_Proj = glm::mat4(1.0f);
    glm::vec3 g_CameraPos = glm::vec3(0);
    glm::vec3 g_CameraDir = glm::vec3(0);
    glm::vec3 lightPos(1.2f, 0.2f, 2.0f);
    
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
    
}

namespace r2::draw
{

    
    void SetupLighting(const opengl::Shader& shader);
    void SetupMVP(const opengl::Shader& shader, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj);
    void SetupVP(const opengl::Shader& shader, const glm::mat4& view, const glm::mat4& proj);
    void SetupModelMat(const opengl::Shader& shader, const glm::mat4& model);
    
    void SetupBoneMats(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats);
    
    void DrawOpenGLMesh(const opengl::Shader& shader, const opengl::OpenGLMesh& mesh);
    void DrawOpenGLMeshes(const opengl::Shader& shader, const std::vector<opengl::OpenGLMesh>& meshes);
    
    void DrawSkinnedModel(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats);
    void DrawSkinnedModelOutline(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats);
    
    void DrawSkinnedModelDemo();
    void SetupSkinnedModelDemo();
    
    void SetupLearnOpenGLDemo();
    void DrawLearnOpenGLDemo();
    
    void OpenGLInit()
    {
        
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        
        
        char vertexPath[r2::fs::FILE_PATH_LENGTH];
        char fragmentPath[r2::fs::FILE_PATH_LENGTH];
        
        s_shaders.resize(NUM_SHADERS, opengl::Shader());
        
#ifdef R2_ASSET_PIPELINE
        //load the shader pipeline assets
        {
            r2::asset::pln::ShaderManifest shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lighting.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lighting.fs", fragmentPath);

            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);

            s_shaders[LIGHTING_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath);
            s_shaders[LIGHTING_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lamp.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lamp.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[LAMP_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath);
            s_shaders[LAMP_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "debug.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "debug.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[DEBUG_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath);
            s_shaders[DEBUG_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "SkeletonOutline.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "SkeletonOutline.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[OUTLINE_SHADER].shaderProg = opengl::CreateShaderProgramFromRawFiles(vertexPath, fragmentPath);
            s_shaders[OUTLINE_SHADER].manifest = shaderManifest;
        }
#endif

        SetupSkinnedModelDemo();
    }
    
    void OpenGLDraw(float alpha)
    {
        DrawSkinnedModelDemo();
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
    
    void DrawSkinnedModel(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats)
    {
        SetupLighting(shader);
        
        float timeVal = static_cast<float>(CENG.GetTicks()) / 1000.f;
        
        shader.SetUFloat("time", timeVal);
        shader.SetUVec3("viewPos", g_CameraPos);
        
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.01f));
        
        SetupMVP(shader, modelMat, g_View, g_Proj);
        
        SetupBoneMats(shader, boneMats);
        
        DrawOpenGLMeshes(shader, s_openglMeshes);
    }
    
    void DrawSkinnedModelOutline(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats)
    {
        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        modelMat = glm::scale(modelMat, glm::vec3(0.01f));
        
        SetupMVP(shader, modelMat, g_View, g_Proj);
        SetupBoneMats(shader, boneMats);
        
        shader.SetUVec4("outlineColor", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

        DrawOpenGLMeshes(shader, s_openglMeshes);
    }

    void SetupBoneMats(const opengl::Shader& shader, const std::vector<glm::mat4>& boneMats)
    {
        for (u32 i = 0; i < boneMats.size(); ++i)
        {
            char boneTransformsStr[512];
            sprintf(boneTransformsStr, "boneTransformations[%u]", i);
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
        
        glm::vec3 diffuseColor = glm::vec3(0.8f);
        glm::vec3 ambientColor = glm::vec3(0.05f);
        glm::vec3 specularColor = glm::vec3(1.0f);
        
        float attenConst = 1.0f;
        float attenLinear = 0.09f;
        float attenQuad = 0.032f;
        //directional light setup
        {
            shader.SetUVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
            shader.SetUVec3("dirLight.light.ambient", glm::vec3(0.05f));
            shader.SetUVec3("dirLight.light.diffuse", glm::vec3(0.4f));
            shader.SetUVec3("dirLight.light.specular", glm::vec3(0.5f));
        }
        
        //point light setup
        {
            for (u32 i = 0; i < COUNT_OF(pointLightPositions); ++i)
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
        
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        
        u32 whiteColor = 0xffffffff;
        defaultTexture = opengl::OpenGLCreateImageTexture(1, 1, &whiteColor);
    }
    
    void DrawSkinnedModelDemo()
    {
        glStencilMask(0xFF);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        std::vector<glm::mat4> boneMats = r2::draw::PlayAnimationForSkinnedModel(CENG.GetTicks(),g_Model, g_ModelAnimation);
        
        //Draw a cube
        {
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            s_shaders[DEBUG_SHADER].UseShader();
            opengl::Bind(g_LampVAO);
            glm::mat4 model = glm::mat4(1.0f);
            
            model = glm::translate(model, glm::vec3(0.f, -3.0f, 2.f));
            model = glm::scale(model, glm::vec3(1.0f));
            
            SetupMVP(s_shaders[DEBUG_SHADER], model, g_View, g_Proj);
            
            s_shaders[DEBUG_SHADER].SetUVec4("debugColor", glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
            
            glDrawElements(GL_TRIANGLES, COUNT_OF(indices), GL_UNSIGNED_INT, 0);
        }
        
        //Draw the model
        {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(0xFF);
            
            glUseProgram(s_shaders[LIGHTING_SHADER].shaderProg);
            
            DrawSkinnedModel(s_shaders[LIGHTING_SHADER], boneMats);
        }
        
        //draw object if it's behind
        {
            
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glDisable(GL_DEPTH_TEST);
            
            glUseProgram(s_shaders[OUTLINE_SHADER].shaderProg);
            
            DrawSkinnedModelOutline(s_shaders[OUTLINE_SHADER], boneMats);
            glEnable(GL_DEPTH_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
        }
        
        //Draw lamp
        {
            
            s_shaders[LAMP_SHADER].UseShader();

            SetupVP(s_shaders[LAMP_SHADER], g_View, g_Proj);
            
            opengl::Bind(g_LampVAO);
            
            
            for (u32 i = 0; i < 2; ++i)
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
    }
    
#ifdef R2_ASSET_PIPELINE
    void ReloadShader(const r2::asset::pln::ShaderManifest& manifest)
    {
        for (auto& shaderAsset : s_shaders)
        {
            if (shaderAsset.manifest.vertexShaderPath == manifest.vertexShaderPath &&
                shaderAsset.manifest.fragmentShaderPath == manifest.fragmentShaderPath)
            {
                opengl::ReloadShaderProgramFromRawFiles(&shaderAsset.shaderProg, shaderAsset.manifest.vertexShaderPath.c_str(), shaderAsset.manifest.fragmentShaderPath.c_str());
            }
        }
    }
#endif
}
