#ifndef __MATERIAL_EDITOR_H__
#define __MATERIAL_EDITOR_H__
#ifdef R2_EDITOR

namespace r2::mat
{
	struct MaterialName;
#ifdef R2_ASSET_PIPELINE
	struct Material;
#endif
}

namespace r2::edit
{
	void EditExistingMaterial(const r2::mat::MaterialName& materialName, bool& windowOpen);
#ifdef R2_ASSET_PIPELINE
	void CreateNewMaterial( bool& windowOpen);
#endif
}

#endif
#endif