// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SHADEREFFECTPASSES_FLAT_H_
#define FLATBUFFERS_GENERATED_SHADEREFFECTPASSES_FLAT_H_

#include "flatbuffers/flatbuffers.h"

#include "MaterialParams_generated.h"
#include "ShaderEffect_generated.h"
#include "ShaderParams_generated.h"

namespace flat {

struct ShaderEffectPasses;
struct ShaderEffectPassesBuilder;

enum eShaderEffectPass {
  eShaderEffectPass_FORWARD = 0,
  eShaderEffectPass_TRANSPARENT = 1,
  eShaderEffectPass_NUM_SHADER_EFFECT_PASSES = 2,
  eShaderEffectPass_MIN = eShaderEffectPass_FORWARD,
  eShaderEffectPass_MAX = eShaderEffectPass_NUM_SHADER_EFFECT_PASSES
};

inline const eShaderEffectPass (&EnumValueseShaderEffectPass())[3] {
  static const eShaderEffectPass values[] = {
    eShaderEffectPass_FORWARD,
    eShaderEffectPass_TRANSPARENT,
    eShaderEffectPass_NUM_SHADER_EFFECT_PASSES
  };
  return values;
}

inline const char * const *EnumNameseShaderEffectPass() {
  static const char * const names[4] = {
    "FORWARD",
    "TRANSPARENT",
    "NUM_SHADER_EFFECT_PASSES",
    nullptr
  };
  return names;
}

inline const char *EnumNameeShaderEffectPass(eShaderEffectPass e) {
  if (flatbuffers::IsOutRange(e, eShaderEffectPass_FORWARD, eShaderEffectPass_NUM_SHADER_EFFECT_PASSES)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNameseShaderEffectPass()[index];
}

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

struct ShaderEffectPasses FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ShaderEffectPassesBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_SHADERPASSES = 4,
    VT_TRANSPARENCY = 6,
    VT_DEFAULTSHADERPARAMS = 8
  };
  const flatbuffers::Vector<flatbuffers::Offset<flat::ShaderEffect>> *shaderPasses() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::ShaderEffect>> *>(VT_SHADERPASSES);
  }
  flat::eTransparencyType transparency() const {
    return static_cast<flat::eTransparencyType>(GetField<uint8_t>(VT_TRANSPARENCY, 0));
  }
  const flat::ShaderParams *defaultShaderParams() const {
    return GetPointer<const flat::ShaderParams *>(VT_DEFAULTSHADERPARAMS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_SHADERPASSES) &&
           verifier.VerifyVector(shaderPasses()) &&
           verifier.VerifyVectorOfTables(shaderPasses()) &&
           VerifyField<uint8_t>(verifier, VT_TRANSPARENCY) &&
           VerifyOffset(verifier, VT_DEFAULTSHADERPARAMS) &&
           verifier.VerifyTable(defaultShaderParams()) &&
           verifier.EndTable();
  }
};

struct ShaderEffectPassesBuilder {
  typedef ShaderEffectPasses Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_shaderPasses(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::ShaderEffect>>> shaderPasses) {
    fbb_.AddOffset(ShaderEffectPasses::VT_SHADERPASSES, shaderPasses);
  }
  void add_transparency(flat::eTransparencyType transparency) {
    fbb_.AddElement<uint8_t>(ShaderEffectPasses::VT_TRANSPARENCY, static_cast<uint8_t>(transparency), 0);
  }
  void add_defaultShaderParams(flatbuffers::Offset<flat::ShaderParams> defaultShaderParams) {
    fbb_.AddOffset(ShaderEffectPasses::VT_DEFAULTSHADERPARAMS, defaultShaderParams);
  }
  explicit ShaderEffectPassesBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ShaderEffectPassesBuilder &operator=(const ShaderEffectPassesBuilder &);
  flatbuffers::Offset<ShaderEffectPasses> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<ShaderEffectPasses>(end);
    return o;
  }
};

inline flatbuffers::Offset<ShaderEffectPasses> CreateShaderEffectPasses(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::ShaderEffect>>> shaderPasses = 0,
    flat::eTransparencyType transparency = flat::eTransparencyType_OPAQUE,
    flatbuffers::Offset<flat::ShaderParams> defaultShaderParams = 0) {
  ShaderEffectPassesBuilder builder_(_fbb);
  builder_.add_defaultShaderParams(defaultShaderParams);
  builder_.add_shaderPasses(shaderPasses);
  builder_.add_transparency(transparency);
  return builder_.Finish();
}

inline flatbuffers::Offset<ShaderEffectPasses> CreateShaderEffectPassesDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const std::vector<flatbuffers::Offset<flat::ShaderEffect>> *shaderPasses = nullptr,
    flat::eTransparencyType transparency = flat::eTransparencyType_OPAQUE,
    flatbuffers::Offset<flat::ShaderParams> defaultShaderParams = 0) {
  auto shaderPasses__ = shaderPasses ? _fbb.CreateVector<flatbuffers::Offset<flat::ShaderEffect>>(*shaderPasses) : 0;
  return flat::CreateShaderEffectPasses(
      _fbb,
      shaderPasses__,
      transparency,
      defaultShaderParams);
}

inline const flat::ShaderEffectPasses *GetShaderEffectPasses(const void *buf) {
  return flatbuffers::GetRoot<flat::ShaderEffectPasses>(buf);
}

inline const flat::ShaderEffectPasses *GetSizePrefixedShaderEffectPasses(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::ShaderEffectPasses>(buf);
}

inline const char *ShaderEffectPassesIdentifier() {
  return "sfxp";
}

inline bool ShaderEffectPassesBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, ShaderEffectPassesIdentifier());
}

inline bool VerifyShaderEffectPassesBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::ShaderEffectPasses>(ShaderEffectPassesIdentifier());
}

inline bool VerifySizePrefixedShaderEffectPassesBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::ShaderEffectPasses>(ShaderEffectPassesIdentifier());
}

inline const char *ShaderEffectPassesExtension() {
  return "sfxp";
}

inline void FinishShaderEffectPassesBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::ShaderEffectPasses> root) {
  fbb.Finish(root, ShaderEffectPassesIdentifier());
}

inline void FinishSizePrefixedShaderEffectPassesBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::ShaderEffectPasses> root) {
  fbb.FinishSizePrefixed(root, ShaderEffectPassesIdentifier());
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_SHADEREFFECTPASSES_FLAT_H_
