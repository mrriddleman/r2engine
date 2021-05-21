#include "ibl/CubemapIBL.h"
#include "ibl/Cubemap.h"
#include "ibl/Utils.h"
#include "ibl/Image.h"

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include <atomic>
#include <algorithm>
#include <random>


namespace
{
	const float PI = 3.14159f;
	const float ONE_OVER_PI = 1.0f / PI;
}

namespace r2::ibl
{

	static glm::vec3 HemisphereImportanceSampleDggx(const glm::vec2& u, float a)
	{
		const float phi = 2.0f * PI * u.x;

		const float cosTheta2 = (1.0f - u.y) / (1.0f + (a + 1) * ((a - 1.0) * u.y));
		const float cosTheta = std::sqrt(cosTheta2);
		const float sinTheta = std::sqrt(1.0 - cosTheta2);
		return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
	}

	static float DistributionGGX(float NoH, float linearRoughness) {
		// NOTE: (aa-1) == (a-1)(a+1) produces better fp accuracy
		float a = linearRoughness;
		float f = (a - 1) * ((a + 1) * (NoH * NoH)) + 1;
		return (a * a) / ((float)PI * f * f);
	}

	static glm::vec3 HemisphereCosSample(glm::vec2 u)
	{
		const float phi = 2.0f * (float)PI * u.x;
		const float cosTheta2 = 1 - u.y;
		const float cosTheta = std::sqrt(cosTheta2);
		const float sinTheta = std::sqrt(1 - cosTheta2);
		return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
	}

	float Saturate(float val)
	{
		return glm::clamp(val, 0.0f, 1.0f);
	}

	static float Visibility(float NoV, float NoL, float a) {
		// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
		// Height-correlated GGX
		const float a2 = a * a;
		const float GGXL = NoV * std::sqrt((NoL - NoL * a2) * NoL + a2);
		const float GGXV = NoL * std::sqrt((NoV - NoV * a2) * NoV + a2);
		return 0.5f / (GGXV + GGXL);
	}

	static glm::vec3 HemisphereUniformSample(glm::vec2 u) { // pdf = 1.0 / (2.0 * F_PI);
		const float phi = 2.0f * (float)PI * u.x;
		const float cosTheta = 1 - u.y;
		const float sinTheta = std::sqrt(1 - cosTheta * cosTheta);
		return { sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta };
	}

	static float VisibilityAshikhmin(float NoV, float NoL, float /*a*/) {
		// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
		return 1 / (4 * (NoL + NoV - NoL * NoV));
	}

	static float DistributionCharlie(float NoH, float linearRoughness) {
		// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
		float a = linearRoughness;
		float invAlpha = 1 / a;
		float cos2h = NoH * NoH;
		float sin2h = 1 - cos2h;
		return (2.0f + invAlpha) * std::pow(sin2h, invAlpha * 0.5f) / (2.0f * (float)PI);
	}

	static float DFV_Charlie_Uniform(float NoV, float linearRoughness, size_t numSamples) {
		float r = 0.0;
		const glm::vec3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
		for (size_t i = 0; i < numSamples; i++) {
			const glm::vec2 u = Hammersley(uint32_t(i), 1.0f / numSamples);
			const glm::vec3 H = HemisphereUniformSample(u);
			const glm::vec3 L = 2 * glm::dot(V, H) * H - V;
			const float VoH = Saturate(glm::dot(V, H));
			const float NoL = Saturate(L.z);
			const float NoH = Saturate(H.z);
			if (NoL > 0) {
				const float v = VisibilityAshikhmin(NoV, NoL, linearRoughness);
				const float d = DistributionCharlie(NoH, linearRoughness);
				r += v * d * NoL * VoH; // VoH comes from the Jacobian, 1/(4*VoH)
			}
		}
		// uniform sampling, the PDF is 1/2pi, 4 comes from the Jacobian
		return r * (4.0f * 2.0f * (float)PI / numSamples);
	}

	static glm::vec2 DFV(float NoV, float linearRoughness, size_t numSamples)
	{
		glm::vec2 r(0);

		const glm::vec3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
		for (size_t i = 0; i < numSamples; ++i)
		{
			const glm::vec2 u = Hammersley(uint32_t(i), 1.0f / numSamples);
			const glm::vec3 H = HemisphereImportanceSampleDggx(u, linearRoughness);
			const glm::vec3 L = 2.f * glm::dot(V, H) * H - V;
			const float VoH = Saturate(glm::dot(V, H));
			const float NoL = Saturate(L.z);
			const float NoH = Saturate(H.z);

			if (NoL > 0)
			{
				const float v = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
				const float Fc = std::pow(1.f - VoH, 5);
				r.x += v * (1.0f - Fc);
				r.y += v * Fc;
			}
		}

		return r * (4.0f / numSamples);
	}

	static glm::vec2 DFV_Multiscatter(float NoV, float linearRoughness, size_t numSamples)
	{
		glm::vec2 r (0);

		const glm::vec3 V(std::sqrt(1 - NoV * NoV), 0, NoV);
		for (size_t i = 0; i < numSamples; ++i)
		{
			const glm::vec2 u = Hammersley(uint32_t(i), 1.0f / numSamples);
			const glm::vec3 H = HemisphereImportanceSampleDggx(u, linearRoughness);
			const glm::vec3 L = 2.f * glm::dot(V, H) * H - V;
			const float VoH = Saturate(glm::dot(V, H));
			const float NoL = Saturate(L.z);
			const float NoH = Saturate(H.z);

			if (NoL > 0)
			{
				const float v = Visibility(NoV, NoL, linearRoughness) * NoL * (VoH / NoH);
				const float Fc = std::pow(1.f - VoH, 5);
				r.x += v * Fc;
				r.y += v;
			}	
		}

		return r * (4.0f / numSamples);
	}


	void CubemapIBL::RoughnessFilter(
		Cubemap& dst,
		const std::vector<Cubemap>& levels,
		float linearRoughness,
		size_t maxNumSamples,
		glm::vec3 mirror,
		bool prefilter,
		Progress updater,
		void* userData)
	{
		const float numSamples = maxNumSamples;
		const float inumSamples = 1.0f / numSamples;
		const size_t maxLevel = levels.size() - 1;
		const float maxLevelf = maxLevel;
		const Cubemap& base(levels[0]);
		const size_t dim0 = base.GetDimensions();
		const float omegaP = (4.0f * (float)PI) / float(6 * dim0 * dim0);

		uint32_t progress = 0;

		if (abs(linearRoughness) < 0.0001f)
		{
			auto Scanline = [&](size_t y, Cubemap::Face f, Cubemap::Texel* data, size_t dim) {
				if (updater)
				{
					size_t p = progress + 1;
					updater(0, (float)p / ((float)dim * 6.0f), userData);
				}

				const Cubemap& cm = levels[0];
				for (size_t x = 0; x < dim; ++x, ++data)
				{
					const glm::vec2 p(Cubemap::Center(x, y));
					const glm::vec3 N(dst.GetDirectionFor(f, p.x, p.y) * mirror);
					//TODO: pick proper lod and do trilinear filtering
					Cubemap::WriteAt(data, cm.SampleAt(N));
				}
			};

			size_t dim = dst.GetDimensions();

			for (size_t faceIndex = 0; faceIndex < 6; faceIndex++) {
				const Cubemap::Face f = (Cubemap::Face)faceIndex;
				Image& image(dst.GetImageForFace(f));

				for (size_t y = 0; y < dim; y++) {
					Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.GetPixelPtr(0, y));
					
					Scanline(y, f, data, dim);
				}
			}
			
			return;
		}

		struct CacheEntry {
			glm::vec3 L;
			float brdf_NoL;
			float lerp;
			uint8_t l0;
			uint8_t l1;
		};

		std::vector<CacheEntry> cache;
		cache.reserve(maxNumSamples);

		float weight = 0;
		for (size_t sampleIndex = 0; sampleIndex < maxNumSamples; sampleIndex++)
		{
			const glm::vec2 u = Hammersley(uint32_t(sampleIndex), inumSamples);

			const glm::vec3 H = HemisphereImportanceSampleDggx(u, linearRoughness);

			const float NoH = H.z;
			const float NoH2 = H.z * H.z;
			const float NoL = 2.0f * NoH2 - 1.0f;
			const glm::vec3 L(2.f * NoH * H.x, 2.0f * NoH * H.y, NoL);

			if (NoL > 0)
			{
				const float pdf = DistributionGGX(NoH, linearRoughness) / 4.0f;

				constexpr float K = 4.f;
				const float omegaS = 1.0f / (numSamples * pdf);
				const float l = float(log4(omegaS) - log4(omegaP) + log4(K));
				const float mipLevel = prefilter ? glm::clamp(float(l), 0.0f, maxLevelf) : 0.0f;


				const float brdf_NoL = float(NoL);
				weight += brdf_NoL;

				uint8_t l0 = uint8_t(mipLevel);
				uint8_t l1 = uint8_t(std::min(maxLevel, size_t(l0 + 1)));
				float lerp = mipLevel - (float)l0;

				cache.push_back({ L, brdf_NoL, lerp, l0, l1 });
			}
		}

		for (auto& entry : cache)
		{
			entry.brdf_NoL *= 1.0f / weight;
		}

		std::sort(cache.begin(), cache.end(), [](CacheEntry const& lhs, CacheEntry const& rhs) {
			return lhs.brdf_NoL < rhs.brdf_NoL;
		});

		struct State
		{
			std::default_random_engine gen;
			std::uniform_real_distribution<float> distribution{ -PI, PI };
		};

		auto Scanline = [&](State& state, size_t y, Cubemap::Face f, Cubemap::Texel* data, size_t dim) {
			if (updater)
			{
				size_t p = progress++;
				updater(0, (float)p / ((float)dim * 6.0f), userData);
			}

			glm::mat3 R;
			const size_t numSamples = cache.size();
			for (size_t x = 0; x < dim; ++x, ++data)
			{
				const glm::vec2 p(Cubemap::Center(x, y));
				const glm::vec3 N(dst.GetDirectionFor(f, p.x, p.y)* mirror);

				const glm::vec3 up = std::abs(N.z) < 0.999 ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
				R[0] = glm::normalize(glm::cross(up, N));
				R[1] = glm::cross(N, R[0]);
				R[2] = N;

				glm::mat4 R4 = glm::mat4(R);
				
				R = glm::mat3(glm::rotate(R4, state.distribution(state.gen), glm::vec3(0,0,1)));

				glm::vec3 Li = glm::vec3(0);
				for (size_t sample = 0; sample < numSamples; ++sample)
				{
					const CacheEntry& e = cache[sample];
					const glm::vec3 L(R* e.L);
					const Cubemap& cmBase = levels[e.l0];;
					const Cubemap& next = levels[e.l1];
					const Cubemap::Texel c0 = Cubemap::TrilinearFilterAt(cmBase, next, e.lerp, L);

					Cubemap::Texel result = c0* e.brdf_NoL;

					glm::vec3 resultglm{ result.r, result.g, result.b };
					Li += resultglm;
				}
				Cubemap::WriteAt(data, Cubemap::Texel(Li));
			}
		};


		State state;
		size_t dim = dst.GetDimensions();

		for (size_t faceIndex = 0; faceIndex < 6; faceIndex++) {
			const Cubemap::Face f = (Cubemap::Face)faceIndex;
			Image& image(dst.GetImageForFace(f));

			for (size_t y = 0; y < dim; y++) {
				Cubemap::Texel* data = static_cast<Cubemap::Texel*>(image.GetPixelPtr(0, y));

				Scanline(state, y, f, data, dim);
			}
		}
	}

	void CubemapIBL::DFG(Image& dst, bool multiscatter, bool cloth)
	{
		auto dfvFunction = multiscatter ? DFV_Multiscatter : DFV;

		const size_t height = dst.GetHeight();
		const size_t width = dst.GetWidth();

		for (size_t y = 0; y < height; y++)
		{
			Cubemap::Texel* data = static_cast<Cubemap::Texel*>(dst.GetPixelPtr(0, y));

			const float h = (float)height;
			const float coord = Saturate((h - y + 0.5f) / h);
			const float linear_roughness = coord * coord;
			for (size_t x = 0; x < width; ++x, ++data)
			{
				const float NoV = Saturate((x + 0.5f) / width);
				glm::vec3 r = { dfvFunction(NoV, linear_roughness, 1024), 0 };
				if (cloth)
				{
					r.b = float(DFV_Charlie_Uniform(NoV, linear_roughness, 4096));
				}

				*data = r;
			}


		}
		
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