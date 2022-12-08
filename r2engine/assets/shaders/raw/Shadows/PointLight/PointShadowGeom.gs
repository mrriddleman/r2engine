#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Common/Defines.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"
#include "Input/ShaderBufferObjects/ShadowData.glsl"

const uint MAX_INVOCATIONS_PER_BATCH = 32; //this is GL_MAX_GEOMETRY_SHADER_INVOCATIONS
const uint MAX_POINTLIGHTS_PER_BATCH = MAX_INVOCATIONS_PER_BATCH;
const uint MAX_VERTICES = NUM_SIDES_FOR_POINTLIGHT * 3;

layout (triangles, invocations = MAX_INVOCATIONS_PER_BATCH) in;
layout (triangle_strip, max_vertices = MAX_VERTICES) out;

uniform uint pointLightBatch;

out vec4 FragPos; // FragPos from GS (output per emitvertex)
out vec3 LightPos;
out float FarPlane;

void main(void)
{
	int lightIndex = gl_InvocationID  + int(pointLightBatch) * int(MAX_POINTLIGHTS_PER_BATCH);

	if(lightIndex < numShadowCastingPointLights)
	{

		int pointLightIndex = (int)shadowCastingPointLights[lightIndex];

		for(int face = 0; face < NUM_SIDES_FOR_POINTLIGHT; ++face)
		{
			gl_Layer = face + int(gPointLightShadowMapPages[int(pointLights[pointLightIndex].lightProperties.lightID)]) * int(NUM_SIDES_FOR_POINTLIGHT);

			vec4 vertex[3];
			int outOfBounds[6] = {0,0,0,0,0,0};

			for(int i = 0; i < 3; ++i)
			{
				vertex[i] = pointLights[pointLightIndex].lightSpaceMatrices[face] * gl_in[i].gl_Position;

				if ( vertex[i].x > +vertex[i].w ) ++outOfBounds[0];
				if ( vertex[i].x < -vertex[i].w ) ++outOfBounds[1];
				if ( vertex[i].y > +vertex[i].w ) ++outOfBounds[2];
				if ( vertex[i].y < -vertex[i].w ) ++outOfBounds[3];
				if ( vertex[i].z > +vertex[i].w ) ++outOfBounds[4];
				if ( vertex[i].z < -vertex[i].w ) ++outOfBounds[5];
			}

			bool inFrustum = true;
			for(int i = 0; i < 6; ++i)
			{
				if(outOfBounds[i] == 3) inFrustum = false;
			}

			if(inFrustum)
			{
				for(int i = 0; i < 3; ++i)
				{
					gl_Position = vertex[i];
					FragPos = gl_in[i].gl_Position;
					LightPos = pointLights[pointLightIndex].position.xyz;
					FarPlane = pointLights[pointLightIndex].lightProperties.intensity; //if we change the compute shader, this needs to change as well
					EmitVertex();
				}

				EndPrimitive();
			}
			
		}
	}
}