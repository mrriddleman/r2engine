#ifndef __TRANSFORM_DIRTY_COMPONENT_H__
#define __TRANSFORM_DIRTY_COMPONENT_H__

namespace r2::ecs
{
	struct TransformDirtyComponent
	{
		unsigned int dirtyFlags = 0;

		//Only used for detach...
		unsigned int parent = 0;

		unsigned int hierarchyAttachmentType = 0;

	};
}

#endif // __TRANSFORM_DIRTY_COMPONENT_H__
