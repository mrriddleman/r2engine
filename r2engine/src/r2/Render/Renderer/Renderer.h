
#ifndef __RENDERER_H__
#define __RENDERER_H__

#include "r2/Core/Memory/Memory.h"
#include "r2/Core/Containers/SArray.h"
#include "r2/Render/Renderer/RendererTypes.h"
#include "r2/Render/Renderer/Shader.h"
#include "r2/Render/Renderer/BufferLayout.h"
#include "r2/Render/Renderer/RenderKey.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Renderer/Commands.h"
#include "r2/Render/Renderer/RenderPass.h"
#include "r2/Render/Model/Model.h"
#include "r2/Render/Model/ModelSystem.h"


namespace r2::draw
{


	class BufferLayout;
	struct LightSystem;
	struct BufferLayoutConfiguration;

	struct BufferHandles
	{
		r2::SArray<BufferLayoutHandle>* bufferLayoutHandles = nullptr;
		r2::SArray<VertexBufferHandle>* vertexBufferHandles = nullptr;
		r2::SArray<IndexBufferHandle>* indexBufferHandles = nullptr;
		r2::SArray<DrawIDHandle>* drawIDHandles = nullptr;
	};

	enum DefaultModel
	{
		QUAD = 0,
		CUBE,
		SPHERE,
		CONE,
		CYLINDER,
		FULLSCREEN_TRIANGLE,
		SKYBOX, 
		NUM_DEFAULT_MODELS,
	};

	enum DebugModelType : u32
	{
		DEBUG_QUAD =0,
		DEBUG_CUBE,
		DEBUG_SPHERE,
		DEBUG_CONE,
		DEBUG_CYLINDER,

		DEBUG_ARROW,
		DEBUG_LINE,

		NUM_DEBUG_MODELS
	};

	struct MaterialBatch
	{
		struct Info
		{
			s32 start = -1;
			s32 numMaterials = 0;
		};

		r2::SArray<Info>* infos = nullptr;
		r2::SArray<MaterialHandle>* materialHandles = nullptr;
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

	struct VertexLayoutConfigHandle
	{
		BufferLayoutHandle mBufferLayoutHandle;
		VertexBufferHandle mVertexBufferHandles[BufferLayoutConfiguration::MAX_VERTEX_BUFFER_CONFIGS];
		IndexBufferHandle mIndexBufferHandle;
		u32 mNumVertexBufferHandles;
	};

	struct VertexLayoutVertexOffset
	{
		u64 baseVertex = 0;
		u64 numVertices = 0;
	};

	struct VertexLayoutIndexOffset
	{
		u64 baseIndex = 0;
		u64 numIndices = 0;
	};

	struct VertexLayoutUploadOffset
	{
		VertexLayoutVertexOffset mVertexBufferOffset;
		VertexLayoutIndexOffset mIndexBufferOffset;
	};

	struct ClearSurfaceOptions
	{
		b32 shouldClear = false;
		u32 flags = 0;
	};

	struct RenderBatch
	{
		VertexConfigHandle vertexLayoutConfigHandle = InvalidVertexConfigHandle;

		ConstantBufferHandle subCommandsHandle;
		ConstantBufferHandle modelsHandle;
		ConstantBufferHandle materialsHandle;
		ConstantBufferHandle boneTransformOffsetsHandle;
		ConstantBufferHandle boneTransformsHandle;

		r2::SArray<ModelRef>* modelRefs = nullptr;

		MaterialBatch materialBatch;

		r2::SArray<glm::mat4>* models = nullptr;
		r2::SArray<r2::draw::ShaderBoneTransform>* boneTransforms = nullptr;
		r2::SArray<cmd::DrawState>* drawState = nullptr; //stuff to help generate the keys

		PrimitiveType primitiveType = PrimitiveType::TRIANGLES;

		static u64 MemorySize(u64 numModels, u64 numModelRefs, u64 numBoneTransforms, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

#ifdef R2_DEBUG

	struct DebugRenderConstants
	{
		glm::vec4 color;
		glm::mat4 modelMatrix;
	};

	struct DebugRenderBatch
	{
		DebugDrawType debugDrawType;

		VertexConfigHandle vertexConfigHandle = InvalidVertexConfigHandle;
		r2::draw::MaterialHandle materialHandle = mat::InvalidMaterial;

		ConstantConfigHandle subCommandsConstantConfigHandle = InvalidConstantConfigHandle;
		ConstantBufferHandle renderDebugConstantsConfigHandle = InvalidConstantConfigHandle;

		r2::SArray<DebugModelType>* debugModelTypesToDraw = nullptr;
		r2::SArray<math::Transform>* transforms = nullptr; //this is for the models - @TODO(Serge): maybe should just convert the transforms in the draw func, but might be more efficient to just do it a big loop so that's what we're doing for now
		r2::SArray<glm::mat4>* matTransforms = nullptr; //this is for the lines
		r2::SArray<glm::vec4>* colors = nullptr;
		r2::SArray<DebugVertex>* vertices = nullptr;
		r2::SArray<DrawFlags>* drawFlags = nullptr;

		static u64 MemorySize(u32 maxDraws, bool hasDebugLines, u64 alignment, u32 headerSize, u32 boundsChecking);
	};

#endif

	template<typename T> struct CommandBucket;



	struct Renderer
	{
		RendererBackend mBackendType;
		//memory
		r2::mem::MemoryArea::Handle mMemoryAreaHandle = r2::mem::MemoryArea::Invalid;
		r2::mem::MemoryArea::SubArea::Handle mSubAreaHandle = r2::mem::MemoryArea::SubArea::Invalid;
		r2::mem::LinearArena* mSubAreaArena = nullptr;

		//@TODO(Serge): don't expose this to the outside (or figure out how to remove this)
		//				we should only be exposing/using mVertexLayoutConfigHandles
		r2::draw::BufferHandles mBufferHandles;
		r2::SArray<r2::draw::ConstantBufferHandle>* mConstantBufferHandles = nullptr;
		r2::SHashMap<ConstantBufferData>* mConstantBufferData = nullptr;


		r2::SArray<VertexLayoutConfigHandle>* mVertexLayoutConfigHandles = nullptr;
		r2::SArray<r2::draw::BufferLayoutConfiguration>* mVertexLayouts = nullptr;
		r2::SArray<r2::draw::ConstantBufferLayoutConfiguration>* mConstantLayouts = nullptr;
		r2::SArray<VertexLayoutUploadOffset>* mVertexLayoutUploadOffsets = nullptr;

		//@TODO(Serge): Maybe should move these to somewhere else?
		r2::SArray<r2::draw::ModelRef>* mEngineModelRefs = nullptr;
		ModelSystem* mModelSystem = nullptr;
		r2::SArray<ModelHandle>* mDefaultModelHandles = nullptr;
		MaterialSystem* mMaterialSystem = nullptr;



		r2::draw::MaterialHandle mFinalCompositeMaterialHandle;

		VertexConfigHandle mStaticVertexModelConfigHandle = InvalidVertexConfigHandle;
		VertexConfigHandle mAnimVertexModelConfigHandle = InvalidVertexConfigHandle;
		VertexConfigHandle mFinalBatchVertexLayoutConfigHandle = InvalidVertexConfigHandle;

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

		r2::mem::StackArena* mRenderTargetsArena = nullptr;

		util::Size mResolutionSize;
		util::Size mCompositeSize;

		RenderTarget mRenderTargets[NUM_RENDER_TARGET_SURFACES];
		RenderPass* mRenderPasses[NUM_RENDER_PASSES];

		//	RenderTarget mOffscreenRenderTarget;
		//	RenderTarget mScreenRenderTarget;


			//@TODO(Serge): Each bucket needs the bucket and an arena for that bucket. We should partition the AUX memory properly
		r2::draw::CommandBucket<r2::draw::key::Basic>* mPreRenderBucket = nullptr;
		r2::draw::CommandBucket<r2::draw::key::Basic>* mPostRenderBucket = nullptr;
		r2::mem::StackArena* mPrePostRenderCommandArena = nullptr;

		r2::draw::CommandBucket<r2::draw::key::Basic>* mCommandBucket = nullptr;
		r2::draw::CommandBucket<r2::draw::key::Basic>* mFinalBucket = nullptr;
		r2::mem::StackArena* mCommandArena = nullptr;

		r2::SArray<RenderBatch>* mRenderBatches = nullptr; //should be size of NUM_DRAW_TYPES

#ifdef R2_DEBUG
		r2::draw::MaterialHandle mDebugLinesMaterialHandle;
		r2::draw::MaterialHandle mDebugModelMaterialHandle;

		VertexConfigHandle mDebugLinesVertexConfigHandle = InvalidVertexConfigHandle;
		VertexConfigHandle mDebugModelVertexConfigHandle = InvalidVertexConfigHandle;

		ConstantConfigHandle mDebugLinesSubCommandsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mDebugModelSubCommandsConfigHandle = InvalidConstantConfigHandle;
		ConstantConfigHandle mDebugRenderConstantsConfigHandle = InvalidConstantConfigHandle;

		r2::draw::CommandBucket<key::DebugKey>* mDebugCommandBucket = nullptr;
		r2::draw::CommandBucket<key::DebugKey>* mPreDebugCommandBucket = nullptr;
		r2::draw::CommandBucket<key::DebugKey>* mPostDebugCommandBucket = nullptr;
		r2::mem::StackArena* mDebugCommandArena = nullptr;

		r2::SArray<DebugRenderBatch>* mDebugRenderBatches = nullptr;
#endif

	};
}




namespace r2::draw::renderer
{
	//basic stuff
	Renderer* CreateRenderer(RendererBackend backendType, r2::mem::MemoryArea::Handle memoryAreaHandle, const char* shaderManifestPath, const char* internalShaderManifestPath);
	void Update(Renderer& renderer);
	void Render(Renderer& renderer, float alpha);
	void Shutdown(Renderer* renderer);
	u64 MemorySize(u64 materialSystemMemorySize);

	
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

	const Model* GetDefaultModel( r2::draw::DefaultModel defaultModel);
	const r2::SArray<r2::draw::ModelRef>* GetDefaultModelRefs();
	r2::draw::ModelRef GetDefaultModelRef( r2::draw::DefaultModel defaultModel);

	void LoadEngineTexturesFromDisk();
	void UploadEngineMaterialTexturesToGPUFromMaterialName( u64 materialName);
	void UploadEngineMaterialTexturesToGPU();

	ModelRef UploadModel(const Model* model);
	void UploadModels(const r2::SArray<const Model*>& models, r2::SArray<ModelRef>& modelRefs);

	ModelRef UploadAnimModel(const AnimModel* model);
	void UploadAnimModels(const r2::SArray<const AnimModel*>& models, r2::SArray<ModelRef>& modelRefs);

	//@TODO(Serge): do we want these methods? Maybe at least not public?
	void ClearVertexLayoutOffsets( VertexConfigHandle vHandle);
	void ClearAllVertexLayoutOffsets();
	
	void GetDefaultModelMaterials( r2::SArray<r2::draw::MaterialHandle>& defaultModelMaterials);
	r2::draw::MaterialHandle GetMaterialHandleForDefaultModel(r2::draw::DefaultModel defaultModel);

	void UpdatePerspectiveMatrix(const glm::mat4& perspectiveMatrix);
	void UpdateViewMatrix(const glm::mat4& viewMatrix);
	void UpdateCameraPosition(const glm::vec3& camPosition);
	void UpdateExposure(float exposure);
	void UpdateSceneLighting(const r2::draw::LightSystem& lightSystem);

	void DrawModels(const r2::SArray<ModelRef>& modelRefs, const r2::SArray<glm::mat4>& modelMatrices, const r2::SArray<DrawFlags>& flags, const r2::SArray<ShaderBoneTransform>* boneTransforms);
	void DrawModel(const ModelRef& modelRef, const glm::mat4& modelMatrix, const DrawFlags& flags, const r2::SArray<ShaderBoneTransform>* boneTransforms);
	
	void DrawModelOnLayer(DrawLayer layer, const ModelRef& modelRef, const r2::SArray<MaterialHandle>* materials, const glm::mat4& modelMatrix, const DrawFlags& flags, const r2::SArray<ShaderBoneTransform>* boneTransforms);
	void DrawModelsOnLayer(DrawLayer layer, const r2::SArray<ModelRef>& modelRefs, const r2::SArray<MaterialHandle>* materialHandles, const r2::SArray<glm::mat4>& modelMatrices, const r2::SArray<DrawFlags>& flags, const r2::SArray<ShaderBoneTransform>* boneTransforms);
	
	///More draw functions...


	//------------------------------------------------------------------------------

#ifdef R2_DEBUG

	void DrawDebugBones(const r2::SArray<DebugBone>& bones, const glm::mat4& modelMatrix, const glm::vec4& color);

	void DrawDebugBones(
		const r2::SArray<DebugBone>& bones,
		const r2::SArray<u64>& numBonesPerModel,
		const r2::SArray<glm::mat4>& numModelMats,
		const glm::vec4& color);


	void DrawSphere(const glm::vec3& center, float radius, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCube(const glm::vec3& center, float scale, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCylinder(const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawCone(const glm::vec3& basePosition, const glm::vec3& dir, float radius, float height, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawArrow(const glm::vec3& basePosition, const glm::vec3& dir, float length, float headBaseRadius, const glm::vec4& color, bool filled, bool depthTest = true);
	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, bool disableDepth);
	void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::mat4& modelMat, const glm::vec4& color, bool depthTest);

	void DrawTangentVectors(DefaultModel model, const glm::mat4& transform);
#endif


}

#endif