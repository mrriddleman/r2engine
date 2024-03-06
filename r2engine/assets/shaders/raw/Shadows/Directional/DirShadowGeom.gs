#version 450 core

#extension GL_NV_gpu_shader5 : enable

#include "Common/Defines.glsl"
#include "Common/Texture.glsl"
#include "Input/UniformBuffers/Vectors.glsl"
#include "Input/ShaderBufferObjects/LightingData.glsl"
#include "Input/ShaderBufferObjects/ShadowData.glsl"

const uint MAX_INVOCATIONS_PER_BATCH = 32; //this is GL_MAX_GEOMETRY_SHADER_INVOCATIONS
const uint MAX_NUM_LIGHTS_PER_BATCH = MAX_INVOCATIONS_PER_BATCH / NUM_FRUSTUM_SPLITS;

layout (triangles, invocations = MAX_INVOCATIONS_PER_BATCH) in;
layout (triangle_strip, max_vertices = 3) out;


uniform uint directionLightBatch;

void main()
{

	int lightIndex = (gl_InvocationID / int(NUM_FRUSTUM_SPLITS)) + int(directionLightBatch) * int(MAX_NUM_LIGHTS_PER_BATCH);
	int cascadeIndex = gl_InvocationID % int(NUM_FRUSTUM_SPLITS);

	if(lightIndex < numShadowCastingDirectionLights)
	{

		int dirLightIndex = (int)shadowCastingDirectionLights[lightIndex];

		vec3 normal = cross(gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz, gl_in[0].gl_Position.xyz - gl_in[1].gl_Position.xyz);
		vec3 view = -dirLights[dirLightIndex].direction.xyz;
	//	Partition part = gPartitions[gl_InvocationID];

		if(dot(normal, view) > 0.0f)
		{
			vec4 vertex[3];
			int outOfBound[6] = { 0 , 0 , 0 , 0 , 0 , 0 };
			
			mat4 shadowMatrix = dirLights[dirLightIndex].lightSpaceMatrixData.lightProjMatrices[cascadeIndex] * dirLights[dirLightIndex].lightSpaceMatrixData.lightViewMatrices[cascadeIndex];

			for (int i =0; i < 3; ++i )
			{
				vertex[i] = shadowMatrix * gl_in[i].gl_Position;

				//BEGIN MAJOR HACK TO REDUCE PETER-PANNING

				// vec3 cameraVec = gl_in[i].gl_Position.xyz - cameraPosTimeW.xyz;
				// float viewLength = length(cameraVec);
				// vec3 shadowOffset = (cameraVec.xyz / viewLength);// * (1.0 - dot(view, cameraVec));
				// float VoL = dot(normalize(cameraVec), normalize(view) );
				// float VoN = dot(normalize(cameraVec), normal);

				// vec4 offsetInShadowSpace = shadowMatrix * (vec4(shadowOffset, 0) * (0.14) * clamp((1.0 - VoL), 0.0, 1.0) ) ;

				// vertex[i] += vec4(offsetInShadowSpace.xy, 0, 0);

				//END MAJOR HACK TO REDUCE PETER-PANNING

				if ( vertex[i].x > +vertex[i].w ) ++outOfBound[0];
				if ( vertex[i].x < -vertex[i].w ) ++outOfBound[1];
				if ( vertex[i].y > +vertex[i].w ) ++outOfBound[2];
				if ( vertex[i].y < -vertex[i].w ) ++outOfBound[3];
				if ( vertex[i].z > +vertex[i].w ) ++outOfBound[4];
				if ( vertex[i].z < -vertex[i].w ) ++outOfBound[5];
			}

			bool inFrustum = true;
			for (int i = 0; i < 6; ++i )
				if ( outOfBound[i] == 3) inFrustum = false;

			if(inFrustum)
			{
				for(int i = 0; i < 3; ++i)
				{
					gl_Position = vertex[i];
					gl_Layer = cascadeIndex + int(gDirectionLightShadowMapPages[int(dirLights[dirLightIndex].lightProperties.lightID)]);
					EmitVertex();
				}

				EndPrimitive();
			}
		}
	}
}