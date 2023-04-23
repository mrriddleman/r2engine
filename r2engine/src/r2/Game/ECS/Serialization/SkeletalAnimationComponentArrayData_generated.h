// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SKELETALANIMATIONCOMPONENTARRAYDATA_FLAT_H_
#define FLATBUFFERS_GENERATED_SKELETALANIMATIONCOMPONENTARRAYDATA_FLAT_H_

#include "flatbuffers/flatbuffers.h"

namespace flat {

struct SkeletalAnimationComponentData;
struct SkeletalAnimationComponentDataBuilder;

struct SkeletalAnimationComponentArrayData;
struct SkeletalAnimationComponentArrayDataBuilder;

struct SkeletalAnimationComponentData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef SkeletalAnimationComponentDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ANIMMODELASSETNAME = 4,
    VT_STARTINGANIMATIONASSETNAME = 6,
    VT_SHOULDUSESAMETRANSFORMFORALLINSTANCES = 8,
    VT_STARTTIME = 10,
    VT_SHOULDLOOP = 12
  };
  uint64_t animModelAssetName() const {
    return GetField<uint64_t>(VT_ANIMMODELASSETNAME, 0);
  }
  uint64_t startingAnimationAssetName() const {
    return GetField<uint64_t>(VT_STARTINGANIMATIONASSETNAME, 0);
  }
  bool shouldUseSameTransformForAllInstances() const {
    return GetField<uint8_t>(VT_SHOULDUSESAMETRANSFORMFORALLINSTANCES, 0) != 0;
  }
  uint32_t startTime() const {
    return GetField<uint32_t>(VT_STARTTIME, 0);
  }
  bool shouldLoop() const {
    return GetField<uint8_t>(VT_SHOULDLOOP, 0) != 0;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_ANIMMODELASSETNAME) &&
           VerifyField<uint64_t>(verifier, VT_STARTINGANIMATIONASSETNAME) &&
           VerifyField<uint8_t>(verifier, VT_SHOULDUSESAMETRANSFORMFORALLINSTANCES) &&
           VerifyField<uint32_t>(verifier, VT_STARTTIME) &&
           VerifyField<uint8_t>(verifier, VT_SHOULDLOOP) &&
           verifier.EndTable();
  }
};

struct SkeletalAnimationComponentDataBuilder {
  typedef SkeletalAnimationComponentData Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_animModelAssetName(uint64_t animModelAssetName) {
    fbb_.AddElement<uint64_t>(SkeletalAnimationComponentData::VT_ANIMMODELASSETNAME, animModelAssetName, 0);
  }
  void add_startingAnimationAssetName(uint64_t startingAnimationAssetName) {
    fbb_.AddElement<uint64_t>(SkeletalAnimationComponentData::VT_STARTINGANIMATIONASSETNAME, startingAnimationAssetName, 0);
  }
  void add_shouldUseSameTransformForAllInstances(bool shouldUseSameTransformForAllInstances) {
    fbb_.AddElement<uint8_t>(SkeletalAnimationComponentData::VT_SHOULDUSESAMETRANSFORMFORALLINSTANCES, static_cast<uint8_t>(shouldUseSameTransformForAllInstances), 0);
  }
  void add_startTime(uint32_t startTime) {
    fbb_.AddElement<uint32_t>(SkeletalAnimationComponentData::VT_STARTTIME, startTime, 0);
  }
  void add_shouldLoop(bool shouldLoop) {
    fbb_.AddElement<uint8_t>(SkeletalAnimationComponentData::VT_SHOULDLOOP, static_cast<uint8_t>(shouldLoop), 0);
  }
  explicit SkeletalAnimationComponentDataBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  SkeletalAnimationComponentDataBuilder &operator=(const SkeletalAnimationComponentDataBuilder &);
  flatbuffers::Offset<SkeletalAnimationComponentData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<SkeletalAnimationComponentData>(end);
    return o;
  }
};

inline flatbuffers::Offset<SkeletalAnimationComponentData> CreateSkeletalAnimationComponentData(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t animModelAssetName = 0,
    uint64_t startingAnimationAssetName = 0,
    bool shouldUseSameTransformForAllInstances = false,
    uint32_t startTime = 0,
    bool shouldLoop = false) {
  SkeletalAnimationComponentDataBuilder builder_(_fbb);
  builder_.add_startingAnimationAssetName(startingAnimationAssetName);
  builder_.add_animModelAssetName(animModelAssetName);
  builder_.add_startTime(startTime);
  builder_.add_shouldLoop(shouldLoop);
  builder_.add_shouldUseSameTransformForAllInstances(shouldUseSameTransformForAllInstances);
  return builder_.Finish();
}

struct SkeletalAnimationComponentArrayData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef SkeletalAnimationComponentArrayDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_SKELETALANIMATIONCOMPONENTARRAY = 4
  };
  const flatbuffers::Vector<flatbuffers::Offset<flat::SkeletalAnimationComponentData>> *skeletalAnimationComponentArray() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::SkeletalAnimationComponentData>> *>(VT_SKELETALANIMATIONCOMPONENTARRAY);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_SKELETALANIMATIONCOMPONENTARRAY) &&
           verifier.VerifyVector(skeletalAnimationComponentArray()) &&
           verifier.VerifyVectorOfTables(skeletalAnimationComponentArray()) &&
           verifier.EndTable();
  }
};

struct SkeletalAnimationComponentArrayDataBuilder {
  typedef SkeletalAnimationComponentArrayData Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_skeletalAnimationComponentArray(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::SkeletalAnimationComponentData>>> skeletalAnimationComponentArray) {
    fbb_.AddOffset(SkeletalAnimationComponentArrayData::VT_SKELETALANIMATIONCOMPONENTARRAY, skeletalAnimationComponentArray);
  }
  explicit SkeletalAnimationComponentArrayDataBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  SkeletalAnimationComponentArrayDataBuilder &operator=(const SkeletalAnimationComponentArrayDataBuilder &);
  flatbuffers::Offset<SkeletalAnimationComponentArrayData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<SkeletalAnimationComponentArrayData>(end);
    return o;
  }
};

inline flatbuffers::Offset<SkeletalAnimationComponentArrayData> CreateSkeletalAnimationComponentArrayData(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::SkeletalAnimationComponentData>>> skeletalAnimationComponentArray = 0) {
  SkeletalAnimationComponentArrayDataBuilder builder_(_fbb);
  builder_.add_skeletalAnimationComponentArray(skeletalAnimationComponentArray);
  return builder_.Finish();
}

inline flatbuffers::Offset<SkeletalAnimationComponentArrayData> CreateSkeletalAnimationComponentArrayDataDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flatbuffers::Offset<flat::SkeletalAnimationComponentData>> *skeletalAnimationComponentArray = nullptr) {
  auto skeletalAnimationComponentArray__ = skeletalAnimationComponentArray ? _fbb.CreateVector<flatbuffers::Offset<flat::SkeletalAnimationComponentData>>(*skeletalAnimationComponentArray) : 0;
  return flat::CreateSkeletalAnimationComponentArrayData(
      _fbb,
      skeletalAnimationComponentArray__);
}

inline const flat::SkeletalAnimationComponentArrayData *GetSkeletalAnimationComponentArrayData(const void *buf) {
  return flatbuffers::GetRoot<flat::SkeletalAnimationComponentArrayData>(buf);
}

inline const flat::SkeletalAnimationComponentArrayData *GetSizePrefixedSkeletalAnimationComponentArrayData(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::SkeletalAnimationComponentArrayData>(buf);
}

inline bool VerifySkeletalAnimationComponentArrayDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::SkeletalAnimationComponentArrayData>(nullptr);
}

inline bool VerifySizePrefixedSkeletalAnimationComponentArrayDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::SkeletalAnimationComponentArrayData>(nullptr);
}

inline void FinishSkeletalAnimationComponentArrayDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::SkeletalAnimationComponentArrayData> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedSkeletalAnimationComponentArrayDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::SkeletalAnimationComponentArrayData> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_SKELETALANIMATIONCOMPONENTARRAYDATA_FLAT_H_
