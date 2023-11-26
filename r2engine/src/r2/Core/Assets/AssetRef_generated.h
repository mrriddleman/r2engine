// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_ASSETREF_FLAT_H_
#define FLATBUFFERS_GENERATED_ASSETREF_FLAT_H_

#include "flatbuffers/flatbuffers.h"

#include "AssetName_generated.h"

namespace flat {

struct AssetRef;
struct AssetRefBuilder;

struct AssetRef FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef AssetRefBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ASSETNAME = 4,
    VT_PATH = 6
  };
  const flat::AssetName *assetName() const {
    return GetPointer<const flat::AssetName *>(VT_ASSETNAME);
  }
  const flatbuffers::String *path() const {
    return GetPointer<const flatbuffers::String *>(VT_PATH);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_ASSETNAME) &&
           verifier.VerifyTable(assetName()) &&
           VerifyOffset(verifier, VT_PATH) &&
           verifier.VerifyString(path()) &&
           verifier.EndTable();
  }
};

struct AssetRefBuilder {
  typedef AssetRef Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_assetName(flatbuffers::Offset<flat::AssetName> assetName) {
    fbb_.AddOffset(AssetRef::VT_ASSETNAME, assetName);
  }
  void add_path(flatbuffers::Offset<flatbuffers::String> path) {
    fbb_.AddOffset(AssetRef::VT_PATH, path);
  }
  explicit AssetRefBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  AssetRefBuilder &operator=(const AssetRefBuilder &);
  flatbuffers::Offset<AssetRef> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<AssetRef>(end);
    return o;
  }
};

inline flatbuffers::Offset<AssetRef> CreateAssetRef(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flat::AssetName> assetName = 0,
    flatbuffers::Offset<flatbuffers::String> path = 0) {
  AssetRefBuilder builder_(_fbb);
  builder_.add_path(path);
  builder_.add_assetName(assetName);
  return builder_.Finish();
}

inline flatbuffers::Offset<AssetRef> CreateAssetRefDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flat::AssetName> assetName = 0,
    const char *path = nullptr) {
  auto path__ = path ? _fbb.CreateString(path) : 0;
  return flat::CreateAssetRef(
      _fbb,
      assetName,
      path__);
}

inline const flat::AssetRef *GetAssetRef(const void *buf) {
  return flatbuffers::GetRoot<flat::AssetRef>(buf);
}

inline const flat::AssetRef *GetSizePrefixedAssetRef(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::AssetRef>(buf);
}

inline const char *AssetRefIdentifier() {
  return "rarf";
}

inline bool AssetRefBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, AssetRefIdentifier());
}

inline bool VerifyAssetRefBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::AssetRef>(AssetRefIdentifier());
}

inline bool VerifySizePrefixedAssetRefBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::AssetRef>(AssetRefIdentifier());
}

inline const char *AssetRefExtension() {
  return "rarf";
}

inline void FinishAssetRefBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::AssetRef> root) {
  fbb.Finish(root, AssetRefIdentifier());
}

inline void FinishSizePrefixedAssetRefBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::AssetRef> root) {
  fbb.FinishSizePrefixed(root, AssetRefIdentifier());
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_ASSETREF_FLAT_H_
