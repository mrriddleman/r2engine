#ifndef __MAKE_ENGINE_MODELS_H__
#define __MAKE_ENGINE_MODELS_H__

#ifdef R2_ASSET_PIPELINE


namespace r2::asset::pln
{
	typedef void (*MakeModlFunc)(const std::string&, const std::string&, const std::string&);

	std::vector<MakeModlFunc> ShouldMakeEngineModels();
	std::vector<MakeModlFunc> ShouldMakeEngineMeshes();
	bool ShouldMakeEngineModelManifest();

	void MakeEngineModels(const std::vector<MakeModlFunc>& makeModels);
	void MakeEngineMeshes(const std::vector<MakeModlFunc>& makeMeshes);
	void MakeEngineModelManifest();
}

#endif
#endif // 
