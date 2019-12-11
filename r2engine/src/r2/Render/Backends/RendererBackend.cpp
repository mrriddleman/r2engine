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
#include "stb_image.h"
#include "r2/Core/File/PathUtils.h"
#include "r2/Render/Camera/Camera.h"
#include "r2/Core/Math/Ray.h"

//For loading shader files
#include "r2/Core/File/FileSystem.h"
#include "r2/Core/File/File.h"
#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Memory/InternalEngineMemory.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#ifdef R2_ASSET_PIPELINE
#include "r2/Core/Assets/Pipeline/ShaderManifest.h"
#endif

namespace
{
    struct Shader
    {
        u32 shaderProg = 0;
#ifdef R2_ASSET_PIPELINE
        r2::asset::pln::ShaderManifest manifest;
#endif
    };
    
    enum
    {
        LIGHTING_SHADER = 0,
        LAMP_SHADER,
        DEBUG_SHADER
    };
    
    //@Temp
    std::vector<Shader> s_shaders;
    
    float cubeVerts[] = {
        //front face
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, //top right
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, //bottom right
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, //bottom left
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top left
        
        //right face
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, //top left 4
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //bottom left 5
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, //bottom right 6
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, // top right 7
        
        //left face
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, //bottom  left 8
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, //top left 9
        -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, //bottom right 10
        -0.5f, 0.5f, 0.5f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, //top right 11
        
        //back face
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, //bottom left 12
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, //top left 13
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, //bottom right 14
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, //top right 15
        
        //top face
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0, 1.0f, 0.0f, //bottom left 16
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //top left 17
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, //bottom right 18
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, //top right 19
        
        //bottom face
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, //bottom left 20
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, //top left 21
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, //bottom right 22
        0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f //top right 23
        
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
    
    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 texCoord;
        glm::vec3 normal;
    };
    
    std::vector<DebugVertex> g_debugVerts;
    
    u32 g_VBO, g_VAO, g_EBO, diffuseMap, specularMap, emissionMap, g_DebugVAO, g_DebugVBO, defaultTexture, g_lightVAO;
}

namespace r2::draw
{
    u32 CreateShaderProgramFromStrings(const char* vertexShaderStr, const char* fragShaderStr);
    u32 CreateShaderProgramFromRawFiles(const char* vertexShaderFilePath, const char* fragmentShaderFilePath);
    void ReloadShaderProgramFromRawFiles(u32* program, const char* vertexShaderFilePath, const char* fragmentShaderFilePath);
    
    
    u32 LoadImageTexture(const char* path);
    u32 CreateImageTexture(u32 width, u32 height, void* data);
    
    void OpenGLInit()
    {
        stbi_set_flip_vertically_on_load(true);
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        
        char vertexPath[r2::fs::FILE_PATH_LENGTH];
        char fragmentPath[r2::fs::FILE_PATH_LENGTH];
        
        s_shaders.push_back(Shader());
        s_shaders.push_back(Shader());
        s_shaders.push_back(Shader());
        
#ifdef R2_ASSET_PIPELINE
        //load the shader pipeline assets
        {
            r2::asset::pln::ShaderManifest shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lighting.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lighting.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);

            s_shaders[LIGHTING_SHADER].shaderProg = CreateShaderProgramFromRawFiles(vertexPath, fragmentPath);
            s_shaders[LIGHTING_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lamp.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "lamp.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[LAMP_SHADER].shaderProg = CreateShaderProgramFromRawFiles(vertexPath, fragmentPath);
            s_shaders[LAMP_SHADER].manifest = shaderManifest;
            
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "debug.vs", vertexPath);
            r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::SHADERS_RAW, "debug.fs", fragmentPath);
            
            shaderManifest.vertexShaderPath = std::string(vertexPath);
            shaderManifest.fragmentShaderPath = std::string(fragmentPath);
            
            s_shaders[DEBUG_SHADER].shaderProg = CreateShaderProgramFromRawFiles(vertexPath, fragmentPath);
            s_shaders[DEBUG_SHADER].manifest = shaderManifest;
        }
#endif

        //basic object stuff
        {
            glGenVertexArrays(1, &g_VAO);
            glGenBuffers(1, &g_VBO);
            glGenBuffers(1, &g_EBO);
            
            glBindVertexArray(g_VAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
            
            //position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
            glEnableVertexAttribArray(0);
            
            //uv attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3*sizeof(float)));
            glEnableVertexAttribArray(1);
            
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(5*sizeof(float)));
            glEnableVertexAttribArray(2);
            
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            
            glUseProgram(s_shaders[LIGHTING_SHADER].shaderProg);
            
            //how we set our sampler2Ds in our shaders - note we set our shader program above
            //First is set to Texture0 - this means material.diffuse is set to Texture0
            glUniform1i(glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "material.diffuse"), 0);
            //Second is set to Texture1 - this means material.specular is set to Texture1
            glUniform1i(glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "material.specular"), 1);
            
            glUniform1i(glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "material.emission"), 2);
            
            glUseProgram(0);
        }
        
        //lamp setup
        {
            glGenVertexArrays(1, &g_lightVAO);
            glad_glBindVertexArray(g_lightVAO);
            glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
            
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }
        
        //debug setup
        {
            glGenVertexArrays(1, &g_DebugVAO);
            glGenBuffers(1, &g_DebugVBO);
            
            glBindVertexArray(g_DebugVAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, g_DebugVBO);
            glBufferData(GL_ARRAY_BUFFER, 100 * 3 * 2 * sizeof(float), nullptr, GL_STREAM_DRAW);
            
            //position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex), (void*)0);
            glEnableVertexAttribArray(0);
            
            glBindVertexArray(0);
        }

        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glEnable(GL_DEPTH_TEST);
        
        u32 whiteColor = 0xffffffff;
        defaultTexture = CreateImageTexture(1, 1, &whiteColor);
        
        char path[r2::fs::FILE_PATH_LENGTH];
        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "container2.png", path);
        //Creates and sets the image to the textureID
        diffuseMap = LoadImageTexture(path);
        
        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "container2_specular.png", path);
        //Same here
        specularMap = LoadImageTexture(path);
        
        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "matrix.jpg", path);
        
        emissionMap = LoadImageTexture(path);
    }
    
    void OpenGLDraw(float alpha)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        //Draw the cube
        {
            glUseProgram(s_shaders[LIGHTING_SHADER].shaderProg);
            
            //We need to activate the texture slot
            glActiveTexture(GL_TEXTURE0);
            //then bind the textureID we desire
            glBindTexture(GL_TEXTURE_2D, diffuseMap);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, specularMap);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, emissionMap);
            
            glBindVertexArray(g_VAO);
            
            float timeVal = static_cast<float>(CENG.GetTicks()) / 1000.f;
            
            int uniformTimeLocation = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "time");
            glUniform1f(uniformTimeLocation, timeVal);
            //
            int viewLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "view");
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(g_View));
            //
            int projectionLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "projection");
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(g_Proj));
            
//            int lightPosLoc = glGetUniformLocation(g_ShaderProg, "lightPos");
//
//            glm::mat4 lightMat = glm::mat4(1.0f);
//
//            float dt = float(CPLAT.TickRate()) / 1000.f;
//            lightMat = glm::rotate(lightMat, 1*dt, glm::vec3(0,1,0));
//
//            lightPos = glm::vec3(lightMat * glm::vec4(lightPos,0.0));
//
//            glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
            
            int viewPosLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "viewPos");
            glUniform3fv(viewPosLoc, 1, glm::value_ptr(g_CameraPos));
            
          //  glm::mat4 model = glm::mat4(1.0f);
            int modelLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "model");
           
            
            int materialShininessLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "material.shininess");
            glUniform1f(materialShininessLoc, 64.0f);
            
            glm::vec3 diffuseColor = glm::vec3(0.8f);
            glm::vec3 ambientColor = glm::vec3(0.05f);
            glm::vec3 specularColor = glm::vec3(1.0f);
            
            float attenConst = 1.0f;
            float attenLinear = 0.09f;
            float attenQuad = 0.032f;
            //directional light setup
            {
                int dirLightDirLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "dirLight.direction");
                
                glUniform3fv(dirLightDirLoc, 1, glm::value_ptr(glm::vec3(-0.2f, -1.0f, -0.3f)));
                
                int dirLightAmbientLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "dirLight.light.ambient");
                glUniform3fv(dirLightAmbientLoc, 1, glm::value_ptr(glm::vec3(0.05f)));
                
                int dirLightDiffuseLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "dirLight.light.diffuse");
                glUniform3fv(dirLightDiffuseLoc, 1, glm::value_ptr(glm::vec3(0.4f)));
                
                int dirLightSpecularLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "dirLight.light.specular");
                glUniform3fv(dirLightSpecularLoc, 1, glm::value_ptr(glm::vec3(0.5f)));
            }
            
            //point light setup
            {
                for (u32 i = 0; i < COUNT_OF(pointLightPositions); ++i)
                {
                    char pointLightStr[512];
                    sprintf(pointLightStr, "pointLights[%i].position", i);
                    int pointLightPositionLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, pointLightStr);
                    
                    glUniform3fv(pointLightPositionLoc, 1, glm::value_ptr(pointLightPositions[i]));
                    
                    sprintf(pointLightStr, "pointLights[%i].attenuationState.constant", i);
                    int pointLightConstantLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, pointLightStr);
                    
                    glUniform1f(pointLightConstantLoc, attenConst);
                    
                    sprintf(pointLightStr, "pointLights[%i].attenuationState.linear", i);
                    int pointLightLinearLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, pointLightStr);
                    
                    glUniform1f(pointLightLinearLoc, attenLinear);
                    
                    sprintf(pointLightStr, "pointLights[%i].attenuationState.quadratic", i);
                    int pointLightQuadLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, pointLightStr);
                    
                    glUniform1f(pointLightQuadLoc, attenQuad);
                    
                    sprintf(pointLightStr, "pointLights[%i].light.ambient", i);
                    int pointLightAmbientLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, pointLightStr);
                    
                    glUniform3fv(pointLightAmbientLoc, 1, glm::value_ptr(ambientColor));
                    
                    sprintf(pointLightStr, "pointLights[%i].light.diffuse", i);
                    int pointLightDiffuseLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, pointLightStr);
                    
                    glUniform3fv(pointLightDiffuseLoc, 1, glm::value_ptr(diffuseColor));
                    
                    sprintf(pointLightStr, "pointLights[%i].light.specular", i);
                    int pointLightSpecularLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, pointLightStr);
                    
                    glUniform3fv(pointLightSpecularLoc, 1, glm::value_ptr(specularColor));
                    
                }
            }
            
            //spotlight setup
            {
                int lightPositionLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.position");
                glUniform3fv(lightPositionLoc, 1, glm::value_ptr(g_CameraPos));
                
                int lightDirectionLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.direction");
                glUniform3fv(lightDirectionLoc, 1, glm::value_ptr(g_CameraDir));
                
                int spotLightAmbientLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.light.ambient");
                glUniform3fv(spotLightAmbientLoc, 1, glm::value_ptr(glm::vec3(0.0f)));
                
                int spotLightDiffuseLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.light.diffuse");
                glUniform3fv(spotLightDiffuseLoc, 1, glm::value_ptr(glm::vec3(1.0f)));
                
                int spotLightSpecularLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.light.specular");
                glUniform3fv(spotLightSpecularLoc, 1, glm::value_ptr(specularColor));
                
                int spotLightEmissionLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.light.emission");
                glUniform3fv(spotLightEmissionLoc, 1, glm::value_ptr(specularColor));
                
                int spotLightConstantLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.attenuationState.constant");
                
                glUniform1f(spotLightConstantLoc, attenConst);
                
                int spotLightLinearLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.attenuationState.linear");
                
                glUniform1f(spotLightLinearLoc, attenLinear);
                
                int spotLightQuadLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.attenuationState.quadratic");
                glUniform1f(spotLightQuadLoc, attenQuad);
                
                int lightCutoffLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.cutoff");
                glUniform1f(lightCutoffLoc, glm::cos(glm::radians(12.5f)));
                
                int lightOuterCutoffLoc = glGetUniformLocation(s_shaders[LIGHTING_SHADER].shaderProg, "spotLight.outerCutoff");
                glUniform1f(lightOuterCutoffLoc, glm::cos(glm::radians(15.f)));
            }
            
            for (u32 i = 0; i < 10; ++i)
            {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, cubePositions[i]);
                float angle = 20.f * i;
                model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
                glDrawElements(GL_TRIANGLES, COUNT_OF(indices), GL_UNSIGNED_INT, 0);
            }
        }

        //Draw lamp
        {
            glUseProgram(s_shaders[LAMP_SHADER].shaderProg);
            
            int lightViewLoc = glGetUniformLocation(s_shaders[LAMP_SHADER].shaderProg, "view");
            glUniformMatrix4fv(lightViewLoc, 1, GL_FALSE, glm::value_ptr(g_View));
            //
            int lightProjectionLoc = glGetUniformLocation(s_shaders[LAMP_SHADER].shaderProg, "projection");
            glUniformMatrix4fv(lightProjectionLoc, 1, GL_FALSE, glm::value_ptr(g_Proj));
            
            int lightModelLoc = glGetUniformLocation(s_shaders[LAMP_SHADER].shaderProg, "model");
            
            for (u32 i = 0; i < COUNT_OF(pointLightPositions); ++i)
            {
                glm::mat4 lightModel = glm::mat4(1.0f);
                
                lightModel = glm::translate(lightModel, pointLightPositions[i]);
                lightModel = glm::scale(lightModel, glm::vec3(0.2f));
                glUniformMatrix4fv(lightModelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));
                
                glDrawElements(GL_TRIANGLES, COUNT_OF(indices), GL_UNSIGNED_INT, 0);
            }
        }
        
        //draw debug stuff
        if (g_debugVerts.size() > 0)
        {
            glUseProgram(s_shaders[DEBUG_SHADER].shaderProg);
            
            glm::mat4 debugModel = glm::mat4(1.0f);
            int debugModelLoc = glGetUniformLocation(s_shaders[DEBUG_SHADER].shaderProg, "model");
            glUniformMatrix4fv(debugModelLoc, 1, GL_FALSE, glm::value_ptr(debugModel));
            
            int debugViewLoc = glGetUniformLocation(s_shaders[DEBUG_SHADER].shaderProg, "view");
            glUniformMatrix4fv(debugViewLoc, 1, GL_FALSE, glm::value_ptr(g_View));
            //
            int debugProjectionLoc = glGetUniformLocation(s_shaders[DEBUG_SHADER].shaderProg, "projection");
            glUniformMatrix4fv(debugProjectionLoc, 1, GL_FALSE, glm::value_ptr(g_Proj));
            //
            int debugColorLoc = glGetUniformLocation(s_shaders[DEBUG_SHADER].shaderProg, "debugColor");
            glUniform4fv(debugColorLoc, 1, glm::value_ptr(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));
            
            glBindVertexArray(g_DebugVAO);
            
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DebugVertex)*g_debugVerts.size(), &g_debugVerts[0]);
            
            glDrawArrays(GL_LINES, 0, g_debugVerts.size());
        }
    }
    
    void OpenGLShutdown()
    {
        glDeleteVertexArrays(1, &g_VAO);
        glDeleteBuffers(1, &g_VBO);
        glDeleteBuffers(1, &g_EBO);
        
        glDeleteVertexArrays(1, &g_lightVAO);
        glDeleteVertexArrays(1, &g_DebugVAO);
        glDeleteBuffers(1, &g_DebugVBO);
        
        glDeleteProgram(s_shaders[LIGHTING_SHADER].shaderProg);
        glDeleteProgram(s_shaders[LAMP_SHADER].shaderProg);
        glDeleteProgram(s_shaders[DEBUG_SHADER].shaderProg);
        
        glDeleteTextures(1, &diffuseMap);
        glDeleteTextures(1, &specularMap);
        glDeleteTextures(1, &defaultTexture);
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
    
    u32 LoadImageTexture(const char* path)
    {
        //load and create texture
        u32 newTex;
        glGenTextures(1, &newTex);
        glBindTexture(GL_TEXTURE_2D, newTex);
        //set the texture wrapping/filtering options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //load the image
        
        s32 texWidth, texHeight, channels;
        u8* data = stbi_load(path, &texWidth, &texHeight, &channels, 0);
        R2_CHECK(data != nullptr, "We didn't load the image!");
        
        GLenum format;
        if (channels == 3)
        {
            format = GL_RGB;
        }
        else if(channels == 4)
        {
            format = GL_RGBA;
        }
        else
        {
            R2_CHECK(false, "UNKNOWN image format");
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        return newTex;
    }
    
    u32 CreateImageTexture(u32 width, u32 height, void* data)
    {
        u32 newTex;
        glGenTextures(1, &newTex);
        glBindTexture(GL_TEXTURE_2D, newTex);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        glTexSubImage2D(GL_TEXTURE_2D, 0,0,0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        return newTex;
    }
    
    
    u32 CreateShaderProgramFromStrings(const char* vertexShaderStr, const char* fragShaderStr)
    {
        R2_CHECK(vertexShaderStr != nullptr && fragShaderStr != nullptr, "Vertex and/or Fragment shader are nullptr");
        
        GLuint shaderProgram          = glCreateProgram();
        GLuint vertexShaderHandle     = glCreateShader( GL_VERTEX_SHADER );
        GLuint fragmentShaderHandle   = glCreateShader( GL_FRAGMENT_SHADER );
        
        { // compile shader and check for errors
            glShaderSource( vertexShaderHandle, 1, &vertexShaderStr, NULL );
            glCompileShader( vertexShaderHandle );
            int lparams = -1;
            glGetShaderiv( vertexShaderHandle, GL_COMPILE_STATUS, &lparams );
            
            if ( GL_TRUE != lparams ) {
                R2_LOGE("ERROR: vertex shader index %u did not compile\n", vertexShaderHandle);
                
                const int max_length = 2048;
                int actual_length    = 0;
                char slog[2048];
                glGetShaderInfoLog( vertexShaderHandle, max_length, &actual_length, slog );
                R2_LOGE("Shader info log for GL index %u:\n%s\n", vertexShaderHandle,
                        slog );
                
                glDeleteShader( vertexShaderHandle );
                glDeleteShader( fragmentShaderHandle );
                glDeleteProgram( shaderProgram );
                return 0;
            }
            
        }
        
        { // compile shader and check for errors
            glShaderSource( fragmentShaderHandle, 1, &fragShaderStr, NULL );
            glCompileShader( fragmentShaderHandle );
            int lparams = -1;
            glGetShaderiv( fragmentShaderHandle, GL_COMPILE_STATUS, &lparams );
            
            if ( GL_TRUE != lparams ) {
                R2_LOGE("ERROR: fragment shader index %u did not compile\n",
                        fragmentShaderHandle );
                
                const int max_length = 2048;
                int actual_length    = 0;
                char slog[2048];
                glGetShaderInfoLog( fragmentShaderHandle, max_length, &actual_length, slog );
                R2_LOGE("Shader info log for GL index %u:\n%s\n", fragmentShaderHandle,
                        slog );
                
                glDeleteShader( vertexShaderHandle );
                glDeleteShader( fragmentShaderHandle );
                glDeleteProgram( shaderProgram );
                return 0;
            }
            
        }
        
        glAttachShader( shaderProgram, fragmentShaderHandle );
        glAttachShader( shaderProgram, vertexShaderHandle );
        
        { // link program and check for errors
            glLinkProgram( shaderProgram );
            glDeleteShader( vertexShaderHandle );
            glDeleteShader( fragmentShaderHandle );
            int lparams = -1;
            glGetProgramiv( shaderProgram, GL_LINK_STATUS, &lparams );
            
            if ( GL_TRUE != lparams ) {
                R2_LOGE("ERROR: could not link shader program GL index %u\n",
                        shaderProgram );
                
                const int max_length = 2048;
                int actual_length    = 0;
                char plog[2048];
                glGetProgramInfoLog( shaderProgram, max_length, &actual_length, plog );
                R2_LOGE("Program info log for GL index %u:\n%s", shaderProgram, plog );
                
                glDeleteProgram( shaderProgram );
                return 0;
            }
        }
        
        return shaderProgram;
    }
    
    u32 CreateShaderProgramFromRawFiles(const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
    {
        R2_CHECK(vertexShaderFilePath != nullptr, "Vertex shader is null");
        R2_CHECK(fragmentShaderFilePath != nullptr, "Fragment shader is null");
        
        char* vertexFileData = nullptr;
        char* fragmentFileData = nullptr;
        {
            r2::fs::File* vertexFile = r2::fs::FileSystem::Open(DISK_CONFIG, vertexShaderFilePath, r2::fs::Read | r2::fs::Binary);
            
            R2_CHECK(vertexFile != nullptr, "Failed to open file: %s\n", vertexShaderFilePath);
            if(!vertexFile)
            {
                R2_LOGE("Failed to open file: %s\n", vertexShaderFilePath);
                return 0;
            }
            
            u64 fileSize = vertexFile->Size();
            
            vertexFileData = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, fileSize+1, sizeof(char));
            
            if(!vertexFileData)
            {
                R2_CHECK(false, "Could not allocate: %llu bytes", fileSize+1);
                return 0;
            }
            
            bool success = vertexFile->ReadAll(vertexFileData);
            
            if(!success)
            {
                FREE(vertexFileData, *MEM_ENG_SCRATCH_PTR);
                R2_LOGE("Failed to read file: %s", vertexShaderFilePath);
                return 0;
            }
            
            vertexFileData[fileSize] = '\0';
            r2::fs::FileSystem::Close(vertexFile);
        }
        
        {
            r2::fs::File* fragmentFile = r2::fs::FileSystem::Open(DISK_CONFIG, fragmentShaderFilePath, r2::fs::Read | r2::fs::Binary);
            
            R2_CHECK(fragmentFile != nullptr, "Failed to open file: %s\n", fragmentShaderFilePath);
            if(!fragmentFile)
            {
                R2_LOGE("Failed to open file: %s\n", fragmentShaderFilePath);
                return 0;
            }
            
            u64 fileSize = fragmentFile->Size();
            
            fragmentFileData = (char*)ALLOC_BYTESN(*MEM_ENG_SCRATCH_PTR, fileSize+1, sizeof(char));
            
            if(!fragmentFileData)
            {
                R2_CHECK(false, "Could not allocate: %llu bytes", fileSize+1);
                return 0;
            }
            
            bool success = fragmentFile->ReadAll(fragmentFileData);
            
            if(!success)
            {
                FREE(fragmentFileData, *MEM_ENG_SCRATCH_PTR);
                R2_LOGE("Failed to read file: %s", fragmentShaderFilePath);
                return 0;
            }
            
            fragmentFileData[fileSize] = '\0';
            r2::fs::FileSystem::Close(fragmentFile);
        }
        
        u32 shaderProg = CreateShaderProgramFromStrings(vertexFileData, fragmentFileData);
        
        FREE(fragmentFileData, *MEM_ENG_SCRATCH_PTR);//fragment first since it was allocated last - this is a stack currently
        FREE(vertexFileData, *MEM_ENG_SCRATCH_PTR);
        
        if (!shaderProg)
        {
            R2_CHECK(false, "Failed to create the shader program for vertex shader: %s\nAND\nFragment shader: %s\n", vertexShaderFilePath, fragmentShaderFilePath);
            
            R2_LOGE("Failed to create the shader program for vertex shader: %s\nAND\nFragment shader: %s\n", vertexShaderFilePath, fragmentShaderFilePath);
            return 0;
        }
        
        return shaderProg;
    }
    
    void ReloadShaderProgramFromRawFiles(u32* program, const char* vertexShaderFilePath, const char* fragmentShaderFilePath)
    {
        R2_CHECK(program != nullptr, "Shader Program is nullptr");
        R2_CHECK(vertexShaderFilePath != nullptr, "vertex shader file path is nullptr");
        R2_CHECK(fragmentShaderFilePath != nullptr, "fragment shader file path is nullptr");
        
        u32 reloadedShaderProgram = CreateShaderProgramFromRawFiles(vertexShaderFilePath, fragmentShaderFilePath);
        
        if (reloadedShaderProgram)
        {
            if(*program != 0)
            {
                glDeleteProgram(*program);
            }
            
            *program = reloadedShaderProgram;
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
                ReloadShaderProgramFromRawFiles(&shaderAsset.shaderProg, shaderAsset.manifest.vertexShaderPath.c_str(), shaderAsset.manifest.fragmentShaderPath.c_str());
            }
        }
    }
#endif
}
