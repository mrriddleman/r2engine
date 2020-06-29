#ifndef __MAKE_ENGINE_MODELS_H__
#define __MAKE_ENGINE_MODELS_H__

#ifdef R2_ASSET_PIPELINE


namespace r2::asset::pln
{
	typedef void (*MakeModlFunc)(const std::string&, const std::string&, const std::string&);
	std::vector<MakeModlFunc> ShouldMakeEngineModels();
	void MakeEngineModels(const std::vector<MakeModlFunc>& makeModels);
}

#endif
#endif // 
