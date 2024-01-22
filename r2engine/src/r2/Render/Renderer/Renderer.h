
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Model/Shader/Shader.h"
#include "r2/Render/Model/Shader/ShaderEffect.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/RenderPass.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/ModelCache.h"
#include "r2/Render/Model/Light.h"
#include "r2/Render/Renderer/VertexBufferLayoutSystem.h"
#include "r2/Render/Model/RenderMaterials/RenderMaterialCache.h"

namespace r2
{
	struct Camera;
}

namespace r2::draw
{
	
	struct ColorCorrection
	{
		float contrast = 1.0f;
		float brightness = 0.0f;
		float saturation = 1.0f;
		float gamma = 1.0f / 2.2f;

		float filmGrainStrength = 0.02f;
		float mColorGradingHalfColX = 0.0f;
		float mColorGradingHalfColY = 0.0f;
		float mNumColorGradingSwatches = 1.0f;
		tex::TextureAddress mColorGradingLUT;
		

		f32 mColorGradingContribution = 0.0f;
		f32 temperature = 0.1f;
		f32 tint = 0.1f;
		f32 unused = 0.0f;
	};

	struct MaterialBatch
	{
		struct Info
		{
			s32 start = -1;
			s32 numMaterials = 0;
		};

		r2::SArray<Info>* infos = nullptr;

		//@NOTE(Serge): I THINK these should just be MaterialNames and we'll resolve them later?
		//				alternatively we could resolve them into flat::Material* here instead?
		//				Or we could resolve the RenderMaterialParams and ALSO have SArray<flat::Material*> just for the shaders (or the flat::ShaderEffectPasses)
		r2::SArray<r2::draw::RenderMaterialParams>* renderMaterialParams = nullptr;
		r2::SArray<r2::draw::ShaderEffectPasses>* shaderEffectPasses = nullptr;

		//r2::SArray<r2::draw::ShaderHandle>* shaderHandles = nullptr;

	};

	struct EntityInstanceBatchOffset
	{
		s32 start = -1;
		u32 numInstances = 0;
	};

	struct ConstantBufferData
	{
		r2::draw::ConstantBufferHandle handle = EMPTY_BUFFER;
		r2::draw::ConstantBufferLayout::Type type = r2::draw::ConstantBufferLayout::Type::Small;
		b32 isPersistent = false;
		u64 bufferSize = 0;
		u64 currentOffset = 0;

		void AddDataSize(u64 size);
	};

	struct Model;

	enum eVertexBufferLayoutTypes : u8
	{
		VBL_STATIC = 0,
		VBL_ANIMATED,
#if defined R2_DEBUG
		VBL_DEBUG_LINES,
#endif
		NUM_VERTEX_BUFFER_LAYOUT_TYPES,
		VBL_FINAL = VBL_STATIC,
#if defined R2_DEBUG
		VBL_DEBUG_MODEL = VBL_STATIC,
#endif
	};

	struct MeshRenderData
	{
		glm::mat4 globalTransform = glm::mat4(1);
		glm::mat4 globalInvTransform = glm::mat4(1);
	};

	struct RenderBatch
	{
		vb::VertexBufferLayoutHandle vertexBufferLayoutHandle = vb::InvalidVertexBufferLayoutHandle;

		ConstantBufferHandle subCommandsHandle;
		ConstantBufferHandle modelsHandle;
		ConstantBufferHandle materialsHandle;
		ConstantBufferHandle materialOffsetsHandle;
		ConstantBufferHandle boneTransformOffsetsHandle;
		ConstantBufferHandle boneTransformsHandle;

		r2::SArray<const vb::GPUModelRef*>* gpuModelRefs = nullptr;
		//@TODO(Serge): Might be a good idea to store all of the shaderIDs that the meshrefs use as well so we don't have to dynamically allocate the shaders array in the populate method later on

		MaterialBatch materialBatch;

		r2::SArray<glm::mat4>* models = nullptr;
		r2::SArray<r2::draw::ShaderBoneTransform>* boneTransforms = nullptr;
		r2::SArray<cmd::DrawState>* drawState = nullptr; //stuff to help generate the keys

		r2::SArray<MeshRenderData>* meshRenderData = nullptr;

#ifdef R2_EDITOR
		r2::SArray<u32>* entityIDs = nullptr;

		r2::SArray<EntityInstanceBatchOffset>* entityInstanceOffsetBatches = nullptr;
		r2::SArray<s32>* entityInstances = nullptr;
#endif

		r2::SArray<u32>* numInstances = nullptr;
		r2::SArray<b32>* useSameBoneTransformsForInstances = nullptr;
		PrimitiveType primitiveType = PrimitiveType::TRIANGLES;

		static u64 MemorySize(u32 numModels, u32 numModelRefs, u32 numMeshes, u32 numBoneTransforms, u64 alignment, u32 headerSize, u32 boundsChecking);

	};

struct BatchRenderOffsets
	{
		//@TODO(Serge): Should this be the forward pass' shader? or should this be ShaderEffectPasses?
		ShaderEffectPasses shaderEffectPasses;
		PrimitiveType primitiveType;
		cmd::DrawState drawState;
		u32 subCommandsOffset = 0;
		u32 numSubCommands = 0;
		u32 cameraDepth;
		u32 depthFunction;
		b32 isDynamic = false;
		u8 blendingFunctionKeyValue = key::TR_OPAQUE;

	};

#ifdef R2_DEBUG

	struct DebugRenderConstants
	{
		glm::vec4 color;
		glm::mat4 modelMatrix;
	};

	

	struct DebugDrawCommandData
	{
		ShaderHandle shaderID = InvalidShader;
		PrimitiveType primitiveType = PrimitiveType::LINES;
		b32 depthEnabled = false;

		r2::SArray<cmd::DrawBatchSubCommand>* debugModelDrawBatchCommands = nullptr;
		r2::SArray<cmd::DrawDebugBatchSubCommand>* debugLineDrawBatchCommands = nullptr;
	};

	struct DebugRenderBatch
	{
		DebugDrawType debugDrawType;

		vb::VertexBufferLayoutHandle vertexBufferLayoutHandle = vb::InvalidVertexBufferLayoutHandle;
		r2::draw::ShaderHandle shaderHandle = r2::draw::InvalidShader;

		ConstantConfigHandle subCommandsConstantConfigHandle = InvalidConstantConfigHandle;
		ConstantBufferHandle renderDebugConstantsConfigHandle = InvalidConstantConfigHandle;

		r2::SArray<DebugModelType>* debugModelTypesToDraw = nullptr;
		r2::SArray<DebugRenderConstants>* debugRenderConstants = nullptr;
		r2::SArray<DebugVertex>* vertices = nullptr;
		r2::SArray<DrawFlags>* drawFlags = nullptr;
		r2::SArray<u32>* numInstances = nullptr;

		DebugDrawCommandData debugDrawCommandData[2]; //0 - depth, 1 - no depth

		static u64 MemorySize(u32 maxDraws, bool hasDebugLines, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

#endif

	template<typename T> struct CommandBucket;

	enum eRendererFlags : u32
	{
		RENDERER_FLAG_NEEDS_SHADOW_MAPS_REFRESH = 1 << 0,
		RENDERER_FLAG_NEEDS_CLUSTER_VOLUME_TILE_UPDATE = 1 << 1,
		RENDERER_FLAG_NEEDS_ALL_SURFACES_UPLOAD = 1 << 2,
	};

	using RendererFlags = r2::Flags<u32, u32>;

	struct ShaderMatrices
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 skyboxView;
		glm::mat4 inverseProjection;
		glm::mat4 inverseView;
		glm::mat4 vpMatrix;
		glm::mat4 prevProjection;
		glm::mat4 prevView;
		glm::mat4 prevVPMatrix;
	};

	struct ShaderVectors
	{
		glm::vec4 cameraPosTimeW;
		glm::vec4 exposureNearFar;
		glm::vec4 shadowMapSizes;
		glm::vec4 fovAspectResXResY;
		u32 frame; //might be a problem - dunno yet
		u32 padding;
		glm::vec2 clusterScaleBias;
		glm::uvec4 clusterTileSizes; //{tileSizeX, tileSizeY, tileSizeZ, tileSizePx}
		glm::vec4 jitter;
	};

	struct SDSMShaderParams
	{
		glm::vec4 lightSpaceBorder;
		glm::vec4 maxScale;
		glm::vec4 projMultSplitScaleZMultLambda;
		f32 dilationFactor;
		u32 scatterTileDim;
		u32 reduceTileDim;
		u32 padding;
		glm::vec4 splitScaleMultFadeFactor;
		r2::draw::tex::TextureAddress blueNoiseTexture;
	};

	struct SSRShaderParams
	{
		float mSSRStride;
		float mSSRThickness;
		int mSSRRayMarchIterations;
		float mSSRStrideZCutoff;

		tex::TextureAddress mSSRDitherTexture;
		float mSSRDitherTilingFactor;
		s32 mSSRRoughnessMips;
		s32 mSSRConeTracingSteps;
		float mSSRMaxFadeDistance;
		float mSSRFadeScreenStart;
		float mSSRFadeScreenEnd;
		float mSSRMaxDistance;
	};

	struct BloomShaderParams
	{
		float mBloomThreshold;
		float mBloomKnee;
		float mBloomIntensity;
		float mBloomFilterSize;
	};

	struct AAShaderParams
	{	
		float mFXAALumaThreshold = 0.5f;
		float mFXAALumaMulReduce = 1.0f / 8.0f;
		float mFXAALumaMinReduce = 1.0f / 128.0f;
		float mFXAAMaxSpan = 8.0f;
		glm::vec2 mFXAATexelStep;


		float mSMAAThreshold = 0.05f;
		int mSMAAMaxSearchSteps = 32;
		tex::TextureAddress mSMAAAreaTexture;
		tex::TextureAddress mSMAASearchTexture;
		glm::ivec4 mSMAASampleIndices;
		int mSMAACornerRounding = 25;
		int mSMAAMaxSearchStepsDiag = 16;
		float mSMAACameraMovementWeight;

	};

	struct Renderer
	{
		RendererBackend mBackendType;
		//--------------BEGIN Memory stuff---------------- -
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;
		//--------------END Memory stuff-----------------

		//--------------BEGIN Systems stuff----------------
		ModelCache* mModelCache = nullptr;
		
		
		LightSystem* mLightSystem = nullptr;
		vb::VertexBufferLayoutSystem* mVertexBufferLayoutSystem = nullptr;
		RenderMaterialCache* mRenderMaterialCache = nullptr;

		r2::SArray<vb::GPUModelRefHandle>* mEngineModelRefs = nullptr;
		r2::SArray<ModelHandle>* mDefaultModelHandles = nullptr;

		r2::SArray<vb::VertexBufferLayoutHandle>* mVertexBufferLayoutHandles = nullptr;

		//--------------END Systems stuff----------------

		//--------------BEGIN Buffer Layout stuff-----------------
		r2::SArray<r2::draw::ConstantBufferHandle>* mConstantBufferHandles = nullptr;
		r2::SHashMap<ConstantBufferData>* mConstantBufferData = nullptr;
		r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>* mConstantLayouts = nullptr;
		
		r2::draw::ShaderHandle mFinalCompositeShaderHandle;

		r2::mat::MaterialName mDefaultOutlineMaterialName;
		//r2::draw::ShaderHandle mDefaultStaticOutlineShaderHandle;
		//r2::draw::ShaderHandle mDefaultDynamicOutlineShaderHandle;

		//r2::draw::RenderMaterialParams mDefaultOutlineRenderMaterialParams;

		r2::draw::RenderMaterialParams mMissingTextureRenderMaterialParams;
		r2::draw::RenderMaterialParams mBlueNoiseRenderMaterialParams;
		r2::draw::tex::Texture mMissingTexture;
		r2::draw::tex::Texture mBlueNoiseTexture;

		tex::TextureHandle mSSRDitherTexture;

		vb::VertexBufferLayoutHandle mStaticVertexModelConfigHandle = vb::InvalidVertexBufferLayoutHandle;
		vb::VertexBufferLayoutHandle mAnimVertexModelConfigHandle = vb::InvalidVertexBufferLayoutHandle;

		ConstantConfigHandle mSurfacesConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mModelConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mMaterialConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mSubcommandsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mLightingConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mResolutionConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mBoneTransformOffsetsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mBoneTransformsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mVPMatricesConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mVectorsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mSDSMParamsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mShadowDataConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mMaterialOffsetsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mClusterVolumesConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mDispatchComputeConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mSSRConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mBloomConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mAAConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mColorCorrectionConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mMeshDataConfigHandle = InvalidConstantConfigHandle;

		//--------------END Buffer Layout stuff-----------------

		//------------BEGIN Drawing Stuff--------------
		Camera* mnoptrRenderCam = nullptr;
		ShaderMatrices mShaderMatrices;
		ShaderVectors mShaderVectors;


		//@TODO(Serge): figure out a nicer way to store all this stuff
		//----------------------------------------------------------------
		ShaderHandle mShadowSplitComputeShader;
		ShaderHandle mShadowDepthShaders[2]; //0 - static, 1 - dynamic
		ShaderHandle mDepthShaders[2];
		ShaderHandle mSDSMReduceZBoundsComputeShader;
		ShaderHandle mSDSMCalculateLogPartitionsComputeShader;
		ShaderHandle mShadowSDSMComputeShader;

		ShaderHandle mSpotLightShadowShaders[2];
		ShaderHandle mSpotLightLightMatrixShader;

		ShaderHandle mPointLightShadowShaders[2];
		ShaderHandle mPointLightLightMatrixShader;

		ShaderHandle mAmbientOcclusionShader;
		ShaderHandle mDenoiseShader;
		ShaderHandle mAmbientOcclusionTemporalDenoiseShader;

		ShaderHandle mCreateClusterComputeShader;
		ShaderHandle mMarkActiveClusterTilesComputeShader;
		ShaderHandle mFindUniqueClustersComputeShader;
		ShaderHandle mAssignLightsToClustersComputeShader;
		
		ShaderHandle mTransparentCompositeShader;

		ShaderHandle mSSRShader;
		ShaderHandle mSSRConeTraceShader;

		ShaderHandle mVerticalBlurShader;
		ShaderHandle mHorizontalBlurShader;

		ShaderHandle mBloomDownSampleShader;
		ShaderHandle mBloomBlurPreFilterShader;
		ShaderHandle mBloomUpSampleShader;

		//Output merger shaders
		ShaderHandle mPassThroughShader;
		
		//FXAA
		ShaderHandle mFXAAShader;
		
		//SMAA
		ShaderHandle mSMAAEdgeDetectionShader;
		ShaderHandle mSMAABlendingWeightShader;
		ShaderHandle mSMAANeighborhoodBlendingShader;
		ShaderHandle mSMAANeighborhoodBlendingReprojectionShader;
		ShaderHandle mSMAAReprojectResolveShader;
		ShaderHandle mSMAASeparateShader;

		//MSAA
		ShaderHandle mMSAAResolveNearestShader;
		ShaderHandle mMSAAResolveLinearShader;
		ShaderHandle mMSAAResolveNormalizedShader;

#ifdef R2_EDITOR
		ShaderHandle mEntityColorShader[2]; //0 - static, 1 - dynamic
#endif
		s32 mStaticDirectionLightBatchUniformLocation;
		s32 mDynamicDirectionLightBatchUniformLocation;

		s32 mStaticSpotLightBatchUniformLocation;
		s32 mDynamicSpotLightBatchUniformLocation;

		s32 mStaticPointLightBatchUniformLocation;
		s32 mDynamicPointLightBatchUniformLocation;

		s32 mVerticalBlurTextureContainerLocation;
		s32 mVerticalBlurTexturePageLocation;
		s32 mVerticalBlurTextureLodLocation;

		s32 mHorizontalBlurTextureContainerLocation;
		s32 mHorizontalBlurTexturePageLocation;
		s32 mHorizontalBlurTextureLodLocation;

		s32 mPassThroughTextureContainerLocation;
		s32 mPassThroughTexturePageLocation;
		s32 mPassThroughTextureLodLocation;

		s32 mSMAASeparateTextureContainerLocation;
		s32 mSMAASeparateTexturePageLocation;
		s32 mSMAASeparateTextureLodLocation;

		s32 mMSAAResolveNearestTextureContainerLocation;
		s32 mMSAAResolveNearestTexturePageLocation;
		s32 mMSAAResolveNearestTextureLodLocation;

		s32 mMSAAResolveLinearTextureContainerLocation;
		s32 mMSAAResolveLinearTexturePageLocation;
		s32 mMSAAResolveLinearTextureLodLocation;

		s32 mMSAAResolveNormalizedTextureContainerLocation;
		s32 mMSAAResolveNormalizedTexturePageLocation;
		s32 mMSAAResolveNormalizedTextureLodLocation;

		//----------------------------------------------------------------

		r2::mem::StackArena* mRenderTargetsArena = nullptr;

		util::Size mResolutionSize;
		util::Size mCompositeSize;

		rt::RenderTargetParams mRenderTargetParams[NUM_RENDER_TARGET_SURFACES];
		RenderTarget mRenderTargets[NUM_RENDER_TARGET_SURFACES];
		RenderPass* mRenderPasses[NUM_RENDER_PASSES];

		//@TODO(Serge): Each bucket needs the bucket and an arena for that bucket. We should partition the AUX memory properly
		CommandBucket<key::Basic>* mPreRenderBucket = nullptr;
		CommandBucket<key::Basic>* mPostRenderBucket = nullptr;
		r2::mem::StackArena* mPrePostRenderCommandArena = nullptr;

		CommandBucket<key::Basic>* mCommandBucket = nullptr;
		CommandBucket<key::Basic>* mTransparentBucket = nullptr;
		CommandBucket<key::Basic>* mFinalBucket = nullptr;
		CommandBucket<key::Basic>* mClustersBucket = nullptr;

		CommandBucket<key::Basic>* mSSRBucket = nullptr;
		
#ifdef R2_EDITOR
		CommandBucket<key::Basic>* mEditorPickingBucket = nullptr;
#endif

		r2::mem::StackArena* mCommandArena = nullptr;

		CommandBucket<key::ShadowKey>* mShadowBucket = nullptr;
		CommandBucket<key::DepthKey>* mDepthPrePassBucket = nullptr;
		CommandBucket<key::DepthKey>* mDepthPrePassShadowBucket = nullptr;
		r2::mem::StackArena* mShadowArena = nullptr;

		CommandBucket<key::DepthKey>* mAmbientOcclusionBucket = nullptr;
		CommandBucket<key::DepthKey>* mAmbientOcclusionDenoiseBucket = nullptr;
		CommandBucket<key::DepthKey>* mAmbientOcclusionTemporalDenoiseBucket = nullptr;
		r2::mem::StackArena* mAmbientOcclusionArena = nullptr;

		r2::SArray<RenderBatch>* mRenderBatches = nullptr; //should be size of NUM_DRAW_TYPES
		r2::SArray<RenderMaterialParams>* mRenderMaterialsToRender = nullptr;

		r2::mem::StackArena* mPreRenderStackArena = nullptr;


		//------------END Drawing Stuff--------------

		//------------BEGIN FLAGS------------------
		RendererFlags mFlags;
		//-------------END FLAGS-------------------

	//	u64 mFrameCounter = 0;

		//------------BEGIN Cluster data-----------
//		glm::uvec4 mClusterTileSizes;
		//------------END Cluster data-------------

		//------------BEGIN SDSM Shader Data-------
		SDSMShaderParams mSDSMShaderParams;
		b32 mSDSMShaderParamsNeedUpdate = true;
		//------------END SDSM Shader Data---------


		//------------BEGIN SSR data---------------
		SSRShaderParams mSSRParams;
		bool mSSRNeedsUpdate = true;
		//------------END SSR data-----------------

		//------------BEGIN Bloom data-----------------
		BloomShaderParams mBloomParams;
		bool mBloomParamsNeedsUpdate = true;
		//--------------END Bloom data-----------------

		//-------------BEGIN FXAA Data-----------------
		AAShaderParams mAAParams;
		b32 mFXAANeedsUpdate = true;
		//float mFXAALumaThreshold = 0.5f;
		//float mFXAALumaMulReduce = 1.0f / 8.0f;
		//float mFXAALumaMinReduce = 1.0f / 128.0f;
		//float mFXAAMaxSpan = 8.0f;
		//glm::vec2 mFXAATexelStep;
		
		//--------------END FXAA Data------------------


		//--------------BEGIN SMAA Data----------------
		b32 mSMAANeedsUpdate = true;

		/*
			{r2::draw::ShaderDataType::Float, "smaa_lumaThreshold"},
			{r2::draw::ShaderDataType::Int, "smaa_maxSearchSteps"},
			{r2::draw::ShaderDataType::Struct, "smaUpda_areaTexture"},
			{r2::draw::ShaderDataType::Struct, "smaa_searchTexture"},
			{r2::draw::ShaderDataType::Int4, "smaa_subSampleIndices"},
			{r2::draw::ShaderDataType::Int, "smaa_cornerRounding"},
			{r2::draw::ShaderDataType::Int, "smaa_maxSearchStepsDiag"},
			{r2::draw::ShaderDataType::Float, "smaa_cameraMovementWeight"}
		*/

		//float mSMAAThreshold = 0.05f;
		//int mSMAAMaxSearchSteps = 32;
		tex::TextureHandle mSMAAAreaTexture;
		tex::TextureHandle mSMAASearchTexture;
		
		//int mSMAACornerRounding = 25;
		//int mSMAAMaxSearchStepsDiag = 16;
		glm::vec3 mSMAALastCameraFacingDirection;
		glm::vec3 mSMAALastCameraPosition;
		//--------------END SMAA Data------------------


		//-------BEGIN COLOR CORRECTION Data-----------

		ColorCorrection mColorCorrectionData;
		b32 mColorCorrectionNeedsUpdate = true;
		b32 mColorGradingEnabled = false;

		//--------END COLOR CORRECTION Data------------


		//------------BEGIN Debug Stuff--------------
		//@NOTE(Serge): These config handles need to exist for all configurations since we need to properly line up
		//				the shader layouts
		ConstantConfigHandle mDebugLinesSubCommandsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mDebugModelSubCommandsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mDebugRenderConstantsConfigHandle = InvalidConstantConfigHandle;

#ifdef R2_DEBUG
		r2::draw::ShaderHandle mDebugLinesShaderHandle;
		r2::draw::ShaderHandle mDebugModelShaderHandle;
		r2::draw::ShaderHandle mDebugTransparentModelShaderHandle;
		r2::draw::ShaderHandle mDebugTransparentLineShaderHandle;

		vb::VertexBufferLayoutHandle mDebugLinesVertexConfigHandle = vb::InvalidVertexBufferLayoutHandle;
		vb::VertexBufferLayoutHandle mDebugModelVertexConfigHandle = vb::InvalidVertexBufferLayoutHandle;

		r2::draw::CommandBucket<key::DebugKey>* mDebugCommandBucket = nullptr;
		r2::draw::CommandBucket<key::DebugKey>* mPreDebugCommandBucket = nullptr;
		r2::draw::CommandBucket<key::DebugKey>* mPostDebugCommandBucket = nullptr;
		r2::mem::StackArena* mDebugCommandArena = nullptr;

		r2::SArray<DebugRenderBatch>* mDebugRenderBatches = nullptr;


		r2::SArray<BatchRenderOffsets>* modelBatchRenderOffsets = nullptr;
		r2::SArray<BatchRenderOffsets>* transparentModelBatchRenderOffsets = nullptr;
		r2::SArray<BatchRenderOffsets>* linesBatchRenderOffsets = nullptr;
		r2::SArray<BatchRenderOffsets>* transparentLinesBatchRenderOffsets = nullptr;

		r2::SArray<cmd::DrawBatchSubCommand>* modelDrawBatchSubCommands = nullptr;
		r2::SArray<cmd::DrawDebugBatchSubCommand>* linesDrawBatchSubCommands = nullptr;
		r2::SArray<DebugVertex>* mDebugVertices = nullptr;
#endif
		//------------END Debug Stuff--------------


		OutputMerger mOutputMerger = OutputMerger::OUTPUT_NO_AA;

	};
}

namespace r2::draw::renderer
{
	//basic stuff
	Renderer* CreateRenderer(RendererBackend backendType, r2::mem::MemoryArea::Handle memoryAreaHandle);
	void Update(Renderer& renderer);
	void Render(Renderer& renderer, float alpha);
	void Shutdown(Renderer* renderer);
	u64 MemorySize(u64 renderTargetsMemorySize);

	
	//events
	void WindowResized(Renderer& renderer, u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset);
	void SetWindowSize(Renderer& renderer, u32 windowWidth, u32 windowHeight, u32 resolutionX, u32 resolutionY, float scaleX, float scaleY, float xOffset, float yOffset);
	void MakeCurrent();
	int SetFullscreen(int flags);
	int SetVSYNC(bool vsync);

	//------------------------------------------------------------------------------
	//NEW Proposal

	//Setup code
	void SetClearColor(const glm::vec4& color);
	void SetClearDepth(float color);

	const Model* GetDefaultModel( r2::draw::DefaultModel defaultModel);
	const r2::SArray<r2::draw::vb::GPUModelRefHandle>* GetDefaultModelRefs();
	r2::draw::vb::GPUModelRefHandle GetDefaultModelRef( r2::draw::DefaultModel defaultModel);

	vb::GPUModelRefHandle UploadModel(const Model* model);
	void UploadModels(const r2::SArray<const Model*>& models, r2::SArray<vb::GPUModelRefHandle>& modelRefs);

	vb::GPUModelRefHandle GetModelRefHandleForModelAssetName(const r2::asset::AssetName& modelAssetName);

	void UnloadModel(const vb::GPUModelRefHandle& modelRefHandle);
	void UnloadStaticModelRefHandles(const r2::SArray<vb::GPUModelRefHandle>* handles);
	void UnloadAnimModelRefHandles(const r2::SArray<vb::GPUModelRefHandle>* handles);
	void UnloadAllStaticModels();
	void UnloadAllAnimModels();

	bool IsModelLoaded(const vb::GPUModelRefHandle& modelRefHandle);
	bool IsModelRefHandleValid(const vb::GPUModelRefHandle& modelRefHandle);


	//Render Material Cache methods

	r2::draw::RenderMaterialCache* GetRenderMaterialCache();


	r2::mat::MaterialName GetDefaultOutlineMaterialName();
	//r2::draw::ShaderHandle GetDefaultOutlineShaderHandle(bool isStatic);
	//const r2::draw::RenderMaterialParams& GetDefaultOutlineRenderMaterialParams(bool isStatic);

	const RenderMaterialParams& GetMissingTextureRenderMaterialParam();
	const tex::Texture* GetMissingTexture();

	const RenderMaterialParams& GetBlueNoise64TextureMaterialParam();
	const tex::Texture* GetBlueNoise64Texture();

	void SetRenderCamera(Camera* cameraPtr);
	Camera* GetRenderCamera();

	void SetOutputMergerType(OutputMerger outputMerger);

	DirectionLightHandle AddDirectionLight(const DirectionLight& light);
	PointLightHandle AddPointLight(const PointLight& pointLight);
	SpotLightHandle AddSpotLight(const SpotLight& spotLight);
	SkyLightHandle AddSkyLight(const SkyLight& skylight, s32 numPrefilteredMips);
	SkyLightHandle AddSkyLight(const RenderMaterialParams& diffuseMaterial, const RenderMaterialParams& prefilteredMaterial, const RenderMaterialParams& lutDFG, s32 numMips);

	const DirectionLight* GetDirectionLightConstPtr(DirectionLightHandle dirLightHandle);
	DirectionLight* GetDirectionLightPtr(DirectionLightHandle dirLightHandle);

	const PointLight* GetPointLightConstPtr(PointLightHandle pointLightHandle);
	PointLight* GetPointLightPtr(PointLightHandle pointLightHandle);

	const SpotLight* GetSpotLightConstPtr(SpotLightHandle spotLightHandle);
	SpotLight* GetSpotLightPtr(SpotLightHandle spotLightHandle);

	const SkyLight* GetSkyLightConstPtr(SkyLightHandle skyLightHandle);
	SkyLight* GetSkyLightPtr(SkyLightHandle skyLightHandle);

	//@TODO(Serge): add the get light properties functions here

	void RemoveDirectionLight(DirectionLightHandle dirLightHandle);
	void RemovePointLight(PointLightHandle pointLightHandle);
	void RemoveSpotLight(SpotLightHandle spotLightHandle);
	void RemoveSkyLight(SkyLightHandle skylightHandle);
	void ClearAllLighting();

	void DrawModel(
		const DrawParameters& drawParameters,
		const vb::GPUModelRefHandle& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<r2::mat::MaterialName>& materialNamesPerMesh,
		const r2::SArray<ShaderBoneTransform>* boneTransforms);
	
	void DrawModels(
		const DrawParameters& drawParameters,
		const r2::SArray<vb::GPUModelRefHandle>& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<u32>& numInstancesPerModel,
		
		const r2::SArray<r2::mat::MaterialName>& materialNamesPerMesh,

		const r2::SArray<ShaderBoneTransform>* boneTransforms);

	void SetDefaultStencilState(DrawParameters& drawParameters);
	void SetDefaultBlendState(DrawParameters& drawParameters);
	void SetDefaultCullState(DrawParameters& drawParameters);

	void SetDefaultDrawParameters(DrawParameters& drawParameters);

	void SetColorCorrection(const ColorCorrection& cc);
	void SetColorGradingLUT(const tex::Texture* lut, u32 numSwatches);
	void EnableColorGrading(bool isEnabled);
	void SetColorGradingContribution(float contribution);


	///More draw functions...
	ShaderHandle GetShadowDepthShaderHandle(bool isDynamic, light::LightType lightType);
	ShaderHandle GetDepthShaderHandle(bool isDynamic);
	//------------------------------------------------------------------------------


	//@Temporary
	const vb::GPUModelRef* GetGPUModelRef(const vb::GPUModelRefHandle& handle);

	vb::VertexBufferLayoutSize GetStaticVertexBufferRemainingSize();
	vb::VertexBufferLayoutSize GetAnimVertexBufferRemainingSize();

	vb::VertexBufferLayoutSize GetStaticVertexBufferSize();
	vb::VertexBufferLayoutSize GetAnimVertexBufferSize();

	vb::VertexBufferLayoutSize GetStaticVertexBufferCapacity();
	vb::VertexBufferLayoutSize GetAnimVertexBufferCapacity();

	u32 GetMaxNumModelsLoadedAtOneTimePerLayout();
	u32 GetAVGMaxNumMeshesPerModel();
	u32 GetMaxNumInstances();
	u32 GetMaxNumBones();

#ifdef R2_EDITOR
	//r2::asset::FileList GetModelFiles();
	const r2::draw::Model* GetDefaultModel(const r2::asset::AssetName& assetName);

	struct EntityInstance
	{
		u32 entityId;
		s32 instanceId;
	};

	EntityInstance ReadEntityInstanceAtMousePosition(s32 x, s32 y);

	void DrawModelEntity(
		u32 entity,
		const r2::SArray<s32>& entityInstances,
		const DrawParameters& drawParameters,
		const vb::GPUModelRefHandle& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<r2::mat::MaterialName>& materialNamePerMesh,
		const r2::SArray<ShaderBoneTransform>* boneTransforms);

	void DrawModelEntities(
		const r2::SArray<u32>& entities,
		const r2::SArray<s32>& entityInstances,
		const r2::SArray<u32>& numEntityInstancesPerEntity,
		const DrawParameters& drawParameters,
		const r2::SArray<vb::GPUModelRefHandle>& modelRefHandles,
		const r2::SArray<glm::mat4>& modelMatrices,
		const r2::SArray<u32>& numInstancesPerModel,
		const r2::SArray<r2::mat::MaterialName>& materialNamesPerMesh,
		const r2::SArray<ShaderBoneTransform>* boneTransforms);


#endif

#ifdef R2_DEBUG

	void DrawDebugBones(const r2::SArray<DebugBone>& bones, const glm::mat4& modelMatrix, const glm::vec4& color);

	void DrawDebugBones(
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& numModelMats,
		const glm::vec4& color);

	void DrawQuad(const glm::vec3& center, glm::vec2 scale, const glm::vec3& normal, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCube(const glm::vec3& center, float scale, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCylinder(const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCone(const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawArrow(const glm::vec3& basePosition, const glm::vec3& dir, float length, float headBaseRadius, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, bool disableDepth);
	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::mat4& modelMat, const glm::vec4& color, bool depthTest);

	void DrawTangentVectors(DefaultModel model, const glm::mat4& transform);

	void DrawQuadInstanced(const r2::SArray<glm::vec3>& centers, const r2::SArray<glm::vec2>& scales, const r2::SArray<glm::vec3>& normals, const r2::SArray<glm::vec4>& colors, bool filled, bool depthTest = true);

	void DrawSphereInstanced(
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<float>& radii,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

	void DrawCubeInstanced(
		const r2::SArray<glm::vec3>& centers,
		const r2::SArray<float>& scales,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

	void DrawCylinderInstanced(
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& radii,
		const r2::SArray<float>& heights,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

	void DrawConeInstanced(
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& radii,
		const r2::SArray<float>& heights,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);

	void DrawArrowInstanced(
		const r2::SArray<glm::vec3>& basePositions,
		const r2::SArray<glm::vec3>& directions,
		const r2::SArray<float>& lengths,
		const r2::SArray<float>& headBaseRadii,
		const r2::SArray<glm::vec4>& colors,
		bool filled,
		bool depthTest);
#endif


}

#endif