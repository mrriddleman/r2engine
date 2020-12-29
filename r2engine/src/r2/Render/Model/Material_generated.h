// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_MATERIAL_FLAT_H_
#define FLATBUFFERS_GENERATED_MATERIAL_FLAT_H_

#include "flatbuffers/flatbuffers.h"

namespace flat {

struct Color;

struct Material;
struct MaterialBuilder;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Color FLATBUFFERS_FINAL_CLASS {
 private:
  float r_;
  float g_;
  float b_;
  float a_;

 public:
  Color() {
    memset(static_cast<void *>(this), 0, sizeof(Color));
  }
  Color(float _r, float _g, float _b, float _a)
      : r_(flatbuffers::EndianScalar(_r)),
        g_(flatbuffers::EndianScalar(_g)),
        b_(flatbuffers::EndianScalar(_b)),
        a_(flatbuffers::EndianScalar(_a)) {
  }
  float r() const {
    return flatbuffers::EndianScalar(r_);
  }
  float g() const {
    return flatbuffers::EndianScalar(g_);
  }
  float b() const {
    return flatbuffers::EndianScalar(b_);
  }
  float a() const {
    return flatbuffers::EndianScalar(a_);
  }
};
FLATBUFFERS_STRUCT_END(Color, 16);

struct Material FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_SHADER = 6,
    VT_TEXTUREPACKNAME = 8,
    VT_DIFFUSETEXTURE = 10,
    VT_SPECULARTEXTURE = 12,
    VT_NORMALMAPTEXTURE = 14,
    VT_EMISSIONTEXTURE = 16,
    VT_BASECOLOR = 18,
    VT_SPECULAR = 20,
    VT_ROUGHNESS = 22,
    VT_METALLIC = 24
  };
  uint64_t name() const {
    return GetField<uint64_t>(VT_NAME, 0);
  }
  uint64_t shader() const {
    return GetField<uint64_t>(VT_SHADER, 0);
  }
  uint64_t texturePackName() const {
    return GetField<uint64_t>(VT_TEXTUREPACKNAME, 0);
  }
  uint64_t diffuseTexture() const {
    return GetField<uint64_t>(VT_DIFFUSETEXTURE, 0);
  }
  uint64_t specularTexture() const {
    return GetField<uint64_t>(VT_SPECULARTEXTURE, 0);
  }
  uint64_t normalMapTexture() const {
    return GetField<uint64_t>(VT_NORMALMAPTEXTURE, 0);
  }
  uint64_t emissionTexture() const {
    return GetField<uint64_t>(VT_EMISSIONTEXTURE, 0);
  }
  const flat::Color *baseColor() const {
    return GetStruct<const flat::Color *>(VT_BASECOLOR);
  }
  float specular() const {
    return GetField<float>(VT_SPECULAR, 0.0f);
  }
  float roughness() const {
    return GetField<float>(VT_ROUGHNESS, 0.0f);
  }
  float metallic() const {
    return GetField<float>(VT_METALLIC, 0.0f);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_NAME) &&
           VerifyField<uint64_t>(verifier, VT_SHADER) &&
           VerifyField<uint64_t>(verifier, VT_TEXTUREPACKNAME) &&
           VerifyField<uint64_t>(verifier, VT_DIFFUSETEXTURE) &&
           VerifyField<uint64_t>(verifier, VT_SPECULARTEXTURE) &&
           VerifyField<uint64_t>(verifier, VT_NORMALMAPTEXTURE) &&
           VerifyField<uint64_t>(verifier, VT_EMISSIONTEXTURE) &&
           VerifyField<flat::Color>(verifier, VT_BASECOLOR) &&
           VerifyField<float>(verifier, VT_SPECULAR) &&
           VerifyField<float>(verifier, VT_ROUGHNESS) &&
           VerifyField<float>(verifier, VT_METALLIC) &&
           verifier.EndTable();
  }
};

struct MaterialBuilder {
  typedef Material Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(uint64_t name) {
    fbb_.AddElement<uint64_t>(Material::VT_NAME, name, 0);
  }
  void add_shader(uint64_t shader) {
    fbb_.AddElement<uint64_t>(Material::VT_SHADER, shader, 0);
  }
  void add_texturePackName(uint64_t texturePackName) {
    fbb_.AddElement<uint64_t>(Material::VT_TEXTUREPACKNAME, texturePackName, 0);
  }
  void add_diffuseTexture(uint64_t diffuseTexture) {
    fbb_.AddElement<uint64_t>(Material::VT_DIFFUSETEXTURE, diffuseTexture, 0);
  }
  void add_specularTexture(uint64_t specularTexture) {
    fbb_.AddElement<uint64_t>(Material::VT_SPECULARTEXTURE, specularTexture, 0);
  }
  void add_normalMapTexture(uint64_t normalMapTexture) {
    fbb_.AddElement<uint64_t>(Material::VT_NORMALMAPTEXTURE, normalMapTexture, 0);
  }
  void add_emissionTexture(uint64_t emissionTexture) {
    fbb_.AddElement<uint64_t>(Material::VT_EMISSIONTEXTURE, emissionTexture, 0);
  }
  void add_baseColor(const flat::Color *baseColor) {
    fbb_.AddStruct(Material::VT_BASECOLOR, baseColor);
  }
  void add_specular(float specular) {
    fbb_.AddElement<float>(Material::VT_SPECULAR, specular, 0.0f);
  }
  void add_roughness(float roughness) {
    fbb_.AddElement<float>(Material::VT_ROUGHNESS, roughness, 0.0f);
  }
  void add_metallic(float metallic) {
    fbb_.AddElement<float>(Material::VT_METALLIC, metallic, 0.0f);
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
    uint64_t name = 0,
    uint64_t shader = 0,
    uint64_t texturePackName = 0,
    uint64_t diffuseTexture = 0,
    uint64_t specularTexture = 0,
    uint64_t normalMapTexture = 0,
    uint64_t emissionTexture = 0,
    const flat::Color *baseColor = 0,
    float specular = 0.0f,
    float roughness = 0.0f,
    float metallic = 0.0f) {
  MaterialBuilder builder_(_fbb);
  builder_.add_emissionTexture(emissionTexture);
  builder_.add_normalMapTexture(normalMapTexture);
  builder_.add_specularTexture(specularTexture);
  builder_.add_diffuseTexture(diffuseTexture);
  builder_.add_texturePackName(texturePackName);
  builder_.add_shader(shader);
  builder_.add_name(name);
  builder_.add_metallic(metallic);
  builder_.add_roughness(roughness);
  builder_.add_specular(specular);
  builder_.add_baseColor(baseColor);
  return builder_.Finish();
}

inline const flat::Material *GetMaterial(const void *buf) {
  return flatbuffers::GetRoot<flat::Material>(buf);
}

inline const flat::Material *GetSizePrefixedMaterial(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::Material>(buf);
}

inline const char *MaterialIdentifier() {
  return "mmat";
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
  return "mmat";
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
