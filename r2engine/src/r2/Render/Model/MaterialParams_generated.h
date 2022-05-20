// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_MATERIALPARAMS_FLAT_H_
#define FLATBUFFERS_GENERATED_MATERIALPARAMS_FLAT_H_

#include "flatbuffers/flatbuffers.h"

namespace flat {

struct Colour;

struct MaterialFloatParam;
struct MaterialFloatParamBuilder;

struct MaterialColorParam;
struct MaterialColorParamBuilder;

struct MaterialBoolParam;
struct MaterialBoolParamBuilder;

struct MaterialULongParam;
struct MaterialULongParamBuilder;

struct MaterialStringParam;
struct MaterialStringParamBuilder;

struct MaterialTextureParam;
struct MaterialTextureParamBuilder;

struct MaterialParams;
struct MaterialParamsBuilder;

enum MaterialPropertyType {
  MaterialPropertyType_ALBEDO = 0,
  MaterialPropertyType_NORMAL = 1,
  MaterialPropertyType_EMISSION = 2,
  MaterialPropertyType_METALLIC = 3,
  MaterialPropertyType_ROUGHNESS = 4,
  MaterialPropertyType_AMBIENT_OCCLUSION = 5,
  MaterialPropertyType_HEIGHT = 6,
  MaterialPropertyType_ANISOTROPY = 7,
  MaterialPropertyType_ANISOTROPY_DIRECTION = 8,
  MaterialPropertyType_DETAIL = 9,
  MaterialPropertyType_HEIGHT_SCALE = 10,
  MaterialPropertyType_REFLECTANCE = 11,
  MaterialPropertyType_CLEAR_COAT = 12,
  MaterialPropertyType_CLEAR_COAT_ROUGHNESS = 13,
  MaterialPropertyType_CLEAR_COAT_NORMAL = 14,
  MaterialPropertyType_BENT_NORMAL = 15,
  MaterialPropertyType_SHEEN_COLOR = 16,
  MaterialPropertyType_SHEEN_ROUGHNESS = 17,
  MaterialPropertyType_DOUBLE_SIDED = 18,
  MaterialPropertyType_SHADER = 19,
  MaterialPropertyType_MIN = MaterialPropertyType_ALBEDO,
  MaterialPropertyType_MAX = MaterialPropertyType_SHADER
};

inline const MaterialPropertyType (&EnumValuesMaterialPropertyType())[20] {
  static const MaterialPropertyType values[] = {
    MaterialPropertyType_ALBEDO,
    MaterialPropertyType_NORMAL,
    MaterialPropertyType_EMISSION,
    MaterialPropertyType_METALLIC,
    MaterialPropertyType_ROUGHNESS,
    MaterialPropertyType_AMBIENT_OCCLUSION,
    MaterialPropertyType_HEIGHT,
    MaterialPropertyType_ANISOTROPY,
    MaterialPropertyType_ANISOTROPY_DIRECTION,
    MaterialPropertyType_DETAIL,
    MaterialPropertyType_HEIGHT_SCALE,
    MaterialPropertyType_REFLECTANCE,
    MaterialPropertyType_CLEAR_COAT,
    MaterialPropertyType_CLEAR_COAT_ROUGHNESS,
    MaterialPropertyType_CLEAR_COAT_NORMAL,
    MaterialPropertyType_BENT_NORMAL,
    MaterialPropertyType_SHEEN_COLOR,
    MaterialPropertyType_SHEEN_ROUGHNESS,
    MaterialPropertyType_DOUBLE_SIDED,
    MaterialPropertyType_SHADER
  };
  return values;
}

inline const char * const *EnumNamesMaterialPropertyType() {
  static const char * const names[21] = {
    "ALBEDO",
    "NORMAL",
    "EMISSION",
    "METALLIC",
    "ROUGHNESS",
    "AMBIENT_OCCLUSION",
    "HEIGHT",
    "ANISOTROPY",
    "ANISOTROPY_DIRECTION",
    "DETAIL",
    "HEIGHT_SCALE",
    "REFLECTANCE",
    "CLEAR_COAT",
    "CLEAR_COAT_ROUGHNESS",
    "CLEAR_COAT_NORMAL",
    "BENT_NORMAL",
    "SHEEN_COLOR",
    "SHEEN_ROUGHNESS",
    "DOUBLE_SIDED",
    "SHADER",
    nullptr
  };
  return names;
}

inline const char *EnumNameMaterialPropertyType(MaterialPropertyType e) {
  if (flatbuffers::IsOutRange(e, MaterialPropertyType_ALBEDO, MaterialPropertyType_SHADER)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesMaterialPropertyType()[index];
}

enum MaterialPropertyPackingType {
  MaterialPropertyPackingType_R = 0,
  MaterialPropertyPackingType_G = 1,
  MaterialPropertyPackingType_B = 2,
  MaterialPropertyPackingType_A = 3,
  MaterialPropertyPackingType_RGB = 4,
  MaterialPropertyPackingType_RGBA = 5,
  MaterialPropertyPackingType_MIN = MaterialPropertyPackingType_R,
  MaterialPropertyPackingType_MAX = MaterialPropertyPackingType_RGBA
};

inline const MaterialPropertyPackingType (&EnumValuesMaterialPropertyPackingType())[6] {
  static const MaterialPropertyPackingType values[] = {
    MaterialPropertyPackingType_R,
    MaterialPropertyPackingType_G,
    MaterialPropertyPackingType_B,
    MaterialPropertyPackingType_A,
    MaterialPropertyPackingType_RGB,
    MaterialPropertyPackingType_RGBA
  };
  return values;
}

inline const char * const *EnumNamesMaterialPropertyPackingType() {
  static const char * const names[7] = {
    "R",
    "G",
    "B",
    "A",
    "RGB",
    "RGBA",
    nullptr
  };
  return names;
}

inline const char *EnumNameMaterialPropertyPackingType(MaterialPropertyPackingType e) {
  if (flatbuffers::IsOutRange(e, MaterialPropertyPackingType_R, MaterialPropertyPackingType_RGBA)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesMaterialPropertyPackingType()[index];
}

enum MinTextureFilter {
  MinTextureFilter_LINEAR = 0,
  MinTextureFilter_NEAREST = 1,
  MinTextureFilter_NEAREST_MIPMAP_NEAREST = 2,
  MinTextureFilter_LINEAR_MIPMAP_NEAREST = 3,
  MinTextureFilter_NEAREST_MIPMAP_LINEAR = 4,
  MinTextureFilter_LINEAR_MIPMAP_LINEAR = 5,
  MinTextureFilter_MIN = MinTextureFilter_LINEAR,
  MinTextureFilter_MAX = MinTextureFilter_LINEAR_MIPMAP_LINEAR
};

inline const MinTextureFilter (&EnumValuesMinTextureFilter())[6] {
  static const MinTextureFilter values[] = {
    MinTextureFilter_LINEAR,
    MinTextureFilter_NEAREST,
    MinTextureFilter_NEAREST_MIPMAP_NEAREST,
    MinTextureFilter_LINEAR_MIPMAP_NEAREST,
    MinTextureFilter_NEAREST_MIPMAP_LINEAR,
    MinTextureFilter_LINEAR_MIPMAP_LINEAR
  };
  return values;
}

inline const char * const *EnumNamesMinTextureFilter() {
  static const char * const names[7] = {
    "LINEAR",
    "NEAREST",
    "NEAREST_MIPMAP_NEAREST",
    "LINEAR_MIPMAP_NEAREST",
    "NEAREST_MIPMAP_LINEAR",
    "LINEAR_MIPMAP_LINEAR",
    nullptr
  };
  return names;
}

inline const char *EnumNameMinTextureFilter(MinTextureFilter e) {
  if (flatbuffers::IsOutRange(e, MinTextureFilter_LINEAR, MinTextureFilter_LINEAR_MIPMAP_LINEAR)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesMinTextureFilter()[index];
}

enum MagTextureFilter {
  MagTextureFilter_LINEAR = 0,
  MagTextureFilter_NEAREST = 1,
  MagTextureFilter_MIN = MagTextureFilter_LINEAR,
  MagTextureFilter_MAX = MagTextureFilter_NEAREST
};

inline const MagTextureFilter (&EnumValuesMagTextureFilter())[2] {
  static const MagTextureFilter values[] = {
    MagTextureFilter_LINEAR,
    MagTextureFilter_NEAREST
  };
  return values;
}

inline const char * const *EnumNamesMagTextureFilter() {
  static const char * const names[3] = {
    "LINEAR",
    "NEAREST",
    nullptr
  };
  return names;
}

inline const char *EnumNameMagTextureFilter(MagTextureFilter e) {
  if (flatbuffers::IsOutRange(e, MagTextureFilter_LINEAR, MagTextureFilter_NEAREST)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesMagTextureFilter()[index];
}

enum TextureWrapMode {
  TextureWrapMode_REPEAT = 0,
  TextureWrapMode_CLAMP_TO_EDGE = 1,
  TextureWrapMode_CLAMP_TO_BORDER = 2,
  TextureWrapMode_MIRRORED_REPEAT = 3,
  TextureWrapMode_MIN = TextureWrapMode_REPEAT,
  TextureWrapMode_MAX = TextureWrapMode_MIRRORED_REPEAT
};

inline const TextureWrapMode (&EnumValuesTextureWrapMode())[4] {
  static const TextureWrapMode values[] = {
    TextureWrapMode_REPEAT,
    TextureWrapMode_CLAMP_TO_EDGE,
    TextureWrapMode_CLAMP_TO_BORDER,
    TextureWrapMode_MIRRORED_REPEAT
  };
  return values;
}

inline const char * const *EnumNamesTextureWrapMode() {
  static const char * const names[5] = {
    "REPEAT",
    "CLAMP_TO_EDGE",
    "CLAMP_TO_BORDER",
    "MIRRORED_REPEAT",
    nullptr
  };
  return names;
}

inline const char *EnumNameTextureWrapMode(TextureWrapMode e) {
  if (flatbuffers::IsOutRange(e, TextureWrapMode_REPEAT, TextureWrapMode_MIRRORED_REPEAT)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesTextureWrapMode()[index];
}

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Colour FLATBUFFERS_FINAL_CLASS {
 private:
  float r_;
  float g_;
  float b_;
  float a_;

 public:
  Colour() {
    memset(static_cast<void *>(this), 0, sizeof(Colour));
  }
  Colour(float _r, float _g, float _b, float _a)
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
FLATBUFFERS_STRUCT_END(Colour, 16);

struct MaterialFloatParam FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialFloatParamBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PROPERTYTYPE = 4,
    VT_VALUE = 6
  };
  flat::MaterialPropertyType propertyType() const {
    return static_cast<flat::MaterialPropertyType>(GetField<uint16_t>(VT_PROPERTYTYPE, 0));
  }
  float value() const {
    return GetField<float>(VT_VALUE, 0.0f);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint16_t>(verifier, VT_PROPERTYTYPE) &&
           VerifyField<float>(verifier, VT_VALUE) &&
           verifier.EndTable();
  }
};

struct MaterialFloatParamBuilder {
  typedef MaterialFloatParam Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_propertyType(flat::MaterialPropertyType propertyType) {
    fbb_.AddElement<uint16_t>(MaterialFloatParam::VT_PROPERTYTYPE, static_cast<uint16_t>(propertyType), 0);
  }
  void add_value(float value) {
    fbb_.AddElement<float>(MaterialFloatParam::VT_VALUE, value, 0.0f);
  }
  explicit MaterialFloatParamBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialFloatParamBuilder &operator=(const MaterialFloatParamBuilder &);
  flatbuffers::Offset<MaterialFloatParam> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MaterialFloatParam>(end);
    return o;
  }
};

inline flatbuffers::Offset<MaterialFloatParam> CreateMaterialFloatParam(
    flatbuffers::FlatBufferBuilder &_fbb,
    flat::MaterialPropertyType propertyType = flat::MaterialPropertyType_ALBEDO,
    float value = 0.0f) {
  MaterialFloatParamBuilder builder_(_fbb);
  builder_.add_value(value);
  builder_.add_propertyType(propertyType);
  return builder_.Finish();
}

struct MaterialColorParam FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialColorParamBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PROPERTYTYPE = 4,
    VT_VALUE = 6
  };
  flat::MaterialPropertyType propertyType() const {
    return static_cast<flat::MaterialPropertyType>(GetField<uint16_t>(VT_PROPERTYTYPE, 0));
  }
  const flat::Colour *value() const {
    return GetStruct<const flat::Colour *>(VT_VALUE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint16_t>(verifier, VT_PROPERTYTYPE) &&
           VerifyField<flat::Colour>(verifier, VT_VALUE) &&
           verifier.EndTable();
  }
};

struct MaterialColorParamBuilder {
  typedef MaterialColorParam Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_propertyType(flat::MaterialPropertyType propertyType) {
    fbb_.AddElement<uint16_t>(MaterialColorParam::VT_PROPERTYTYPE, static_cast<uint16_t>(propertyType), 0);
  }
  void add_value(const flat::Colour *value) {
    fbb_.AddStruct(MaterialColorParam::VT_VALUE, value);
  }
  explicit MaterialColorParamBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialColorParamBuilder &operator=(const MaterialColorParamBuilder &);
  flatbuffers::Offset<MaterialColorParam> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MaterialColorParam>(end);
    return o;
  }
};

inline flatbuffers::Offset<MaterialColorParam> CreateMaterialColorParam(
    flatbuffers::FlatBufferBuilder &_fbb,
    flat::MaterialPropertyType propertyType = flat::MaterialPropertyType_ALBEDO,
    const flat::Colour *value = 0) {
  MaterialColorParamBuilder builder_(_fbb);
  builder_.add_value(value);
  builder_.add_propertyType(propertyType);
  return builder_.Finish();
}

struct MaterialBoolParam FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialBoolParamBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PROPERTYTYPE = 4,
    VT_VALUE = 6
  };
  flat::MaterialPropertyType propertyType() const {
    return static_cast<flat::MaterialPropertyType>(GetField<uint16_t>(VT_PROPERTYTYPE, 0));
  }
  bool value() const {
    return GetField<uint8_t>(VT_VALUE, 0) != 0;
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint16_t>(verifier, VT_PROPERTYTYPE) &&
           VerifyField<uint8_t>(verifier, VT_VALUE) &&
           verifier.EndTable();
  }
};

struct MaterialBoolParamBuilder {
  typedef MaterialBoolParam Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_propertyType(flat::MaterialPropertyType propertyType) {
    fbb_.AddElement<uint16_t>(MaterialBoolParam::VT_PROPERTYTYPE, static_cast<uint16_t>(propertyType), 0);
  }
  void add_value(bool value) {
    fbb_.AddElement<uint8_t>(MaterialBoolParam::VT_VALUE, static_cast<uint8_t>(value), 0);
  }
  explicit MaterialBoolParamBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialBoolParamBuilder &operator=(const MaterialBoolParamBuilder &);
  flatbuffers::Offset<MaterialBoolParam> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MaterialBoolParam>(end);
    return o;
  }
};

inline flatbuffers::Offset<MaterialBoolParam> CreateMaterialBoolParam(
    flatbuffers::FlatBufferBuilder &_fbb,
    flat::MaterialPropertyType propertyType = flat::MaterialPropertyType_ALBEDO,
    bool value = false) {
  MaterialBoolParamBuilder builder_(_fbb);
  builder_.add_propertyType(propertyType);
  builder_.add_value(value);
  return builder_.Finish();
}

struct MaterialULongParam FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialULongParamBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PROPERTYTYPE = 4,
    VT_VALUE = 6
  };
  flat::MaterialPropertyType propertyType() const {
    return static_cast<flat::MaterialPropertyType>(GetField<uint16_t>(VT_PROPERTYTYPE, 0));
  }
  uint64_t value() const {
    return GetField<uint64_t>(VT_VALUE, 0);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint16_t>(verifier, VT_PROPERTYTYPE) &&
           VerifyField<uint64_t>(verifier, VT_VALUE) &&
           verifier.EndTable();
  }
};

struct MaterialULongParamBuilder {
  typedef MaterialULongParam Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_propertyType(flat::MaterialPropertyType propertyType) {
    fbb_.AddElement<uint16_t>(MaterialULongParam::VT_PROPERTYTYPE, static_cast<uint16_t>(propertyType), 0);
  }
  void add_value(uint64_t value) {
    fbb_.AddElement<uint64_t>(MaterialULongParam::VT_VALUE, value, 0);
  }
  explicit MaterialULongParamBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialULongParamBuilder &operator=(const MaterialULongParamBuilder &);
  flatbuffers::Offset<MaterialULongParam> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MaterialULongParam>(end);
    return o;
  }
};

inline flatbuffers::Offset<MaterialULongParam> CreateMaterialULongParam(
    flatbuffers::FlatBufferBuilder &_fbb,
    flat::MaterialPropertyType propertyType = flat::MaterialPropertyType_ALBEDO,
    uint64_t value = 0) {
  MaterialULongParamBuilder builder_(_fbb);
  builder_.add_value(value);
  builder_.add_propertyType(propertyType);
  return builder_.Finish();
}

struct MaterialStringParam FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialStringParamBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PROPERTYTYPE = 4,
    VT_VALUE = 6
  };
  flat::MaterialPropertyType propertyType() const {
    return static_cast<flat::MaterialPropertyType>(GetField<uint16_t>(VT_PROPERTYTYPE, 0));
  }
  const flatbuffers::String *value() const {
    return GetPointer<const flatbuffers::String *>(VT_VALUE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint16_t>(verifier, VT_PROPERTYTYPE) &&
           VerifyOffset(verifier, VT_VALUE) &&
           verifier.VerifyString(value()) &&
           verifier.EndTable();
  }
};

struct MaterialStringParamBuilder {
  typedef MaterialStringParam Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_propertyType(flat::MaterialPropertyType propertyType) {
    fbb_.AddElement<uint16_t>(MaterialStringParam::VT_PROPERTYTYPE, static_cast<uint16_t>(propertyType), 0);
  }
  void add_value(flatbuffers::Offset<flatbuffers::String> value) {
    fbb_.AddOffset(MaterialStringParam::VT_VALUE, value);
  }
  explicit MaterialStringParamBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialStringParamBuilder &operator=(const MaterialStringParamBuilder &);
  flatbuffers::Offset<MaterialStringParam> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MaterialStringParam>(end);
    return o;
  }
};

inline flatbuffers::Offset<MaterialStringParam> CreateMaterialStringParam(
    flatbuffers::FlatBufferBuilder &_fbb,
    flat::MaterialPropertyType propertyType = flat::MaterialPropertyType_ALBEDO,
    flatbuffers::Offset<flatbuffers::String> value = 0) {
  MaterialStringParamBuilder builder_(_fbb);
  builder_.add_value(value);
  builder_.add_propertyType(propertyType);
  return builder_.Finish();
}

inline flatbuffers::Offset<MaterialStringParam> CreateMaterialStringParamDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    flat::MaterialPropertyType propertyType = flat::MaterialPropertyType_ALBEDO,
    const char *value = nullptr) {
  auto value__ = value ? _fbb.CreateString(value) : 0;
  return flat::CreateMaterialStringParam(
      _fbb,
      propertyType,
      value__);
}

struct MaterialTextureParam FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialTextureParamBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PROPERTYTYPE = 4,
    VT_VALUE = 6,
    VT_PACKINGTYPE = 8,
    VT_TEXTUREPACKNAME = 10,
    VT_TEXTUREPACKNAMESTR = 12,
    VT_MINFILTER = 14,
    VT_MAGFILTER = 16,
    VT_ANISOTROPICFILTERING = 18,
    VT_WRAPS = 20,
    VT_WRAPT = 22,
    VT_WRAPR = 24
  };
  flat::MaterialPropertyType propertyType() const {
    return static_cast<flat::MaterialPropertyType>(GetField<uint16_t>(VT_PROPERTYTYPE, 0));
  }
  uint64_t value() const {
    return GetField<uint64_t>(VT_VALUE, 0);
  }
  flat::MaterialPropertyPackingType packingType() const {
    return static_cast<flat::MaterialPropertyPackingType>(GetField<int8_t>(VT_PACKINGTYPE, 0));
  }
  uint64_t texturePackName() const {
    return GetField<uint64_t>(VT_TEXTUREPACKNAME, 0);
  }
  const flatbuffers::String *texturePackNameStr() const {
    return GetPointer<const flatbuffers::String *>(VT_TEXTUREPACKNAMESTR);
  }
  flat::MinTextureFilter minFilter() const {
    return static_cast<flat::MinTextureFilter>(GetField<uint8_t>(VT_MINFILTER, 0));
  }
  flat::MagTextureFilter magFilter() const {
    return static_cast<flat::MagTextureFilter>(GetField<uint8_t>(VT_MAGFILTER, 0));
  }
  float anisotropicFiltering() const {
    return GetField<float>(VT_ANISOTROPICFILTERING, 0.0f);
  }
  flat::TextureWrapMode wrapS() const {
    return static_cast<flat::TextureWrapMode>(GetField<uint8_t>(VT_WRAPS, 0));
  }
  flat::TextureWrapMode wrapT() const {
    return static_cast<flat::TextureWrapMode>(GetField<uint8_t>(VT_WRAPT, 0));
  }
  flat::TextureWrapMode wrapR() const {
    return static_cast<flat::TextureWrapMode>(GetField<uint8_t>(VT_WRAPR, 0));
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint16_t>(verifier, VT_PROPERTYTYPE) &&
           VerifyField<uint64_t>(verifier, VT_VALUE) &&
           VerifyField<int8_t>(verifier, VT_PACKINGTYPE) &&
           VerifyField<uint64_t>(verifier, VT_TEXTUREPACKNAME) &&
           VerifyOffset(verifier, VT_TEXTUREPACKNAMESTR) &&
           verifier.VerifyString(texturePackNameStr()) &&
           VerifyField<uint8_t>(verifier, VT_MINFILTER) &&
           VerifyField<uint8_t>(verifier, VT_MAGFILTER) &&
           VerifyField<float>(verifier, VT_ANISOTROPICFILTERING) &&
           VerifyField<uint8_t>(verifier, VT_WRAPS) &&
           VerifyField<uint8_t>(verifier, VT_WRAPT) &&
           VerifyField<uint8_t>(verifier, VT_WRAPR) &&
           verifier.EndTable();
  }
};

struct MaterialTextureParamBuilder {
  typedef MaterialTextureParam Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_propertyType(flat::MaterialPropertyType propertyType) {
    fbb_.AddElement<uint16_t>(MaterialTextureParam::VT_PROPERTYTYPE, static_cast<uint16_t>(propertyType), 0);
  }
  void add_value(uint64_t value) {
    fbb_.AddElement<uint64_t>(MaterialTextureParam::VT_VALUE, value, 0);
  }
  void add_packingType(flat::MaterialPropertyPackingType packingType) {
    fbb_.AddElement<int8_t>(MaterialTextureParam::VT_PACKINGTYPE, static_cast<int8_t>(packingType), 0);
  }
  void add_texturePackName(uint64_t texturePackName) {
    fbb_.AddElement<uint64_t>(MaterialTextureParam::VT_TEXTUREPACKNAME, texturePackName, 0);
  }
  void add_texturePackNameStr(flatbuffers::Offset<flatbuffers::String> texturePackNameStr) {
    fbb_.AddOffset(MaterialTextureParam::VT_TEXTUREPACKNAMESTR, texturePackNameStr);
  }
  void add_minFilter(flat::MinTextureFilter minFilter) {
    fbb_.AddElement<uint8_t>(MaterialTextureParam::VT_MINFILTER, static_cast<uint8_t>(minFilter), 0);
  }
  void add_magFilter(flat::MagTextureFilter magFilter) {
    fbb_.AddElement<uint8_t>(MaterialTextureParam::VT_MAGFILTER, static_cast<uint8_t>(magFilter), 0);
  }
  void add_anisotropicFiltering(float anisotropicFiltering) {
    fbb_.AddElement<float>(MaterialTextureParam::VT_ANISOTROPICFILTERING, anisotropicFiltering, 0.0f);
  }
  void add_wrapS(flat::TextureWrapMode wrapS) {
    fbb_.AddElement<uint8_t>(MaterialTextureParam::VT_WRAPS, static_cast<uint8_t>(wrapS), 0);
  }
  void add_wrapT(flat::TextureWrapMode wrapT) {
    fbb_.AddElement<uint8_t>(MaterialTextureParam::VT_WRAPT, static_cast<uint8_t>(wrapT), 0);
  }
  void add_wrapR(flat::TextureWrapMode wrapR) {
    fbb_.AddElement<uint8_t>(MaterialTextureParam::VT_WRAPR, static_cast<uint8_t>(wrapR), 0);
  }
  explicit MaterialTextureParamBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialTextureParamBuilder &operator=(const MaterialTextureParamBuilder &);
  flatbuffers::Offset<MaterialTextureParam> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MaterialTextureParam>(end);
    return o;
  }
};

inline flatbuffers::Offset<MaterialTextureParam> CreateMaterialTextureParam(
    flatbuffers::FlatBufferBuilder &_fbb,
    flat::MaterialPropertyType propertyType = flat::MaterialPropertyType_ALBEDO,
    uint64_t value = 0,
    flat::MaterialPropertyPackingType packingType = flat::MaterialPropertyPackingType_R,
    uint64_t texturePackName = 0,
    flatbuffers::Offset<flatbuffers::String> texturePackNameStr = 0,
    flat::MinTextureFilter minFilter = flat::MinTextureFilter_LINEAR,
    flat::MagTextureFilter magFilter = flat::MagTextureFilter_LINEAR,
    float anisotropicFiltering = 0.0f,
    flat::TextureWrapMode wrapS = flat::TextureWrapMode_REPEAT,
    flat::TextureWrapMode wrapT = flat::TextureWrapMode_REPEAT,
    flat::TextureWrapMode wrapR = flat::TextureWrapMode_REPEAT) {
  MaterialTextureParamBuilder builder_(_fbb);
  builder_.add_texturePackName(texturePackName);
  builder_.add_value(value);
  builder_.add_anisotropicFiltering(anisotropicFiltering);
  builder_.add_texturePackNameStr(texturePackNameStr);
  builder_.add_propertyType(propertyType);
  builder_.add_wrapR(wrapR);
  builder_.add_wrapT(wrapT);
  builder_.add_wrapS(wrapS);
  builder_.add_magFilter(magFilter);
  builder_.add_minFilter(minFilter);
  builder_.add_packingType(packingType);
  return builder_.Finish();
}

inline flatbuffers::Offset<MaterialTextureParam> CreateMaterialTextureParamDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    flat::MaterialPropertyType propertyType = flat::MaterialPropertyType_ALBEDO,
    uint64_t value = 0,
    flat::MaterialPropertyPackingType packingType = flat::MaterialPropertyPackingType_R,
    uint64_t texturePackName = 0,
    const char *texturePackNameStr = nullptr,
    flat::MinTextureFilter minFilter = flat::MinTextureFilter_LINEAR,
    flat::MagTextureFilter magFilter = flat::MagTextureFilter_LINEAR,
    float anisotropicFiltering = 0.0f,
    flat::TextureWrapMode wrapS = flat::TextureWrapMode_REPEAT,
    flat::TextureWrapMode wrapT = flat::TextureWrapMode_REPEAT,
    flat::TextureWrapMode wrapR = flat::TextureWrapMode_REPEAT) {
  auto texturePackNameStr__ = texturePackNameStr ? _fbb.CreateString(texturePackNameStr) : 0;
  return flat::CreateMaterialTextureParam(
      _fbb,
      propertyType,
      value,
      packingType,
      texturePackName,
      texturePackNameStr__,
      minFilter,
      magFilter,
      anisotropicFiltering,
      wrapS,
      wrapT,
      wrapR);
}

struct MaterialParams FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MaterialParamsBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_ULONGPARAMS = 6,
    VT_BOOLPARAMS = 8,
    VT_FLOATPARAMS = 10,
    VT_COLORPARAMS = 12,
    VT_TEXTUREPARAMS = 14,
    VT_STRINGPARAMS = 16
  };
  uint64_t name() const {
    return GetField<uint64_t>(VT_NAME, 0);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialULongParam>> *ulongParams() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialULongParam>> *>(VT_ULONGPARAMS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialBoolParam>> *boolParams() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialBoolParam>> *>(VT_BOOLPARAMS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialFloatParam>> *floatParams() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialFloatParam>> *>(VT_FLOATPARAMS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialColorParam>> *colorParams() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialColorParam>> *>(VT_COLORPARAMS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialTextureParam>> *textureParams() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialTextureParam>> *>(VT_TEXTUREPARAMS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialStringParam>> *stringParams() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::MaterialStringParam>> *>(VT_STRINGPARAMS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_NAME) &&
           VerifyOffset(verifier, VT_ULONGPARAMS) &&
           verifier.VerifyVector(ulongParams()) &&
           verifier.VerifyVectorOfTables(ulongParams()) &&
           VerifyOffset(verifier, VT_BOOLPARAMS) &&
           verifier.VerifyVector(boolParams()) &&
           verifier.VerifyVectorOfTables(boolParams()) &&
           VerifyOffset(verifier, VT_FLOATPARAMS) &&
           verifier.VerifyVector(floatParams()) &&
           verifier.VerifyVectorOfTables(floatParams()) &&
           VerifyOffset(verifier, VT_COLORPARAMS) &&
           verifier.VerifyVector(colorParams()) &&
           verifier.VerifyVectorOfTables(colorParams()) &&
           VerifyOffset(verifier, VT_TEXTUREPARAMS) &&
           verifier.VerifyVector(textureParams()) &&
           verifier.VerifyVectorOfTables(textureParams()) &&
           VerifyOffset(verifier, VT_STRINGPARAMS) &&
           verifier.VerifyVector(stringParams()) &&
           verifier.VerifyVectorOfTables(stringParams()) &&
           verifier.EndTable();
  }
};

struct MaterialParamsBuilder {
  typedef MaterialParams Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(uint64_t name) {
    fbb_.AddElement<uint64_t>(MaterialParams::VT_NAME, name, 0);
  }
  void add_ulongParams(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialULongParam>>> ulongParams) {
    fbb_.AddOffset(MaterialParams::VT_ULONGPARAMS, ulongParams);
  }
  void add_boolParams(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialBoolParam>>> boolParams) {
    fbb_.AddOffset(MaterialParams::VT_BOOLPARAMS, boolParams);
  }
  void add_floatParams(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialFloatParam>>> floatParams) {
    fbb_.AddOffset(MaterialParams::VT_FLOATPARAMS, floatParams);
  }
  void add_colorParams(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialColorParam>>> colorParams) {
    fbb_.AddOffset(MaterialParams::VT_COLORPARAMS, colorParams);
  }
  void add_textureParams(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialTextureParam>>> textureParams) {
    fbb_.AddOffset(MaterialParams::VT_TEXTUREPARAMS, textureParams);
  }
  void add_stringParams(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialStringParam>>> stringParams) {
    fbb_.AddOffset(MaterialParams::VT_STRINGPARAMS, stringParams);
  }
  explicit MaterialParamsBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MaterialParamsBuilder &operator=(const MaterialParamsBuilder &);
  flatbuffers::Offset<MaterialParams> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<MaterialParams>(end);
    return o;
  }
};

inline flatbuffers::Offset<MaterialParams> CreateMaterialParams(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t name = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialULongParam>>> ulongParams = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialBoolParam>>> boolParams = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialFloatParam>>> floatParams = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialColorParam>>> colorParams = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialTextureParam>>> textureParams = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::MaterialStringParam>>> stringParams = 0) {
  MaterialParamsBuilder builder_(_fbb);
  builder_.add_name(name);
  builder_.add_stringParams(stringParams);
  builder_.add_textureParams(textureParams);
  builder_.add_colorParams(colorParams);
  builder_.add_floatParams(floatParams);
  builder_.add_boolParams(boolParams);
  builder_.add_ulongParams(ulongParams);
  return builder_.Finish();
}

inline flatbuffers::Offset<MaterialParams> CreateMaterialParamsDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t name = 0,
    const std::vector<flatbuffers::Offset<flat::MaterialULongParam>> *ulongParams = nullptr,
    const std::vector<flatbuffers::Offset<flat::MaterialBoolParam>> *boolParams = nullptr,
    const std::vector<flatbuffers::Offset<flat::MaterialFloatParam>> *floatParams = nullptr,
    const std::vector<flatbuffers::Offset<flat::MaterialColorParam>> *colorParams = nullptr,
    const std::vector<flatbuffers::Offset<flat::MaterialTextureParam>> *textureParams = nullptr,
    const std::vector<flatbuffers::Offset<flat::MaterialStringParam>> *stringParams = nullptr) {
  auto ulongParams__ = ulongParams ? _fbb.CreateVector<flatbuffers::Offset<flat::MaterialULongParam>>(*ulongParams) : 0;
  auto boolParams__ = boolParams ? _fbb.CreateVector<flatbuffers::Offset<flat::MaterialBoolParam>>(*boolParams) : 0;
  auto floatParams__ = floatParams ? _fbb.CreateVector<flatbuffers::Offset<flat::MaterialFloatParam>>(*floatParams) : 0;
  auto colorParams__ = colorParams ? _fbb.CreateVector<flatbuffers::Offset<flat::MaterialColorParam>>(*colorParams) : 0;
  auto textureParams__ = textureParams ? _fbb.CreateVector<flatbuffers::Offset<flat::MaterialTextureParam>>(*textureParams) : 0;
  auto stringParams__ = stringParams ? _fbb.CreateVector<flatbuffers::Offset<flat::MaterialStringParam>>(*stringParams) : 0;
  return flat::CreateMaterialParams(
      _fbb,
      name,
      ulongParams__,
      boolParams__,
      floatParams__,
      colorParams__,
      textureParams__,
      stringParams__);
}

inline const flat::MaterialParams *GetMaterialParams(const void *buf) {
  return flatbuffers::GetRoot<flat::MaterialParams>(buf);
}

inline const flat::MaterialParams *GetSizePrefixedMaterialParams(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::MaterialParams>(buf);
}

inline const char *MaterialParamsIdentifier() {
  return "mprm";
}

inline bool MaterialParamsBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, MaterialParamsIdentifier());
}

inline bool VerifyMaterialParamsBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::MaterialParams>(MaterialParamsIdentifier());
}

inline bool VerifySizePrefixedMaterialParamsBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::MaterialParams>(MaterialParamsIdentifier());
}

inline const char *MaterialParamsExtension() {
  return "mprm";
}

inline void FinishMaterialParamsBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::MaterialParams> root) {
  fbb.Finish(root, MaterialParamsIdentifier());
}

inline void FinishSizePrefixedMaterialParamsBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::MaterialParams> root) {
  fbb.FinishSizePrefixed(root, MaterialParamsIdentifier());
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_MATERIALPARAMS_FLAT_H_
