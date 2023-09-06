// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_LEVELDATA_FLAT_H_
#define FLATBUFFERS_GENERATED_LEVELDATA_FLAT_H_

#include "flatbuffers/flatbuffers.h"

#include "ComponentArrayData_generated.h"
#include "EntityData_generated.h"
#include "Model_generated.h"

namespace flat {

struct PackReference;
struct PackReferenceBuilder;

struct LevelData;
struct LevelDataBuilder;

struct PackReference FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef PackReferenceBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_HASHNAME = 4,
    VT_BINPATH = 6
  };
  uint64_t hashName() const {
    return GetField<uint64_t>(VT_HASHNAME, 0);
  }
  const flatbuffers::String *binPath() const {
    return GetPointer<const flatbuffers::String *>(VT_BINPATH);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_HASHNAME) &&
           VerifyOffset(verifier, VT_BINPATH) &&
           verifier.VerifyString(binPath()) &&
           verifier.EndTable();
  }
};

struct PackReferenceBuilder {
  typedef PackReference Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_hashName(uint64_t hashName) {
    fbb_.AddElement<uint64_t>(PackReference::VT_HASHNAME, hashName, 0);
  }
  void add_binPath(flatbuffers::Offset<flatbuffers::String> binPath) {
    fbb_.AddOffset(PackReference::VT_BINPATH, binPath);
  }
  explicit PackReferenceBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  PackReferenceBuilder &operator=(const PackReferenceBuilder &);
  flatbuffers::Offset<PackReference> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<PackReference>(end);
    return o;
  }
};

inline flatbuffers::Offset<PackReference> CreatePackReference(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t hashName = 0,
    flatbuffers::Offset<flatbuffers::String> binPath = 0) {
  PackReferenceBuilder builder_(_fbb);
  builder_.add_hashName(hashName);
  builder_.add_binPath(binPath);
  return builder_.Finish();
}

inline flatbuffers::Offset<PackReference> CreatePackReferenceDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t hashName = 0,
    const char *binPath = nullptr) {
  auto binPath__ = binPath ? _fbb.CreateString(binPath) : 0;
  return flat::CreatePackReference(
      _fbb,
      hashName,
      binPath__);
}

struct LevelData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef LevelDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_VERSION = 4,
    VT_LEVELASSETNAME = 6,
    VT_LEVELNAMESTRING = 8,
    VT_GROUPASSETNAME = 10,
    VT_GROUPNAMESTRING = 12,
    VT_PATH = 14,
    VT_NUMENTITIES = 16,
    VT_ENTITIES = 18,
    VT_COMPONENTARRAYS = 20,
    VT_MODELFILEPATHS = 22,
    VT_MATERIALNAMES = 24,
    VT_SOUNDPATHS = 26
  };
  uint32_t version() const {
    return GetField<uint32_t>(VT_VERSION, 0);
  }
  uint64_t levelAssetName() const {
    return GetField<uint64_t>(VT_LEVELASSETNAME, 0);
  }
  const flatbuffers::String *levelNameString() const {
    return GetPointer<const flatbuffers::String *>(VT_LEVELNAMESTRING);
  }
  uint64_t groupAssetName() const {
    return GetField<uint64_t>(VT_GROUPASSETNAME, 0);
  }
  const flatbuffers::String *groupNameString() const {
    return GetPointer<const flatbuffers::String *>(VT_GROUPNAMESTRING);
  }
  const flatbuffers::String *path() const {
    return GetPointer<const flatbuffers::String *>(VT_PATH);
  }
  uint32_t numEntities() const {
    return GetField<uint32_t>(VT_NUMENTITIES, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::EntityData>> *entities() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::EntityData>> *>(VT_ENTITIES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::ComponentArrayData>> *componentArrays() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::ComponentArrayData>> *>(VT_COMPONENTARRAYS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *modelFilePaths() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *>(VT_MODELFILEPATHS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>> *materialNames() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>> *>(VT_MATERIALNAMES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *soundPaths() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *>(VT_SOUNDPATHS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_VERSION) &&
           VerifyField<uint64_t>(verifier, VT_LEVELASSETNAME) &&
           VerifyOffset(verifier, VT_LEVELNAMESTRING) &&
           verifier.VerifyString(levelNameString()) &&
           VerifyField<uint64_t>(verifier, VT_GROUPASSETNAME) &&
           VerifyOffset(verifier, VT_GROUPNAMESTRING) &&
           verifier.VerifyString(groupNameString()) &&
           VerifyOffset(verifier, VT_PATH) &&
           verifier.VerifyString(path()) &&
           VerifyField<uint32_t>(verifier, VT_NUMENTITIES) &&
           VerifyOffset(verifier, VT_ENTITIES) &&
           verifier.VerifyVector(entities()) &&
           verifier.VerifyVectorOfTables(entities()) &&
           VerifyOffset(verifier, VT_COMPONENTARRAYS) &&
           verifier.VerifyVector(componentArrays()) &&
           verifier.VerifyVectorOfTables(componentArrays()) &&
           VerifyOffset(verifier, VT_MODELFILEPATHS) &&
           verifier.VerifyVector(modelFilePaths()) &&
           verifier.VerifyVectorOfTables(modelFilePaths()) &&
           VerifyOffset(verifier, VT_MATERIALNAMES) &&
           verifier.VerifyVector(materialNames()) &&
           verifier.VerifyVectorOfTables(materialNames()) &&
           VerifyOffset(verifier, VT_SOUNDPATHS) &&
           verifier.VerifyVector(soundPaths()) &&
           verifier.VerifyVectorOfTables(soundPaths()) &&
           verifier.EndTable();
  }
};

struct LevelDataBuilder {
  typedef LevelData Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_version(uint32_t version) {
    fbb_.AddElement<uint32_t>(LevelData::VT_VERSION, version, 0);
  }
  void add_levelAssetName(uint64_t levelAssetName) {
    fbb_.AddElement<uint64_t>(LevelData::VT_LEVELASSETNAME, levelAssetName, 0);
  }
  void add_levelNameString(flatbuffers::Offset<flatbuffers::String> levelNameString) {
    fbb_.AddOffset(LevelData::VT_LEVELNAMESTRING, levelNameString);
  }
  void add_groupAssetName(uint64_t groupAssetName) {
    fbb_.AddElement<uint64_t>(LevelData::VT_GROUPASSETNAME, groupAssetName, 0);
  }
  void add_groupNameString(flatbuffers::Offset<flatbuffers::String> groupNameString) {
    fbb_.AddOffset(LevelData::VT_GROUPNAMESTRING, groupNameString);
  }
  void add_path(flatbuffers::Offset<flatbuffers::String> path) {
    fbb_.AddOffset(LevelData::VT_PATH, path);
  }
  void add_numEntities(uint32_t numEntities) {
    fbb_.AddElement<uint32_t>(LevelData::VT_NUMENTITIES, numEntities, 0);
  }
  void add_entities(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::EntityData>>> entities) {
    fbb_.AddOffset(LevelData::VT_ENTITIES, entities);
  }
  void add_componentArrays(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::ComponentArrayData>>> componentArrays) {
    fbb_.AddOffset(LevelData::VT_COMPONENTARRAYS, componentArrays);
  }
  void add_modelFilePaths(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> modelFilePaths) {
    fbb_.AddOffset(LevelData::VT_MODELFILEPATHS, modelFilePaths);
  }
  void add_materialNames(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>>> materialNames) {
    fbb_.AddOffset(LevelData::VT_MATERIALNAMES, materialNames);
  }
  void add_soundPaths(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> soundPaths) {
    fbb_.AddOffset(LevelData::VT_SOUNDPATHS, soundPaths);
  }
  explicit LevelDataBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  LevelDataBuilder &operator=(const LevelDataBuilder &);
  flatbuffers::Offset<LevelData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<LevelData>(end);
    return o;
  }
};

inline flatbuffers::Offset<LevelData> CreateLevelData(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t version = 0,
    uint64_t levelAssetName = 0,
    flatbuffers::Offset<flatbuffers::String> levelNameString = 0,
    uint64_t groupAssetName = 0,
    flatbuffers::Offset<flatbuffers::String> groupNameString = 0,
    flatbuffers::Offset<flatbuffers::String> path = 0,
    uint32_t numEntities = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::EntityData>>> entities = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::ComponentArrayData>>> componentArrays = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> modelFilePaths = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialName>>> materialNames = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> soundPaths = 0) {
  LevelDataBuilder builder_(_fbb);
  builder_.add_groupAssetName(groupAssetName);
  builder_.add_levelAssetName(levelAssetName);
  builder_.add_soundPaths(soundPaths);
  builder_.add_materialNames(materialNames);
  builder_.add_modelFilePaths(modelFilePaths);
  builder_.add_componentArrays(componentArrays);
  builder_.add_entities(entities);
  builder_.add_numEntities(numEntities);
  builder_.add_path(path);
  builder_.add_groupNameString(groupNameString);
  builder_.add_levelNameString(levelNameString);
  builder_.add_version(version);
  return builder_.Finish();
}

inline flatbuffers::Offset<LevelData> CreateLevelDataDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t version = 0,
    uint64_t levelAssetName = 0,
    const char *levelNameString = nullptr,
    uint64_t groupAssetName = 0,
    const char *groupNameString = nullptr,
    const char *path = nullptr,
    uint32_t numEntities = 0,
    const std::vector<flatbuffers::Offset<flat::EntityData>> *entities = nullptr,
    const std::vector<flatbuffers::Offset<flat::ComponentArrayData>> *componentArrays = nullptr,
    const std::vector<flatbuffers::Offset<flat::PackReference>> *modelFilePaths = nullptr,
    const std::vector<flatbuffers::Offset<flat::MaterialName>> *materialNames = nullptr,
    const std::vector<flatbuffers::Offset<flat::PackReference>> *soundPaths = nullptr) {
  auto levelNameString__ = levelNameString ? _fbb.CreateString(levelNameString) : 0;
  auto groupNameString__ = groupNameString ? _fbb.CreateString(groupNameString) : 0;
  auto path__ = path ? _fbb.CreateString(path) : 0;
  auto entities__ = entities ? _fbb.CreateVector<flatbuffers::Offset<flat::EntityData>>(*entities) : 0;
  auto componentArrays__ = componentArrays ? _fbb.CreateVector<flatbuffers::Offset<flat::ComponentArrayData>>(*componentArrays) : 0;
  auto modelFilePaths__ = modelFilePaths ? _fbb.CreateVector<flatbuffers::Offset<flat::PackReference>>(*modelFilePaths) : 0;
  auto materialNames__ = materialNames ? _fbb.CreateVector<flatbuffers::Offset<flat::MaterialName>>(*materialNames) : 0;
  auto soundPaths__ = soundPaths ? _fbb.CreateVector<flatbuffers::Offset<flat::PackReference>>(*soundPaths) : 0;
  return flat::CreateLevelData(
      _fbb,
      version,
      levelAssetName,
      levelNameString__,
      groupAssetName,
      groupNameString__,
      path__,
      numEntities,
      entities__,
      componentArrays__,
      modelFilePaths__,
      materialNames__,
      soundPaths__);
}

inline const flat::LevelData *GetLevelData(const void *buf) {
  return flatbuffers::GetRoot<flat::LevelData>(buf);
}

inline const flat::LevelData *GetSizePrefixedLevelData(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::LevelData>(buf);
}

inline const char *LevelDataIdentifier() {
  return "rlvl";
}

inline bool LevelDataBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, LevelDataIdentifier());
}

inline bool VerifyLevelDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::LevelData>(LevelDataIdentifier());
}

inline bool VerifySizePrefixedLevelDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::LevelData>(LevelDataIdentifier());
}

inline const char *LevelDataExtension() {
  return "rlvl";
}

inline void FinishLevelDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::LevelData> root) {
  fbb.Finish(root, LevelDataIdentifier());
}

inline void FinishSizePrefixedLevelDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::LevelData> root) {
  fbb.FinishSizePrefixed(root, LevelDataIdentifier());
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_LEVELDATA_FLAT_H_
