// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_RMODEL_FLAT_H_
#define FLATBUFFERS_GENERATED_RMODEL_FLAT_H_

#include "flatbuffers/flatbuffers.h"

#include "AssetName_generated.h"
#include "Mesh_generated.h"
#include "RAnimation_generated.h"

namespace flat {

struct RMesh;
struct RMeshBuilder;

struct Transform;

struct Vertex4;

struct Vertex4i;

struct Matrix4;

struct Matrix3;

struct BoneData;

struct BoneInfo;

struct AnimationData;
struct AnimationDataBuilder;

struct GLTFMeshInfo;
struct GLTFMeshInfoBuilder;

struct RModel;
struct RModelBuilder;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Transform FLATBUFFERS_FINAL_CLASS {
 private:
  float position_[3];
  float scale_[3];
  float rotation_[4];

 public:
  Transform() {
    memset(static_cast<void *>(this), 0, sizeof(Transform));
  }
  const flatbuffers::Array<float, 3> *position() const {
    return reinterpret_cast<const flatbuffers::Array<float, 3> *>(position_);
  }
  flatbuffers::Array<float, 3> *mutable_position() {
    return reinterpret_cast<flatbuffers::Array<float, 3> *>(position_);
  }
  const flatbuffers::Array<float, 3> *scale() const {
    return reinterpret_cast<const flatbuffers::Array<float, 3> *>(scale_);
  }
  flatbuffers::Array<float, 3> *mutable_scale() {
    return reinterpret_cast<flatbuffers::Array<float, 3> *>(scale_);
  }
  const flatbuffers::Array<float, 4> *rotation() const {
    return reinterpret_cast<const flatbuffers::Array<float, 4> *>(rotation_);
  }
  flatbuffers::Array<float, 4> *mutable_rotation() {
    return reinterpret_cast<flatbuffers::Array<float, 4> *>(rotation_);
  }
};
FLATBUFFERS_STRUCT_END(Transform, 40);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Vertex4 FLATBUFFERS_FINAL_CLASS {
 private:
  float v_[4];

 public:
  Vertex4() {
    memset(static_cast<void *>(this), 0, sizeof(Vertex4));
  }
  const flatbuffers::Array<float, 4> *v() const {
    return reinterpret_cast<const flatbuffers::Array<float, 4> *>(v_);
  }
  flatbuffers::Array<float, 4> *mutable_v() {
    return reinterpret_cast<flatbuffers::Array<float, 4> *>(v_);
  }
};
FLATBUFFERS_STRUCT_END(Vertex4, 16);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Vertex4i FLATBUFFERS_FINAL_CLASS {
 private:
  int32_t v_[4];

 public:
  Vertex4i() {
    memset(static_cast<void *>(this), 0, sizeof(Vertex4i));
  }
  const flatbuffers::Array<int32_t, 4> *v() const {
    return reinterpret_cast<const flatbuffers::Array<int32_t, 4> *>(v_);
  }
  flatbuffers::Array<int32_t, 4> *mutable_v() {
    return reinterpret_cast<flatbuffers::Array<int32_t, 4> *>(v_);
  }
};
FLATBUFFERS_STRUCT_END(Vertex4i, 16);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Matrix4 FLATBUFFERS_FINAL_CLASS {
 private:
  flat::Vertex4 cols_[4];

 public:
  Matrix4() {
    memset(static_cast<void *>(this), 0, sizeof(Matrix4));
  }
  const flatbuffers::Array<flat::Vertex4, 4> *cols() const {
    return reinterpret_cast<const flatbuffers::Array<flat::Vertex4, 4> *>(cols_);
  }
  flatbuffers::Array<flat::Vertex4, 4> *mutable_cols() {
    return reinterpret_cast<flatbuffers::Array<flat::Vertex4, 4> *>(cols_);
  }
};
FLATBUFFERS_STRUCT_END(Matrix4, 64);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Matrix3 FLATBUFFERS_FINAL_CLASS {
 private:
  flat::Vertex3 cols_[3];

 public:
  Matrix3() {
    memset(static_cast<void *>(this), 0, sizeof(Matrix3));
  }
  const flatbuffers::Array<flat::Vertex3, 3> *cols() const {
    return reinterpret_cast<const flatbuffers::Array<flat::Vertex3, 3> *>(cols_);
  }
  flatbuffers::Array<flat::Vertex3, 3> *mutable_cols() {
    return reinterpret_cast<flatbuffers::Array<flat::Vertex3, 3> *>(cols_);
  }
};
FLATBUFFERS_STRUCT_END(Matrix3, 36);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) BoneData FLATBUFFERS_FINAL_CLASS {
 private:
  flat::Vertex4 boneWeights_;
  flat::Vertex4i boneIDs_;

 public:
  BoneData() {
    memset(static_cast<void *>(this), 0, sizeof(BoneData));
  }
  BoneData(const flat::Vertex4 &_boneWeights, const flat::Vertex4i &_boneIDs)
      : boneWeights_(_boneWeights),
        boneIDs_(_boneIDs) {
  }
  const flat::Vertex4 &boneWeights() const {
    return boneWeights_;
  }
  flat::Vertex4 &mutable_boneWeights() {
    return boneWeights_;
  }
  const flat::Vertex4i &boneIDs() const {
    return boneIDs_;
  }
  flat::Vertex4i &mutable_boneIDs() {
    return boneIDs_;
  }
};
FLATBUFFERS_STRUCT_END(BoneData, 32);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) BoneInfo FLATBUFFERS_FINAL_CLASS {
 private:
  flat::Matrix4 offsetTransform_;

 public:
  BoneInfo() {
    memset(static_cast<void *>(this), 0, sizeof(BoneInfo));
  }
  BoneInfo(const flat::Matrix4 &_offsetTransform)
      : offsetTransform_(_offsetTransform) {
  }
  const flat::Matrix4 &offsetTransform() const {
    return offsetTransform_;
  }
  flat::Matrix4 &mutable_offsetTransform() {
    return offsetTransform_;
  }
};
FLATBUFFERS_STRUCT_END(BoneInfo, 64);

struct RMesh FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef RMeshBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_MATERIALINDEX = 4,
    VT_DATA = 6
  };
  int32_t materialIndex() const {
    return GetField<int32_t>(VT_MATERIALINDEX, 0);
  }
  bool mutate_materialIndex(int32_t _materialIndex) {
    return SetField<int32_t>(VT_MATERIALINDEX, _materialIndex, 0);
  }
  const flatbuffers::Vector<int8_t> *data() const {
    return GetPointer<const flatbuffers::Vector<int8_t> *>(VT_DATA);
  }
  flatbuffers::Vector<int8_t> *mutable_data() {
    return GetPointer<flatbuffers::Vector<int8_t> *>(VT_DATA);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, VT_MATERIALINDEX) &&
           VerifyOffset(verifier, VT_DATA) &&
           verifier.VerifyVector(data()) &&
           verifier.EndTable();
  }
};

struct RMeshBuilder {
  typedef RMesh Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_materialIndex(int32_t materialIndex) {
    fbb_.AddElement<int32_t>(RMesh::VT_MATERIALINDEX, materialIndex, 0);
  }
  void add_data(flatbuffers::Offset<flatbuffers::Vector<int8_t>> data) {
    fbb_.AddOffset(RMesh::VT_DATA, data);
  }
  explicit RMeshBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  RMeshBuilder &operator=(const RMeshBuilder &);
  flatbuffers::Offset<RMesh> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<RMesh>(end);
    return o;
  }
};

inline flatbuffers::Offset<RMesh> CreateRMesh(
    flatbuffers::FlatBufferBuilder &_fbb,
    int32_t materialIndex = 0,
    flatbuffers::Offset<flatbuffers::Vector<int8_t>> data = 0) {
  RMeshBuilder builder_(_fbb);
  builder_.add_data(data);
  builder_.add_materialIndex(materialIndex);
  return builder_.Finish();
}

inline flatbuffers::Offset<RMesh> CreateRMeshDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    int32_t materialIndex = 0,
    const std::vector<int8_t> *data = nullptr) {
  auto data__ = data ? _fbb.CreateVector<int8_t>(*data) : 0;
  return flat::CreateRMesh(
      _fbb,
      materialIndex,
      data__);
}

struct AnimationData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef AnimationDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_BONEDATA = 4,
    VT_BONEINFO = 6,
    VT_PARENTS = 8,
    VT_RESTPOSETRANSFORMS = 10,
    VT_BINDPOSETRANSFORMS = 12,
    VT_JOINTNAMES = 14,
    VT_JOINTNAMESTRINGS = 16
  };
  const flatbuffers::Vector<const flat::BoneData *> *boneData() const {
    return GetPointer<const flatbuffers::Vector<const flat::BoneData *> *>(VT_BONEDATA);
  }
  flatbuffers::Vector<const flat::BoneData *> *mutable_boneData() {
    return GetPointer<flatbuffers::Vector<const flat::BoneData *> *>(VT_BONEDATA);
  }
  const flatbuffers::Vector<const flat::BoneInfo *> *boneInfo() const {
    return GetPointer<const flatbuffers::Vector<const flat::BoneInfo *> *>(VT_BONEINFO);
  }
  flatbuffers::Vector<const flat::BoneInfo *> *mutable_boneInfo() {
    return GetPointer<flatbuffers::Vector<const flat::BoneInfo *> *>(VT_BONEINFO);
  }
  const flatbuffers::Vector<int32_t> *parents() const {
    return GetPointer<const flatbuffers::Vector<int32_t> *>(VT_PARENTS);
  }
  flatbuffers::Vector<int32_t> *mutable_parents() {
    return GetPointer<flatbuffers::Vector<int32_t> *>(VT_PARENTS);
  }
  const flatbuffers::Vector<const flat::Transform *> *restPoseTransforms() const {
    return GetPointer<const flatbuffers::Vector<const flat::Transform *> *>(VT_RESTPOSETRANSFORMS);
  }
  flatbuffers::Vector<const flat::Transform *> *mutable_restPoseTransforms() {
    return GetPointer<flatbuffers::Vector<const flat::Transform *> *>(VT_RESTPOSETRANSFORMS);
  }
  const flatbuffers::Vector<const flat::Transform *> *bindPoseTransforms() const {
    return GetPointer<const flatbuffers::Vector<const flat::Transform *> *>(VT_BINDPOSETRANSFORMS);
  }
  flatbuffers::Vector<const flat::Transform *> *mutable_bindPoseTransforms() {
    return GetPointer<flatbuffers::Vector<const flat::Transform *> *>(VT_BINDPOSETRANSFORMS);
  }
  const flatbuffers::Vector<uint64_t> *jointNames() const {
    return GetPointer<const flatbuffers::Vector<uint64_t> *>(VT_JOINTNAMES);
  }
  flatbuffers::Vector<uint64_t> *mutable_jointNames() {
    return GetPointer<flatbuffers::Vector<uint64_t> *>(VT_JOINTNAMES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *jointNameStrings() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_JOINTNAMESTRINGS);
  }
  flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *mutable_jointNameStrings() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>> *>(VT_JOINTNAMESTRINGS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_BONEDATA) &&
           verifier.VerifyVector(boneData()) &&
           VerifyOffset(verifier, VT_BONEINFO) &&
           verifier.VerifyVector(boneInfo()) &&
           VerifyOffset(verifier, VT_PARENTS) &&
           verifier.VerifyVector(parents()) &&
           VerifyOffset(verifier, VT_RESTPOSETRANSFORMS) &&
           verifier.VerifyVector(restPoseTransforms()) &&
           VerifyOffset(verifier, VT_BINDPOSETRANSFORMS) &&
           verifier.VerifyVector(bindPoseTransforms()) &&
           VerifyOffset(verifier, VT_JOINTNAMES) &&
           verifier.VerifyVector(jointNames()) &&
           VerifyOffset(verifier, VT_JOINTNAMESTRINGS) &&
           verifier.VerifyVector(jointNameStrings()) &&
           verifier.VerifyVectorOfStrings(jointNameStrings()) &&
           verifier.EndTable();
  }
};

struct AnimationDataBuilder {
  typedef AnimationData Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_boneData(flatbuffers::Offset<flatbuffers::Vector<const flat::BoneData *>> boneData) {
    fbb_.AddOffset(AnimationData::VT_BONEDATA, boneData);
  }
  void add_boneInfo(flatbuffers::Offset<flatbuffers::Vector<const flat::BoneInfo *>> boneInfo) {
    fbb_.AddOffset(AnimationData::VT_BONEINFO, boneInfo);
  }
  void add_parents(flatbuffers::Offset<flatbuffers::Vector<int32_t>> parents) {
    fbb_.AddOffset(AnimationData::VT_PARENTS, parents);
  }
  void add_restPoseTransforms(flatbuffers::Offset<flatbuffers::Vector<const flat::Transform *>> restPoseTransforms) {
    fbb_.AddOffset(AnimationData::VT_RESTPOSETRANSFORMS, restPoseTransforms);
  }
  void add_bindPoseTransforms(flatbuffers::Offset<flatbuffers::Vector<const flat::Transform *>> bindPoseTransforms) {
    fbb_.AddOffset(AnimationData::VT_BINDPOSETRANSFORMS, bindPoseTransforms);
  }
  void add_jointNames(flatbuffers::Offset<flatbuffers::Vector<uint64_t>> jointNames) {
    fbb_.AddOffset(AnimationData::VT_JOINTNAMES, jointNames);
  }
  void add_jointNameStrings(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> jointNameStrings) {
    fbb_.AddOffset(AnimationData::VT_JOINTNAMESTRINGS, jointNameStrings);
  }
  explicit AnimationDataBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  AnimationDataBuilder &operator=(const AnimationDataBuilder &);
  flatbuffers::Offset<AnimationData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<AnimationData>(end);
    return o;
  }
};

inline flatbuffers::Offset<AnimationData> CreateAnimationData(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<const flat::BoneData *>> boneData = 0,
    flatbuffers::Offset<flatbuffers::Vector<const flat::BoneInfo *>> boneInfo = 0,
    flatbuffers::Offset<flatbuffers::Vector<int32_t>> parents = 0,
    flatbuffers::Offset<flatbuffers::Vector<const flat::Transform *>> restPoseTransforms = 0,
    flatbuffers::Offset<flatbuffers::Vector<const flat::Transform *>> bindPoseTransforms = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint64_t>> jointNames = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>> jointNameStrings = 0) {
  AnimationDataBuilder builder_(_fbb);
  builder_.add_jointNameStrings(jointNameStrings);
  builder_.add_jointNames(jointNames);
  builder_.add_bindPoseTransforms(bindPoseTransforms);
  builder_.add_restPoseTransforms(restPoseTransforms);
  builder_.add_parents(parents);
  builder_.add_boneInfo(boneInfo);
  builder_.add_boneData(boneData);
  return builder_.Finish();
}

inline flatbuffers::Offset<AnimationData> CreateAnimationDataDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flat::BoneData> *boneData = nullptr,
    const std::vector<flat::BoneInfo> *boneInfo = nullptr,
    const std::vector<int32_t> *parents = nullptr,
    const std::vector<flat::Transform> *restPoseTransforms = nullptr,
    const std::vector<flat::Transform> *bindPoseTransforms = nullptr,
    const std::vector<uint64_t> *jointNames = nullptr,
    const std::vector<flatbuffers::Offset<flatbuffers::String>> *jointNameStrings = nullptr) {
  auto boneData__ = boneData ? _fbb.CreateVectorOfStructs<flat::BoneData>(*boneData) : 0;
  auto boneInfo__ = boneInfo ? _fbb.CreateVectorOfStructs<flat::BoneInfo>(*boneInfo) : 0;
  auto parents__ = parents ? _fbb.CreateVector<int32_t>(*parents) : 0;
  auto restPoseTransforms__ = restPoseTransforms ? _fbb.CreateVectorOfStructs<flat::Transform>(*restPoseTransforms) : 0;
  auto bindPoseTransforms__ = bindPoseTransforms ? _fbb.CreateVectorOfStructs<flat::Transform>(*bindPoseTransforms) : 0;
  auto jointNames__ = jointNames ? _fbb.CreateVector<uint64_t>(*jointNames) : 0;
  auto jointNameStrings__ = jointNameStrings ? _fbb.CreateVector<flatbuffers::Offset<flatbuffers::String>>(*jointNameStrings) : 0;
  return flat::CreateAnimationData(
      _fbb,
      boneData__,
      boneInfo__,
      parents__,
      restPoseTransforms__,
      bindPoseTransforms__,
      jointNames__,
      jointNameStrings__);
}

struct GLTFMeshInfo FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef GLTFMeshInfoBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NUMPRIMITIVES = 4,
    VT_GLOBALINVERSETRANSFORM = 6,
    VT_GLOBALTRANSFORM = 8
  };
  uint32_t numPrimitives() const {
    return GetField<uint32_t>(VT_NUMPRIMITIVES, 0);
  }
  bool mutate_numPrimitives(uint32_t _numPrimitives) {
    return SetField<uint32_t>(VT_NUMPRIMITIVES, _numPrimitives, 0);
  }
  const flat::Matrix4 *globalInverseTransform() const {
    return GetStruct<const flat::Matrix4 *>(VT_GLOBALINVERSETRANSFORM);
  }
  flat::Matrix4 *mutable_globalInverseTransform() {
    return GetStruct<flat::Matrix4 *>(VT_GLOBALINVERSETRANSFORM);
  }
  const flat::Matrix4 *globalTransform() const {
    return GetStruct<const flat::Matrix4 *>(VT_GLOBALTRANSFORM);
  }
  flat::Matrix4 *mutable_globalTransform() {
    return GetStruct<flat::Matrix4 *>(VT_GLOBALTRANSFORM);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_NUMPRIMITIVES) &&
           VerifyField<flat::Matrix4>(verifier, VT_GLOBALINVERSETRANSFORM) &&
           VerifyField<flat::Matrix4>(verifier, VT_GLOBALTRANSFORM) &&
           verifier.EndTable();
  }
};

struct GLTFMeshInfoBuilder {
  typedef GLTFMeshInfo Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_numPrimitives(uint32_t numPrimitives) {
    fbb_.AddElement<uint32_t>(GLTFMeshInfo::VT_NUMPRIMITIVES, numPrimitives, 0);
  }
  void add_globalInverseTransform(const flat::Matrix4 *globalInverseTransform) {
    fbb_.AddStruct(GLTFMeshInfo::VT_GLOBALINVERSETRANSFORM, globalInverseTransform);
  }
  void add_globalTransform(const flat::Matrix4 *globalTransform) {
    fbb_.AddStruct(GLTFMeshInfo::VT_GLOBALTRANSFORM, globalTransform);
  }
  explicit GLTFMeshInfoBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  GLTFMeshInfoBuilder &operator=(const GLTFMeshInfoBuilder &);
  flatbuffers::Offset<GLTFMeshInfo> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<GLTFMeshInfo>(end);
    return o;
  }
};

inline flatbuffers::Offset<GLTFMeshInfo> CreateGLTFMeshInfo(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t numPrimitives = 0,
    const flat::Matrix4 *globalInverseTransform = 0,
    const flat::Matrix4 *globalTransform = 0) {
  GLTFMeshInfoBuilder builder_(_fbb);
  builder_.add_globalTransform(globalTransform);
  builder_.add_globalInverseTransform(globalInverseTransform);
  builder_.add_numPrimitives(numPrimitives);
  return builder_.Finish();
}

struct RModel FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef RModelBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ASSETNAME = 4,
    VT_GLOBALINVERSETRANSFORM = 6,
    VT_MATERIALS = 8,
    VT_MESHES = 10,
    VT_ANIMATIONDATA = 12,
    VT_ANIMATIONS = 14,
    VT_GLTFMESHINFOS = 16
  };
  const flat::AssetName *assetName() const {
    return GetPointer<const flat::AssetName *>(VT_ASSETNAME);
  }
  flat::AssetName *mutable_assetName() {
    return GetPointer<flat::AssetName *>(VT_ASSETNAME);
  }
  const flat::Matrix4 *globalInverseTransform() const {
    return GetStruct<const flat::Matrix4 *>(VT_GLOBALINVERSETRANSFORM);
  }
  flat::Matrix4 *mutable_globalInverseTransform() {
    return GetStruct<flat::Matrix4 *>(VT_GLOBALINVERSETRANSFORM);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>> *materials() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>> *>(VT_MATERIALS);
  }
  flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>> *mutable_materials() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>> *>(VT_MATERIALS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::RMesh>> *meshes() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::RMesh>> *>(VT_MESHES);
  }
  flatbuffers::Vector<flatbuffers::Offset<flat::RMesh>> *mutable_meshes() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<flat::RMesh>> *>(VT_MESHES);
  }
  const flat::AnimationData *animationData() const {
    return GetPointer<const flat::AnimationData *>(VT_ANIMATIONDATA);
  }
  flat::AnimationData *mutable_animationData() {
    return GetPointer<flat::AnimationData *>(VT_ANIMATIONDATA);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::RAnimation>> *animations() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::RAnimation>> *>(VT_ANIMATIONS);
  }
  flatbuffers::Vector<flatbuffers::Offset<flat::RAnimation>> *mutable_animations() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<flat::RAnimation>> *>(VT_ANIMATIONS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::GLTFMeshInfo>> *gltfMeshInfos() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::GLTFMeshInfo>> *>(VT_GLTFMESHINFOS);
  }
  flatbuffers::Vector<flatbuffers::Offset<flat::GLTFMeshInfo>> *mutable_gltfMeshInfos() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<flat::GLTFMeshInfo>> *>(VT_GLTFMESHINFOS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_ASSETNAME) &&
           verifier.VerifyTable(assetName()) &&
           VerifyField<flat::Matrix4>(verifier, VT_GLOBALINVERSETRANSFORM) &&
           VerifyOffset(verifier, VT_MATERIALS) &&
           verifier.VerifyVector(materials()) &&
           verifier.VerifyVectorOfTables(materials()) &&
           VerifyOffset(verifier, VT_MESHES) &&
           verifier.VerifyVector(meshes()) &&
           verifier.VerifyVectorOfTables(meshes()) &&
           VerifyOffset(verifier, VT_ANIMATIONDATA) &&
           verifier.VerifyTable(animationData()) &&
           VerifyOffset(verifier, VT_ANIMATIONS) &&
           verifier.VerifyVector(animations()) &&
           verifier.VerifyVectorOfTables(animations()) &&
           VerifyOffset(verifier, VT_GLTFMESHINFOS) &&
           verifier.VerifyVector(gltfMeshInfos()) &&
           verifier.VerifyVectorOfTables(gltfMeshInfos()) &&
           verifier.EndTable();
  }
};

struct RModelBuilder {
  typedef RModel Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_assetName(flatbuffers::Offset<flat::AssetName> assetName) {
    fbb_.AddOffset(RModel::VT_ASSETNAME, assetName);
  }
  void add_globalInverseTransform(const flat::Matrix4 *globalInverseTransform) {
    fbb_.AddStruct(RModel::VT_GLOBALINVERSETRANSFORM, globalInverseTransform);
  }
  void add_materials(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>>> materials) {
    fbb_.AddOffset(RModel::VT_MATERIALS, materials);
  }
  void add_meshes(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::RMesh>>> meshes) {
    fbb_.AddOffset(RModel::VT_MESHES, meshes);
  }
  void add_animationData(flatbuffers::Offset<flat::AnimationData> animationData) {
    fbb_.AddOffset(RModel::VT_ANIMATIONDATA, animationData);
  }
  void add_animations(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::RAnimation>>> animations) {
    fbb_.AddOffset(RModel::VT_ANIMATIONS, animations);
  }
  void add_gltfMeshInfos(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::GLTFMeshInfo>>> gltfMeshInfos) {
    fbb_.AddOffset(RModel::VT_GLTFMESHINFOS, gltfMeshInfos);
  }
  explicit RModelBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  RModelBuilder &operator=(const RModelBuilder &);
  flatbuffers::Offset<RModel> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<RModel>(end);
    return o;
  }
};

inline flatbuffers::Offset<RModel> CreateRModel(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flat::AssetName> assetName = 0,
    const flat::Matrix4 *globalInverseTransform = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>>> materials = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::RMesh>>> meshes = 0,
    flatbuffers::Offset<flat::AnimationData> animationData = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::RAnimation>>> animations = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::GLTFMeshInfo>>> gltfMeshInfos = 0) {
  RModelBuilder builder_(_fbb);
  builder_.add_gltfMeshInfos(gltfMeshInfos);
  builder_.add_animations(animations);
  builder_.add_animationData(animationData);
  builder_.add_meshes(meshes);
  builder_.add_materials(materials);
  builder_.add_globalInverseTransform(globalInverseTransform);
  builder_.add_assetName(assetName);
  return builder_.Finish();
}

inline flatbuffers::Offset<RModel> CreateRModelDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flat::AssetName> assetName = 0,
    const flat::Matrix4 *globalInverseTransform = 0,
    const std::vector<flatbuffers::Offset<flat::MaterialName>> *materials = nullptr,
    const std::vector<flatbuffers::Offset<flat::RMesh>> *meshes = nullptr,
    flatbuffers::Offset<flat::AnimationData> animationData = 0,
    const std::vector<flatbuffers::Offset<flat::RAnimation>> *animations = nullptr,
    const std::vector<flatbuffers::Offset<flat::GLTFMeshInfo>> *gltfMeshInfos = nullptr) {
  auto materials__ = materials ? _fbb.CreateVector<flatbuffers::Offset<flat::MaterialName>>(*materials) : 0;
  auto meshes__ = meshes ? _fbb.CreateVector<flatbuffers::Offset<flat::RMesh>>(*meshes) : 0;
  auto animations__ = animations ? _fbb.CreateVector<flatbuffers::Offset<flat::RAnimation>>(*animations) : 0;
  auto gltfMeshInfos__ = gltfMeshInfos ? _fbb.CreateVector<flatbuffers::Offset<flat::GLTFMeshInfo>>(*gltfMeshInfos) : 0;
  return flat::CreateRModel(
      _fbb,
      assetName,
      globalInverseTransform,
      materials__,
      meshes__,
      animationData,
      animations__,
      gltfMeshInfos__);
}

inline const flat::RModel *GetRModel(const void *buf) {
  return flatbuffers::GetRoot<flat::RModel>(buf);
}

inline const flat::RModel *GetSizePrefixedRModel(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::RModel>(buf);
}

inline RModel *GetMutableRModel(void *buf) {
  return flatbuffers::GetMutableRoot<RModel>(buf);
}

inline const char *RModelIdentifier() {
  return "rmdl";
}

inline bool RModelBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, RModelIdentifier());
}

inline bool VerifyRModelBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::RModel>(RModelIdentifier());
}

inline bool VerifySizePrefixedRModelBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::RModel>(RModelIdentifier());
}

inline const char *RModelExtension() {
  return "rmdl";
}

inline void FinishRModelBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::RModel> root) {
  fbb.Finish(root, RModelIdentifier());
}

inline void FinishSizePrefixedRModelBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::RModel> root) {
  fbb.FinishSizePrefixed(root, RModelIdentifier());
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_RMODEL_FLAT_H_
