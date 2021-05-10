#include "ibl/CubemapIBL.h"
#include "ibl/Cubemap.h"
#include "ibl/Utils.h"
#include "ibl/Image.h"

#include "glm/glm.hpp"
#include <atomic>

namespace
{
	const float PI = 3.14159f;
	const float ONE_OVER_PI = 1.0f / PI;
}

namespace r2::ibl
{

	static glm::vec3 HemisphereCosSample(glm::vec2 u)
	{
		const float phi = 2.0f * (float)PI * u.x;
		const float cosTheta2 = 1 - u.y;
		const float cosTheta = std::sqrt(cosTheta2);
		const float sinTheta = std::sqrt(1 - cosTheta2);
		return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
	}

	void CubemapIBL::DiffuseIrradiance(Cubemap& dst, const std::vector<Cubemap>& levels,
		size_t maxNumSamples, Progress updater, void* userData)
	{
		const float numSamples = maxNumSamples;
		const float inumSamples = 1.0f / numSamples;
		const size_t maxLevel = levels.size()-1;
		const float maxLevelf = maxLevel;
		const Cubemap& base(levels[0]);
		const size_t dim0 = base.GetDimensions();
		const float omegaP = (4.0f * (float)PI) / float(6 * dim0 * dim0);

		//std::atomic_uint progress = { 0 };
		uint32_t progress = 0;


		struct CacheEntry 
		{
			glm::vec3 L;
			float lerp;
			uint8_t l0;
			uint8_t l1;
		};

		std::vector<CacheEntry> cache;
		cache.reserve(maxNumSamples);

		for (size_t sampleIndex = 0; sampleIndex < maxNumSamples; ++sampleIndex)
		{
			const glm::vec2 u = Hammersley(uint32_t(sampleIndex), inumSamples);
			const glm::vec3 L = HemisphereCosSample(u);
			const glm::vec3 N = { 0, 0, 1 };
			const float NoL = glm::dot(N, L);

			if (NoL > 0)
			{
				float pdf = NoL * (float)ONE_OVER_PI;

				constexpr float K = 4;
				const float omegaS = 1.0f / (numSamples * pdf);
				const float l = float(log4(omegaS) - log4(omegaP) + log4(K));
				const float mipLevel = glm::clamp(float(l), 0.0f, maxLevelf);

				uint8_t l0 = uint8_t(mipLevel);
				uint8_t l1 = uint8_t(std::min(maxLevel, size_t(l0 + 1)));
				float lerp = mipLevel - (float)l0;
				cache.push_back({ L, lerp, l0, l1 });
			}

			const size_t dim = dst.GetDimensions();

			for (size_t faceIndex = 0; faceIndex < 6; faceIndex++) 
			{
				const Cubemap::Face f = (Cubemap::Face)faceIndex;
				Image& image(dst.GetImageForFace(f));

				for (size_t y = 0; y < dim; y++) 
				{
					Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.GetPixelPtr(0, y));
					
					if (updater)
					{
						progress++;
						updater(0, (float)progress / ((float)dim * 6.0f), userData);
					}

					glm::mat3 R;
					const size_t numSamples = cache.size();
					for (size_t x = 0; x < dim; ++x, ++data)
					{
						const glm::vec2 p(Cubemap::Center(x, y));
						const glm::vec3 N(dst.GetDirectionFor(f, p.x, p.y));

						const glm::vec3 up = std::abs(N.z) < 0.999 ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
						R[0] = glm::normalize(glm::cross(up, N));
						R[1] = glm::cross(N, R[0]);
						R[2] = N;

						Cubemap::Texel Li = Cubemap::Texel(0.0f);
						for (size_t sample = 0; sample < numSamples; ++sample)
						{
							const CacheEntry& e = cache[sample];
							const glm::vec3 L(R* e.L);
							const Cubemap& cmBase = levels[e.l0];
							const Cubemap& next = levels[e.l1];
							const Cubemap::Texel c0 = Cubemap::TrilinearFilterAt(cmBase, next, e.lerp, L);
							Li += c0;
						}

						Cubemap::WriteAt(data, Cubemap::Texel(Li* inumSamples));
					}
				}
			}
		}
	}
}