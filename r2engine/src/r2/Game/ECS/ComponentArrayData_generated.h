// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_COMPONENTARRAYDATA_FLAT_H_
#define FLATBUFFERS_GENERATED_COMPONENTARRAYDATA_FLAT_H_

#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/flexbuffers.h"

namespace flat {

struct ComponentArrayData;
struct ComponentArrayDataBuilder;

struct ComponentArrayData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ComponentArrayDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_COMPONENTTYPE = 4,
    VT_ENTITYTOINDEXMAP = 6,
    VT_COMPONENTARRAY = 8
  };
  uint64_t componentType() const {
    return GetField<uint64_t>(VT_COMPONENTTYPE, 0);
  }
  const flatbuffers::Vector<uint32_t> *entityToIndexMap() const {
    return GetPointer<const flatbuffers::Vector<uint32_t> *>(VT_ENTITYTOINDEXMAP);
  }
  const flatbuffers::Vector<uint8_t> *componentArray() const {
    return GetPointer<const flatbuffers::Vector<uint8_t> *>(VT_COMPONENTARRAY);
  }
  flexbuffers::Reference componentArray_flexbuffer_root() const {
    return flexbuffers::GetRoot(componentArray()->Data(), componentArray()->size());
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_COMPONENTTYPE) &&
           VerifyOffset(verifier, VT_ENTITYTOINDEXMAP) &&
           verifier.VerifyVector(entityToIndexMap()) &&
           VerifyOffset(verifier, VT_COMPONENTARRAY) &&
           verifier.VerifyVector(componentArray()) &&
           verifier.EndTable();
  }
};

struct ComponentArrayDataBuilder {
  typedef ComponentArrayData Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_componentType(uint64_t componentType) {
    fbb_.AddElement<uint64_t>(ComponentArrayData::VT_COMPONENTTYPE, componentType, 0);
  }
  void add_entityToIndexMap(flatbuffers::Offset<flatbuffers::Vector<uint32_t>> entityToIndexMap) {
    fbb_.AddOffset(ComponentArrayData::VT_ENTITYTOINDEXMAP, entityToIndexMap);
  }
  void add_componentArray(flatbuffers::Offset<flatbuffers::Vector<uint8_t>> componentArray) {
    fbb_.AddOffset(ComponentArrayData::VT_COMPONENTARRAY, componentArray);
  }
  explicit ComponentArrayDataBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ComponentArrayDataBuilder &operator=(const ComponentArrayDataBuilder &);
  flatbuffers::Offset<ComponentArrayData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<ComponentArrayData>(end);
    return o;
  }
};

inline flatbuffers::Offset<ComponentArrayData> CreateComponentArrayData(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t componentType = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint32_t>> entityToIndexMap = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint8_t>> componentArray = 0) {
  ComponentArrayDataBuilder builder_(_fbb);
  builder_.add_componentType(componentType);
  builder_.add_componentArray(componentArray);
  builder_.add_entityToIndexMap(entityToIndexMap);
  return builder_.Finish();
}

inline flatbuffers::Offset<ComponentArrayData> CreateComponentArrayDataDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t componentType = 0,
    const std::vector<uint32_t> *entityToIndexMap = nullptr,
    const std::vector<uint8_t> *componentArray = nullptr) {
  auto entityToIndexMap__ = entityToIndexMap ? _fbb.CreateVector<uint32_t>(*entityToIndexMap) : 0;
  auto componentArray__ = componentArray ? _fbb.CreateVector<uint8_t>(*componentArray) : 0;
  return flat::CreateComponentArrayData(
      _fbb,
      componentType,
      entityToIndexMap__,
      componentArray__);
}

inline const flat::ComponentArrayData *GetComponentArrayData(const void *buf) {
  return flatbuffers::GetRoot<flat::ComponentArrayData>(buf);
}

inline const flat::ComponentArrayData *GetSizePrefixedComponentArrayData(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::ComponentArrayData>(buf);
}

inline bool VerifyComponentArrayDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::ComponentArrayData>(nullptr);
}

inline bool VerifySizePrefixedComponentArrayDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::ComponentArrayData>(nullptr);
}

inline void FinishComponentArrayDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::ComponentArrayData> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedComponentArrayDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::ComponentArrayData> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_COMPONENTARRAYDATA_FLAT_H_
