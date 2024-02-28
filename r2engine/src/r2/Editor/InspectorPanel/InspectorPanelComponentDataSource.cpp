#include "r2pch.h"

#ifdef R2_EDITOR


#include "r2/Editor/InspectorPanel/InspectorPanelComponentDataSource.h"
#include "r2/Editor/Editor.h"
#include "R2/Game/Level/Level.h"


namespace r2::edit 
{
	bool InspectorPanelComponentDataSource::IsInLevelMaterialList(r2::Editor* editor, const r2::asset::AssetName& materialAsset) const
	{
		const r2::Level& editorLevel = editor->GetEditorLevelConst();

		const r2::SArray<r2::mat::MaterialName>* levelMaterials = editorLevel.GetMaterials();

		const auto numMaterialsInLevel = r2::sarr::Size(*levelMaterials);

		for (u32 i = 0; i < numMaterialsInLevel; ++i)
		{
			const r2::mat::MaterialName& materialName = r2::sarr::At(*levelMaterials, i);

			if (materialName.assetName == materialAsset)
			{
				return true;
			}
		}

		return false;
	}
}



#endif