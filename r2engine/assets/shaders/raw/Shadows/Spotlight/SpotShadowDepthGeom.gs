#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Common/Defines.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"
#include "Input/ShaderBufferObjects/ShadowData.glsl"

const uint MAX_INVOCATIONS_PER_BATCH = 32; //this is GL_MAX_GEOMETRY_SHADER_INVOCATIONS
const uint MAX_SPOTLIGHTS_PER_BATCH = MAX_INVOCATIONS_PER_BATCH / NUM_SPOTLIGHT_LAYERS;

layout (triangles, invocations = MAX_INVOCATIONS_PER_BATCH) in;
layout (triangle_strip, max_vertices = 3) out;

uniform uint spotLightBatch;

void main(void)
{
	int lightIndex = gl_InvocationID + int(spotLightBatch) * int(MAX_SPOTLIGHTS_PER_BATCH);

	if(lightIndex < numShadowCastingSpotLights)
	{

		int spotLightIndex = (int)shadowCastingSpotLights[lightIndex];

		vec3 normal = cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz);
		vec3 lightDir = -spotLights[spotLightIndex].direction.xyz;

		if(dot(normal, lightDir) > 0.0)
		{
			vec4 vertex[3];
			int outOfBounds[6] = {0,0,0,0,0,0};

			for(int i = 0; i < 3; ++i)
			{
				vertex[i] = spotLights[spotLightIndex].lightSpaceMatrix * gl_in[i].gl_Position;

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
					gl_Layer = int(gSpotLightShadowMapPages[int(spotLights[spotLightIndex].lightProperties.lightID)]);
					EmitVertex();
				}

				EndPrimitive();
			}
		}
	}
}