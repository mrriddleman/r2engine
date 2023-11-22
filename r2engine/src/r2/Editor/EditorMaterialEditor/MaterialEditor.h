#ifndef __MATERIAL_EDITOR_H__
#define __MATERIAL_EDITOR_H__
#ifdef R2_EDITOR

namespace r2::mat
{
	struct MaterialName;
}

namespace r2::edit
{
	void MaterialEditor(const r2::mat::MaterialName& materialName, bool& windowOpen);
}

#endif
#endif