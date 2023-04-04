// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_LEVELDATA_FLAT_H_
#define FLATBUFFERS_GENERATED_LEVELDATA_FLAT_H_

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/flexbuffers.h"

#include "ComponentArrayData_generated.h"
#include "EntityData_generated.h"

namespace flat {

struct PackReference;
struct PackReferenceBuilder;

struct LevelData;
struct LevelDataBuilder;

struct PackReference FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef PackReferenceBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_HASHNAME = 4,
    VT_RAWPATH = 6,
    VT_BINPATH = 8
  };
  uint64_t hashName() const {
    return GetField<uint64_t>(VT_HASHNAME, 0);
  }
  const flatbuffers::String *rawPath() const {
    return GetPointer<const flatbuffers::String *>(VT_RAWPATH);
  }
  const flatbuffers::String *binPath() const {
    return GetPointer<const flatbuffers::String *>(VT_BINPATH);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_HASHNAME) &&
           VerifyOffset(verifier, VT_RAWPATH) &&
           verifier.VerifyString(rawPath()) &&
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
  void add_rawPath(flatbuffers::Offset<flatbuffers::String> rawPath) {
    fbb_.AddOffset(PackReference::VT_RAWPATH, rawPath);
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
    flatbuffers::Offset<flatbuffers::String> rawPath = 0,
    flatbuffers::Offset<flatbuffers::String> binPath = 0) {
  PackReferenceBuilder builder_(_fbb);
  builder_.add_hashName(hashName);
  builder_.add_binPath(binPath);
  builder_.add_rawPath(rawPath);
  return builder_.Finish();
}

inline flatbuffers::Offset<PackReference> CreatePackReferenceDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t hashName = 0,
    const char *rawPath = nullptr,
    const char *binPath = nullptr) {
  auto rawPath__ = rawPath ? _fbb.CreateString(rawPath) : 0;
  auto binPath__ = binPath ? _fbb.CreateString(binPath) : 0;
  return flat::CreatePackReference(
      _fbb,
      hashName,
      rawPath__,
      binPath__);
}

struct LevelData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef LevelDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_VERSION = 4,
    VT_HASH = 6,
    VT_GROUPHASH = 8,
    VT_GROUPNAME = 10,
    VT_NAME = 12,
    VT_PATH = 14,
    VT_TOTALMEMORYNEEDED = 16,
    VT_MATERIALMEMORYNEEDED = 18,
    VT_STATICMODELMEMORYNEEDED = 20,
    VT_ANIMATEDMODELMEMORYNEEDED = 22,
    VT_TEXTUREMEMORYNEEDED = 24,
    VT_SOUNDMEMORYNEEDED = 26,
    VT_MUSICMEMORYNEEDED = 28,
    VT_MATERIALPACKREFERENCES = 30,
    VT_STATICMODELSREFERENCES = 32,
    VT_ANIMATEDMODELSREFERENCES = 34,
    VT_TEXTUREPACKSREFERENCES = 36,
    VT_SOUNDREFERENCES = 38,
    VT_MUSICREFERENCES = 40,
    VT_NUMENTITIES = 42,
    VT_ENTITIES = 44,
    VT_COMPONENTARRAYS = 46
  };
  uint32_t version() const {
    return GetField<uint32_t>(VT_VERSION, 0);
  }
  uint64_t hash() const {
    return GetField<uint64_t>(VT_HASH, 0);
  }
  uint64_t groupHash() const {
    return GetField<uint64_t>(VT_GROUPHASH, 0);
  }
  const flatbuffers::String *groupName() const {
    return GetPointer<const flatbuffers::String *>(VT_GROUPNAME);
  }
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  const flatbuffers::String *path() const {
    return GetPointer<const flatbuffers::String *>(VT_PATH);
  }
  uint32_t totalMemoryNeeded() const {
    return GetField<uint32_t>(VT_TOTALMEMORYNEEDED, 0);
  }
  uint32_t materialMemoryNeeded() const {
    return GetField<uint32_t>(VT_MATERIALMEMORYNEEDED, 0);
  }
  uint32_t staticModelMemoryNeeded() const {
    return GetField<uint32_t>(VT_STATICMODELMEMORYNEEDED, 0);
  }
  uint32_t animatedModelMemoryNeeded() const {
    return GetField<uint32_t>(VT_ANIMATEDMODELMEMORYNEEDED, 0);
  }
  uint32_t textureMemoryNeeded() const {
    return GetField<uint32_t>(VT_TEXTUREMEMORYNEEDED, 0);
  }
  uint32_t soundMemoryNeeded() const {
    return GetField<uint32_t>(VT_SOUNDMEMORYNEEDED, 0);
  }
  uint32_t musicMemoryNeeded() const {
    return GetField<uint32_t>(VT_MUSICMEMORYNEEDED, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *materialPackReferences() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *>(VT_MATERIALPACKREFERENCES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *staticModelsReferences() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *>(VT_STATICMODELSREFERENCES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *animatedModelsReferences() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *>(VT_ANIMATEDMODELSREFERENCES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *texturePacksReferences() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *>(VT_TEXTUREPACKSREFERENCES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *soundReferences() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *>(VT_SOUNDREFERENCES);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *musicReferences() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>> *>(VT_MUSICREFERENCES);
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
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_VERSION) &&
           VerifyField<uint64_t>(verifier, VT_HASH) &&
           VerifyField<uint64_t>(verifier, VT_GROUPHASH) &&
           VerifyOffset(verifier, VT_GROUPNAME) &&
           verifier.VerifyString(groupName()) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyOffset(verifier, VT_PATH) &&
           verifier.VerifyString(path()) &&
           VerifyField<uint32_t>(verifier, VT_TOTALMEMORYNEEDED) &&
           VerifyField<uint32_t>(verifier, VT_MATERIALMEMORYNEEDED) &&
           VerifyField<uint32_t>(verifier, VT_STATICMODELMEMORYNEEDED) &&
           VerifyField<uint32_t>(verifier, VT_ANIMATEDMODELMEMORYNEEDED) &&
           VerifyField<uint32_t>(verifier, VT_TEXTUREMEMORYNEEDED) &&
           VerifyField<uint32_t>(verifier, VT_SOUNDMEMORYNEEDED) &&
           VerifyField<uint32_t>(verifier, VT_MUSICMEMORYNEEDED) &&
           VerifyOffset(verifier, VT_MATERIALPACKREFERENCES) &&
           verifier.VerifyVector(materialPackReferences()) &&
           verifier.VerifyVectorOfTables(materialPackReferences()) &&
           VerifyOffset(verifier, VT_STATICMODELSREFERENCES) &&
           verifier.VerifyVector(staticModelsReferences()) &&
           verifier.VerifyVectorOfTables(staticModelsReferences()) &&
           VerifyOffset(verifier, VT_ANIMATEDMODELSREFERENCES) &&
           verifier.VerifyVector(animatedModelsReferences()) &&
           verifier.VerifyVectorOfTables(animatedModelsReferences()) &&
           VerifyOffset(verifier, VT_TEXTUREPACKSREFERENCES) &&
           verifier.VerifyVector(texturePacksReferences()) &&
           verifier.VerifyVectorOfTables(texturePacksReferences()) &&
           VerifyOffset(verifier, VT_SOUNDREFERENCES) &&
           verifier.VerifyVector(soundReferences()) &&
           verifier.VerifyVectorOfTables(soundReferences()) &&
           VerifyOffset(verifier, VT_MUSICREFERENCES) &&
           verifier.VerifyVector(musicReferences()) &&
           verifier.VerifyVectorOfTables(musicReferences()) &&
           VerifyField<uint32_t>(verifier, VT_NUMENTITIES) &&
           VerifyOffset(verifier, VT_ENTITIES) &&
           verifier.VerifyVector(entities()) &&
           verifier.VerifyVectorOfTables(entities()) &&
           VerifyOffset(verifier, VT_COMPONENTARRAYS) &&
           verifier.VerifyVector(componentArrays()) &&
           verifier.VerifyVectorOfTables(componentArrays()) &&
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
  void add_hash(uint64_t hash) {
    fbb_.AddElement<uint64_t>(LevelData::VT_HASH, hash, 0);
  }
  void add_groupHash(uint64_t groupHash) {
    fbb_.AddElement<uint64_t>(LevelData::VT_GROUPHASH, groupHash, 0);
  }
  void add_groupName(flatbuffers::Offset<flatbuffers::String> groupName) {
    fbb_.AddOffset(LevelData::VT_GROUPNAME, groupName);
  }
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(LevelData::VT_NAME, name);
  }
  void add_path(flatbuffers::Offset<flatbuffers::String> path) {
    fbb_.AddOffset(LevelData::VT_PATH, path);
  }
  void add_totalMemoryNeeded(uint32_t totalMemoryNeeded) {
    fbb_.AddElement<uint32_t>(LevelData::VT_TOTALMEMORYNEEDED, totalMemoryNeeded, 0);
  }
  void add_materialMemoryNeeded(uint32_t materialMemoryNeeded) {
    fbb_.AddElement<uint32_t>(LevelData::VT_MATERIALMEMORYNEEDED, materialMemoryNeeded, 0);
  }
  void add_staticModelMemoryNeeded(uint32_t staticModelMemoryNeeded) {
    fbb_.AddElement<uint32_t>(LevelData::VT_STATICMODELMEMORYNEEDED, staticModelMemoryNeeded, 0);
  }
  void add_animatedModelMemoryNeeded(uint32_t animatedModelMemoryNeeded) {
    fbb_.AddElement<uint32_t>(LevelData::VT_ANIMATEDMODELMEMORYNEEDED, animatedModelMemoryNeeded, 0);
  }
  void add_textureMemoryNeeded(uint32_t textureMemoryNeeded) {
    fbb_.AddElement<uint32_t>(LevelData::VT_TEXTUREMEMORYNEEDED, textureMemoryNeeded, 0);
  }
  void add_soundMemoryNeeded(uint32_t soundMemoryNeeded) {
    fbb_.AddElement<uint32_t>(LevelData::VT_SOUNDMEMORYNEEDED, soundMemoryNeeded, 0);
  }
  void add_musicMemoryNeeded(uint32_t musicMemoryNeeded) {
    fbb_.AddElement<uint32_t>(LevelData::VT_MUSICMEMORYNEEDED, musicMemoryNeeded, 0);
  }
  void add_materialPackReferences(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> materialPackReferences) {
    fbb_.AddOffset(LevelData::VT_MATERIALPACKREFERENCES, materialPackReferences);
  }
  void add_staticModelsReferences(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> staticModelsReferences) {
    fbb_.AddOffset(LevelData::VT_STATICMODELSREFERENCES, staticModelsReferences);
  }
  void add_animatedModelsReferences(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> animatedModelsReferences) {
    fbb_.AddOffset(LevelData::VT_ANIMATEDMODELSREFERENCES, animatedModelsReferences);
  }
  void add_texturePacksReferences(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> texturePacksReferences) {
    fbb_.AddOffset(LevelData::VT_TEXTUREPACKSREFERENCES, texturePacksReferences);
  }
  void add_soundReferences(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> soundReferences) {
    fbb_.AddOffset(LevelData::VT_SOUNDREFERENCES, soundReferences);
  }
  void add_musicReferences(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> musicReferences) {
    fbb_.AddOffset(LevelData::VT_MUSICREFERENCES, musicReferences);
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
    uint64_t hash = 0,
    uint64_t groupHash = 0,
    flatbuffers::Offset<flatbuffers::String> groupName = 0,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    flatbuffers::Offset<flatbuffers::String> path = 0,
    uint32_t totalMemoryNeeded = 0,
    uint32_t materialMemoryNeeded = 0,
    uint32_t staticModelMemoryNeeded = 0,
    uint32_t animatedModelMemoryNeeded = 0,
    uint32_t textureMemoryNeeded = 0,
    uint32_t soundMemoryNeeded = 0,
    uint32_t musicMemoryNeeded = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> materialPackReferences = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> staticModelsReferences = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> animatedModelsReferences = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> texturePacksReferences = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> soundReferences = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::PackReference>>> musicReferences = 0,
    uint32_t numEntities = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::EntityData>>> entities = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::ComponentArrayData>>> componentArrays = 0) {
  LevelDataBuilder builder_(_fbb);
  builder_.add_groupHash(groupHash);
  builder_.add_hash(hash);
  builder_.add_componentArrays(componentArrays);
  builder_.add_entities(entities);
  builder_.add_numEntities(numEntities);
  builder_.add_musicReferences(musicReferences);
  builder_.add_soundReferences(soundReferences);
  builder_.add_texturePacksReferences(texturePacksReferences);
  builder_.add_animatedModelsReferences(animatedModelsReferences);
  builder_.add_staticModelsReferences(staticModelsReferences);
  builder_.add_materialPackReferences(materialPackReferences);
  builder_.add_musicMemoryNeeded(musicMemoryNeeded);
  builder_.add_soundMemoryNeeded(soundMemoryNeeded);
  builder_.add_textureMemoryNeeded(textureMemoryNeeded);
  builder_.add_animatedModelMemoryNeeded(animatedModelMemoryNeeded);
  builder_.add_staticModelMemoryNeeded(staticModelMemoryNeeded);
  builder_.add_materialMemoryNeeded(materialMemoryNeeded);
  builder_.add_totalMemoryNeeded(totalMemoryNeeded);
  builder_.add_path(path);
  builder_.add_name(name);
  builder_.add_groupName(groupName);
  builder_.add_version(version);
  return builder_.Finish();
}

inline flatbuffers::Offset<LevelData> CreateLevelDataDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t version = 0,
    uint64_t hash = 0,
    uint64_t groupHash = 0,
    const char *groupName = nullptr,
    const char *name = nullptr,
    const char *path = nullptr,
    uint32_t totalMemoryNeeded = 0,
    uint32_t materialMemoryNeeded = 0,
    uint32_t staticModelMemoryNeeded = 0,
    uint32_t animatedModelMemoryNeeded = 0,
    uint32_t textureMemoryNeeded = 0,
    uint32_t soundMemoryNeeded = 0,
    uint32_t musicMemoryNeeded = 0,
    const std::vector<flatbuffers::Offset<flat::PackReference>> *materialPackReferences = nullptr,
    const std::vector<flatbuffers::Offset<flat::PackReference>> *staticModelsReferences = nullptr,
    const std::vector<flatbuffers::Offset<flat::PackReference>> *animatedModelsReferences = nullptr,
    const std::vector<flatbuffers::Offset<flat::PackReference>> *texturePacksReferences = nullptr,
    const std::vector<flatbuffers::Offset<flat::PackReference>> *soundReferences = nullptr,
    const std::vector<flatbuffers::Offset<flat::PackReference>> *musicReferences = nullptr,
    uint32_t numEntities = 0,
    const std::vector<flatbuffers::Offset<flat::EntityData>> *entities = nullptr,
    const std::vector<flatbuffers::Offset<flat::ComponentArrayData>> *componentArrays = nullptr) {
  auto groupName__ = groupName ? _fbb.CreateString(groupName) : 0;
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto path__ = path ? _fbb.CreateString(path) : 0;
  auto materialPackReferences__ = materialPackReferences ? _fbb.CreateVector<flatbuffers::Offset<flat::PackReference>>(*materialPackReferences) : 0;
  auto staticModelsReferences__ = staticModelsReferences ? _fbb.CreateVector<flatbuffers::Offset<flat::PackReference>>(*staticModelsReferences) : 0;
  auto animatedModelsReferences__ = animatedModelsReferences ? _fbb.CreateVector<flatbuffers::Offset<flat::PackReference>>(*animatedModelsReferences) : 0;
  auto texturePacksReferences__ = texturePacksReferences ? _fbb.CreateVector<flatbuffers::Offset<flat::PackReference>>(*texturePacksReferences) : 0;
  auto soundReferences__ = soundReferences ? _fbb.CreateVector<flatbuffers::Offset<flat::PackReference>>(*soundReferences) : 0;
  auto musicReferences__ = musicReferences ? _fbb.CreateVector<flatbuffers::Offset<flat::PackReference>>(*musicReferences) : 0;
  auto entities__ = entities ? _fbb.CreateVector<flatbuffers::Offset<flat::EntityData>>(*entities) : 0;
  auto componentArrays__ = componentArrays ? _fbb.CreateVector<flatbuffers::Offset<flat::ComponentArrayData>>(*componentArrays) : 0;
  return flat::CreateLevelData(
      _fbb,
      version,
      hash,
      groupHash,
      groupName__,
      name__,
      path__,
      totalMemoryNeeded,
      materialMemoryNeeded,
      staticModelMemoryNeeded,
      animatedModelMemoryNeeded,
      textureMemoryNeeded,
      soundMemoryNeeded,
      musicMemoryNeeded,
      materialPackReferences__,
      staticModelsReferences__,
      animatedModelsReferences__,
      texturePacksReferences__,
      soundReferences__,
      musicReferences__,
      numEntities,
      entities__,
      componentArrays__);
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
