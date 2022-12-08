#ifndef GLSL_DISPATCH_INDIRECT_DATA
#define GLSL_DISPATCH_INDIRECT_DATA

struct DispatchIndirectCommand
{
	uint numGroupsX;
	uint numGroupsY;
	uint numGroupsZ;
	uint unused;
};

layout (std430, binding=9) buffer DispatchIndirectCommands
{
	DispatchIndirectCommand dispatchCMDs[];
};

#endif