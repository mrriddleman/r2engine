#ifndef __ASSET_LIB_ANIMATION_CONVERT_H__
#define __ASSET_LIB_ANIMATION_CONVERT_H__

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace r2::assets::assetlib
{
	//struct VectorKey
	//{
	//	double time;
	//	glm::vec3 value;
	//};

	//struct RotationKey
	//{
	//	double time;
	//	glm::quat quat;
	//};

	//struct Channel
	//{
	//	uint64_t channelName;

	//	std::vector<VectorKey> positionKeys;
	//	std::vector<VectorKey> scaleKeys;
	//	std::vector<RotationKey> rotationKeys;
	//};

	////struct Animation
	////{
	////	uint64_t animationName = 0;
	////	double duration = 0; //in ticks
	////	double ticksPerSecond = 0;
	////	std::vector<Channel> channels;
	////	std::string originalPath;
	////};

	/*bool LoadAnimationFromFile(const std::filesystem::path& inputFilePath, Animation& animation);*/
}

#endif