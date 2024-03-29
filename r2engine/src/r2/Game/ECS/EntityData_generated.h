// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_ENTITYDATA_FLAT_H_
#define FLATBUFFERS_GENERATED_ENTITYDATA_FLAT_H_

#include "flatbuffers/flatbuffers.h"

namespace flat {

struct EntityData;
struct EntityDataBuilder;

struct EntityData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef EntityDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ENTITYID = 4
  };
  uint32_t entityID() const {
    return GetField<uint32_t>(VT_ENTITYID, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_ENTITYID) &&
           verifier.EndTable();
  }
};

struct EntityDataBuilder {
  typedef EntityData Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_entityID(uint32_t entityID) {
    fbb_.AddElement<uint32_t>(EntityData::VT_ENTITYID, entityID, 0);
  }
  explicit EntityDataBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  EntityDataBuilder &operator=(const EntityDataBuilder &);
  flatbuffers::Offset<EntityData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<EntityData>(end);
    return o;
  }
};

inline flatbuffers::Offset<EntityData> CreateEntityData(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t entityID = 0) {
  EntityDataBuilder builder_(_fbb);
  builder_.add_entityID(entityID);
  return builder_.Finish();
}

inline const flat::EntityData *GetEntityData(const void *buf) {
  return flatbuffers::GetRoot<flat::EntityData>(buf);
}

inline const flat::EntityData *GetSizePrefixedEntityData(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::EntityData>(buf);
}

inline bool VerifyEntityDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::EntityData>(nullptr);
}

inline bool VerifySizePrefixedEntityDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::EntityData>(nullptr);
}

inline void FinishEntityDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::EntityData> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedEntityDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::EntityData> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_ENTITYDATA_FLAT_H_
