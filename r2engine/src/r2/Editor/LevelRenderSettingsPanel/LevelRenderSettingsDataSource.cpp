#include "r2pch.h"

#ifdef R2_EDITOR

#include "R2/Editor/LevelRenderSettingsPanel/LevelRenderSettingsDataSource.h"
#include "r2/Core/Assets/AssetTypes.h"
#include "r2/Render/Model/Materials/MaterialTypes.h"

#include "r2/Editor/Editor.h"
#include "r2/Game/Level/Level.h"


namespace r2::edit
{

	bool LevelRenderSettingsDataSource::IsInLevelMaterialList(const r2::asset::AssetName& materialAsset)
	{
		const r2::Level& editorLevel = mnoptrEditor->GetEditorLevelConst();
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