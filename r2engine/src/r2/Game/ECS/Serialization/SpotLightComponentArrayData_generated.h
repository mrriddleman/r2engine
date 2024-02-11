// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SPOTLIGHTCOMPONENTARRAYDATA_FLAT_H_
#define FLATBUFFERS_GENERATED_SPOTLIGHTCOMPONENTARRAYDATA_FLAT_H_

#include "flatbuffers/flatbuffers.h"

#include "LightProperties_generated.h"

namespace flat {

struct SpotLightData;
struct SpotLightDataBuilder;

struct SpotLightComponentArrayData;
struct SpotLightComponentArrayDataBuilder;

struct SpotLightData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef SpotLightDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_LIGHTPROPERTIES = 4,
    VT_INNERCUTOFFANGLE = 6,
    VT_OUTERCUTOFFANGLE = 8
  };
  const flat::LightProperties *lightProperties() const {
    return GetPointer<const flat::LightProperties *>(VT_LIGHTPROPERTIES);
  }
  float innerCutoffAngle() const {
    return GetField<float>(VT_INNERCUTOFFANGLE, 0.0f);
  }
  float outerCutoffAngle() const {
    return GetField<float>(VT_OUTERCUTOFFANGLE, 0.0f);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_LIGHTPROPERTIES) &&
           verifier.VerifyTable(lightProperties()) &&
           VerifyField<float>(verifier, VT_INNERCUTOFFANGLE) &&
           VerifyField<float>(verifier, VT_OUTERCUTOFFANGLE) &&
           verifier.EndTable();
  }
};

struct SpotLightDataBuilder {
  typedef SpotLightData Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_lightProperties(flatbuffers::Offset<flat::LightProperties> lightProperties) {
    fbb_.AddOffset(SpotLightData::VT_LIGHTPROPERTIES, lightProperties);
  }
  void add_innerCutoffAngle(float innerCutoffAngle) {
    fbb_.AddElement<float>(SpotLightData::VT_INNERCUTOFFANGLE, innerCutoffAngle, 0.0f);
  }
  void add_outerCutoffAngle(float outerCutoffAngle) {
    fbb_.AddElement<float>(SpotLightData::VT_OUTERCUTOFFANGLE, outerCutoffAngle, 0.0f);
  }
  explicit SpotLightDataBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  SpotLightDataBuilder &operator=(const SpotLightDataBuilder &);
  flatbuffers::Offset<SpotLightData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<SpotLightData>(end);
    return o;
  }
};

inline flatbuffers::Offset<SpotLightData> CreateSpotLightData(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flat::LightProperties> lightProperties = 0,
    float innerCutoffAngle = 0.0f,
    float outerCutoffAngle = 0.0f) {
  SpotLightDataBuilder builder_(_fbb);
  builder_.add_outerCutoffAngle(outerCutoffAngle);
  builder_.add_innerCutoffAngle(innerCutoffAngle);
  builder_.add_lightProperties(lightProperties);
  return builder_.Finish();
}

struct SpotLightComponentArrayData FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef SpotLightComponentArrayDataBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_SPOTLIGHTCOMPONENTARRAY = 4
  };
  const flatbuffers::Vector<flatbuffers::Offset<flat::SpotLightData>> *spotLightComponentArray() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::SpotLightData>> *>(VT_SPOTLIGHTCOMPONENTARRAY);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_SPOTLIGHTCOMPONENTARRAY) &&
           verifier.VerifyVector(spotLightComponentArray()) &&
           verifier.VerifyVectorOfTables(spotLightComponentArray()) &&
           verifier.EndTable();
  }
};

struct SpotLightComponentArrayDataBuilder {
  typedef SpotLightComponentArrayData Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_spotLightComponentArray(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::SpotLightData>>> spotLightComponentArray) {
    fbb_.AddOffset(SpotLightComponentArrayData::VT_SPOTLIGHTCOMPONENTARRAY, spotLightComponentArray);
  }
  explicit SpotLightComponentArrayDataBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  SpotLightComponentArrayDataBuilder &operator=(const SpotLightComponentArrayDataBuilder &);
  flatbuffers::Offset<SpotLightComponentArrayData> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<SpotLightComponentArrayData>(end);
    return o;
  }
};

inline flatbuffers::Offset<SpotLightComponentArrayData> CreateSpotLightComponentArrayData(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::SpotLightData>>> spotLightComponentArray = 0) {
  SpotLightComponentArrayDataBuilder builder_(_fbb);
  builder_.add_spotLightComponentArray(spotLightComponentArray);
  return builder_.Finish();
}

inline flatbuffers::Offset<SpotLightComponentArrayData> CreateSpotLightComponentArrayDataDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flatbuffers::Offset<flat::SpotLightData>> *spotLightComponentArray = nullptr) {
  auto spotLightComponentArray__ = spotLightComponentArray ? _fbb.CreateVector<flatbuffers::Offset<flat::SpotLightData>>(*spotLightComponentArray) : 0;
  return flat::CreateSpotLightComponentArrayData(
      _fbb,
      spotLightComponentArray__);
}

inline const flat::SpotLightComponentArrayData *GetSpotLightComponentArrayData(const void *buf) {
  return flatbuffers::GetRoot<flat::SpotLightComponentArrayData>(buf);
}

inline const flat::SpotLightComponentArrayData *GetSizePrefixedSpotLightComponentArrayData(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::SpotLightComponentArrayData>(buf);
}

inline bool VerifySpotLightComponentArrayDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::SpotLightComponentArrayData>(nullptr);
}

inline bool VerifySizePrefixedSpotLightComponentArrayDataBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::SpotLightComponentArrayData>(nullptr);
}

inline void FinishSpotLightComponentArrayDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::SpotLightComponentArrayData> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedSpotLightComponentArrayDataBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::SpotLightComponentArrayData> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_SPOTLIGHTCOMPONENTARRAYDATA_FLAT_H_