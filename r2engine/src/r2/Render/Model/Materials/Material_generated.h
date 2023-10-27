// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_MATERIAL_FLAT_H_
#define FLATBUFFERS_GENERATED_MATERIAL_FLAT_H_

#include "flatbuffers/flatbuffers.h"

#include "ShaderEffect_generated.h"
#include "ShaderEffectPasses_generated.h"
#include "ShaderParams_generated.h"

namespace flat {

struct Material;
struct MaterialBuilder;

enum eTransparencyType {
  eTransparencyType_OPAQUE = 0,
  eTransparencyType_TRANSPARENT = 1,
  eTransparencyType_MIN = eTransparencyType_OPAQUE,
  eTransparencyType_MAX = eTransparencyType_TRANSPARENT
};

inline const eTransparencyType (&EnumValueseTransparencyType())[2] {
  static const eTransparencyType values[] = {
    eTransparencyType_OPAQUE,
    eTransparencyType_TRANSPARENT
  };
  return values;
}

inline const char * const *EnumNameseTransparencyType() {
  static const char * const names[3] = {
    "OPAQUE",
    "TRANSPARENT",
    nullptr
  };
  return names;
}

inline const char *EnumNameeTransparencyType(eTransparencyType e) {
  if (flatbuffers::IsOutRange(e, eTransparencyType_OPAQUE, eTransparencyType_TRANSPARENT)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNameseTransparencyType()[index];
}

struct Material FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ASSETNAME = 4,
    VT_STRINGNAME = 6,
    VT_TRANSPARENCYTYPE = 8,
    VT_SHADEREFFECTPASSES = 10,
    VT_SHADERPARAMS = 12
  };
  uint64_t assetName() const {
    return GetField<uint64_t>(VT_ASSETNAME, 0);
  }
  const flatbuffers::String *stringName() const {
    return GetPointer<const flatbuffers::String *>(VT_STRINGNAME);
  }
  flat::eTransparencyType transparencyType() const {
    return static_cast<flat::eTransparencyType>(GetField<uint8_t>(VT_TRANSPARENCYTYPE, 0));
  }
  const flat::ShaderEffectPasses *shaderEffectPasses() const {
    return GetPointer<const flat::ShaderEffectPasses *>(VT_SHADEREFFECTPASSES);
  }
  const flat::ShaderParams *shaderParams() const {
    return GetPointer<const flat::ShaderParams *>(VT_SHADERPARAMS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_ASSETNAME) &&
           VerifyOffset(verifier, VT_STRINGNAME) &&
           verifier.VerifyString(stringName()) &&
           VerifyField<uint8_t>(verifier, VT_TRANSPARENCYTYPE) &&
           VerifyOffset(verifier, VT_SHADEREFFECTPASSES) &&
           verifier.VerifyTable(shaderEffectPasses()) &&
           VerifyOffset(verifier, VT_SHADERPARAMS) &&
           verifier.VerifyTable(shaderParams()) &&
           verifier.EndTable();
  }
};

struct MaterialBuilder {
  typedef Material Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_assetName(uint64_t assetName) {
    fbb_.AddElement<uint64_t>(Material::VT_ASSETNAME, assetName, 0);
  }
  void add_stringName(flatbuffers::Offset<flatbuffers::String> stringName) {
    fbb_.AddOffset(Material::VT_STRINGNAME, stringName);
  }
  void add_transparencyType(flat::eTransparencyType transparencyType) {
    fbb_.AddElement<uint8_t>(Material::VT_TRANSPARENCYTYPE, static_cast<uint8_t>(transparencyType), 0);
  }
  void add_shaderEffectPasses(flatbuffers::Offset<flat::ShaderEffectPasses> shaderEffectPasses) {
    fbb_.AddOffset(Material::VT_SHADEREFFECTPASSES, shaderEffectPasses);
  }
  void add_shaderParams(flatbuffers::Offset<flat::ShaderParams> shaderParams) {
    fbb_.AddOffset(Material::VT_SHADERPARAMS, shaderParams);
  }
  explicit MaterialBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialBuilder &operator=(const MaterialBuilder &);
  flatbuffers::Offset<Material> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Material>(end);
    return o;
  }
};

inline flatbuffers::Offset<Material> CreateMaterial(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t assetName = 0,
    flatbuffers::Offset<flatbuffers::String> stringName = 0,
    flat::eTransparencyType transparencyType = flat::eTransparencyType_OPAQUE,
    flatbuffers::Offset<flat::ShaderEffectPasses> shaderEffectPasses = 0,
    flatbuffers::Offset<flat::ShaderParams> shaderParams = 0) {
  MaterialBuilder builder_(_fbb);
  builder_.add_assetName(assetName);
  builder_.add_shaderParams(shaderParams);
  builder_.add_shaderEffectPasses(shaderEffectPasses);
  builder_.add_stringName(stringName);
  builder_.add_transparencyType(transparencyType);
  return builder_.Finish();
}

inline flatbuffers::Offset<Material> CreateMaterialDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t assetName = 0,
    const char *stringName = nullptr,
    flat::eTransparencyType transparencyType = flat::eTransparencyType_OPAQUE,
    flatbuffers::Offset<flat::ShaderEffectPasses> shaderEffectPasses = 0,
    flatbuffers::Offset<flat::ShaderParams> shaderParams = 0) {
  auto stringName__ = stringName ? _fbb.CreateString(stringName) : 0;
  return flat::CreateMaterial(
      _fbb,
      assetName,
      stringName__,
      transparencyType,
      shaderEffectPasses,
      shaderParams);
}

inline const flat::Material *GetMaterial(const void *buf) {
  return flatbuffers::GetRoot<flat::Material>(buf);
}

inline const flat::Material *GetSizePrefixedMaterial(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::Material>(buf);
}

inline const char *MaterialIdentifier() {
  return "mtrl";
}

inline bool MaterialBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, MaterialIdentifier());
}

inline bool VerifyMaterialBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::Material>(MaterialIdentifier());
}

inline bool VerifySizePrefixedMaterialBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::Material>(MaterialIdentifier());
}

inline const char *MaterialExtension() {
  return "mtrl";
}

inline void FinishMaterialBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::Material> root) {
  fbb.Finish(root, MaterialIdentifier());
}

inline void FinishSizePrefixedMaterialBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::Material> root) {
  fbb.FinishSizePrefixed(root, MaterialIdentifier());
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_MATERIAL_FLAT_H_
