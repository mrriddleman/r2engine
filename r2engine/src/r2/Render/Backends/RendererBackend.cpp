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

namespace
{
    float cubeVerts[] = {
        //front face
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, //top right
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, //bottom right
        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, //bottom left
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, // top left
        
        //right face
        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, //top left 4
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, //bottom left 5
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, //bottom right 6
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, // top right 7
        
        //left face
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, //bottom  left 8
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, //top left 9
        -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, //bottom right 10
        -0.5f, 0.5f, 0.5f, 1.0f, 1.0f, //top right 11
        
        //back face
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, //bottom left 12
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, //top left 13
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, //bottom right 14
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, //top right 15
        
        //top face
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, //bottom left 16
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, //top left 17
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, //bottom right 18
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, //top right 19
        
        //bottom face
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, //bottom left 20
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, //top left 21
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, //bottom right 22
        0.5f, -0.5f, 0.5f, 1.0f, 1.0f //top right 23
        
    };
    
    float lineVerts[] = {
        -0.5, 0, 0, 0, 0,
        0.5, 0, 0, 0, 0
        
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
    
    glm::mat4 g_View = glm::mat4(1.0f);
    glm::mat4 g_Proj = glm::mat4(1.0f);
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    
    struct DebugVertex
    {
        glm::vec3 position;
    };
    
    std::vector<DebugVertex> g_debugVerts;
    
    const char *vertexShaderSource = "#version 410 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "uniform mat4 model;"
    "uniform mat4 view;"
    "uniform mat4 projection;"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\0";
    const char *fragmentShaderSource = "#version 410 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform float time;\n"
    "uniform sampler2D texture1;\n"
    "uniform sampler2D texture2;\n"
    "uniform vec3 objectColor;\n"
    "uniform vec3 lightColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(objectColor * lightColor, 1.0);\n"
    "}\n\0";
    
    const char* lampVertexShaderSource = "#version 410 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "}\n\0";
    
    const char* lampFragmentShaderSource = "#version 410 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0);\n"
    "}\n\0";
    
    const char* debugVertexShaderSource = "#version 410 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 model;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
    "}\n\0";
    
    const char* debugFragmentShaderSource = "#version 410 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 debugColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = debugColor;\n"
    "}\n\0";
    
    
    u32 g_ShaderProg, g_lightShaderProg, g_debugShaderProg, g_VBO, g_VAO, g_EBO, texture1, texture2, g_DebugVAO, g_DebugVBO, defaultTexture, g_lightVAO;
}

namespace r2::draw
{
    u32 CreateShaderProgram(const char* vertexShaderStr, const char* fragShaderStr);
    u32 LoadImageTexture(const char* path);
    u32 CreateImageTexture(u32 width, u32 height, void* data);
    
    void OpenGLInit()
    {
        stbi_set_flip_vertically_on_load(true);
        glClearColor(0.25f, 0.25f, 0.4f, 1.0f);
        
        //basic object stuff
        {
            g_ShaderProg = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
            
            glGenVertexArrays(1, &g_VAO);
            glGenBuffers(1, &g_VBO);
            glGenBuffers(1, &g_EBO);
            
            glBindVertexArray(g_VAO);
            
            glBindBuffer(GL_ARRAY_BUFFER, g_VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
            
            //position attribute
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            
            //uv attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
            glEnableVertexAttribArray(1);
            
            glBindVertexArray(0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            
            glUseProgram(g_ShaderProg);
            
            glUniform1i(glGetUniformLocation(g_ShaderProg, "texture1"), 0);
            glUniform1i(glGetUniformLocation(g_ShaderProg, "texture2"), 1);
            
            glUseProgram(0);
        }
        
        //lamp setup
        {
            g_lightShaderProg = CreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource);
            
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
            g_debugShaderProg = CreateShaderProgram(debugVertexShaderSource, debugFragmentShaderSource);
            
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
        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "container.jpg", path);
        texture1 = LoadImageTexture(path);
        r2::fs::utils::BuildPathFromCategory(r2::fs::utils::Directory::TEXTURES, "awesomeface.png", path);
        texture2 = LoadImageTexture(path);
    }
    
    void OpenGLDraw(float alpha)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        //Draw the cube
        {
            glUseProgram(g_ShaderProg);
            
            //        glActiveTexture(GL_TEXTURE0);
            //        glBindTexture(GL_TEXTURE_2D, texture1);
            //        glActiveTexture(GL_TEXTURE1);
            //        glBindTexture(GL_TEXTURE_2D, texture2);
            
            glBindVertexArray(g_VAO);
            
            float timeVal = static_cast<float>(CENG.GetTicks()) / 1000.f;
            
            int uniformTimeLocation = glGetUniformLocation(g_ShaderProg, "time");
            glUniform1f(uniformTimeLocation, timeVal);
            //
            int viewLoc = glGetUniformLocation(g_ShaderProg, "view");
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(g_View));
            //
            int projectionLoc = glGetUniformLocation(g_ShaderProg, "projection");
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(g_Proj));
            //
            int colorLoc = glGetUniformLocation(g_ShaderProg, "objectColor");
            glUniform3fv(colorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 0.5f, 0.31f)));
            
            int lightColorLoc = glGetUniformLocation(g_ShaderProg, "lightColor");
            glUniform3fv(lightColorLoc, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));
            
            glm::mat4 model = glm::mat4(1.0f);
            int modelLoc = glGetUniformLocation(g_ShaderProg, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            glDrawElements(GL_TRIANGLES, COUNT_OF(indices), GL_UNSIGNED_INT, 0);
        }

        //Draw lamp
        {
            glUseProgram(g_lightShaderProg);
            
            int lightViewLoc = glGetUniformLocation(g_lightShaderProg, "view");
            glUniformMatrix4fv(lightViewLoc, 1, GL_FALSE, glm::value_ptr(g_View));
            //
            int lightProjectionLoc = glGetUniformLocation(g_lightShaderProg, "projection");
            glUniformMatrix4fv(lightProjectionLoc, 1, GL_FALSE, glm::value_ptr(g_Proj));
            
            glm::mat4 lightModel = glm::mat4(1.0f);
            lightModel = glm::translate(lightModel, lightPos);
            lightModel = glm::scale(lightModel, glm::vec3(0.2f));
            
            int lightModelLoc = glGetUniformLocation(g_lightShaderProg, "model");
            glUniformMatrix4fv(lightModelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));
            
            glDrawElements(GL_TRIANGLES, COUNT_OF(indices), GL_UNSIGNED_INT, 0);
        }
        
        //draw debug stuff
        if (g_debugVerts.size() > 0)
        {
            glUseProgram(g_debugShaderProg);
            
            glm::mat4 debugModel = glm::mat4(1.0f);
            int debugModelLoc = glGetUniformLocation(g_debugShaderProg, "model");
            glUniformMatrix4fv(debugModelLoc, 1, GL_FALSE, glm::value_ptr(debugModel));
            
            int debugViewLoc = glGetUniformLocation(g_debugShaderProg, "view");
            glUniformMatrix4fv(debugViewLoc, 1, GL_FALSE, glm::value_ptr(g_View));
            //
            int debugProjectionLoc = glGetUniformLocation(g_debugShaderProg, "projection");
            glUniformMatrix4fv(debugProjectionLoc, 1, GL_FALSE, glm::value_ptr(g_Proj));
            //
            int debugColorLoc = glGetUniformLocation(g_debugShaderProg, "debugColor");
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
        
        glDeleteProgram(g_ShaderProg);
        glDeleteProgram(g_lightShaderProg);
        glDeleteProgram(g_debugShaderProg);
        
        glDeleteTextures(1, &texture1);
        glDeleteTextures(1, &texture2);
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
    
    u32 CreateShaderProgram(const char* vertexShaderStr, const char* fragShaderStr)
    {
        R2_CHECK(vertexShaderStr != nullptr && fragShaderStr != nullptr, "Vertex and/or Fragment shader are nullptr");
        
        GLuint shaderProgram         = glCreateProgram();
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
}
