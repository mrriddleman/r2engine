// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_MESH_FLAT_H_
#define FLATBUFFERS_GENERATED_MESH_FLAT_H_

#include "flatbuffers/flatbuffers.h"

namespace flat {

struct Vertex3;

struct Vertex2;

struct Face;
struct FaceBuilder;

struct Mesh;
struct MeshBuilder;

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Vertex3 FLATBUFFERS_FINAL_CLASS {
 private:
  float x_;
  float y_;
  float z_;

 public:
  Vertex3() {
    memset(static_cast<void *>(this), 0, sizeof(Vertex3));
  }
  Vertex3(float _x, float _y, float _z)
      : x_(flatbuffers::EndianScalar(_x)),
        y_(flatbuffers::EndianScalar(_y)),
        z_(flatbuffers::EndianScalar(_z)) {
  }
  float x() const {
    return flatbuffers::EndianScalar(x_);
  }
  float y() const {
    return flatbuffers::EndianScalar(y_);
  }
  float z() const {
    return flatbuffers::EndianScalar(z_);
  }
};
FLATBUFFERS_STRUCT_END(Vertex3, 12);

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) Vertex2 FLATBUFFERS_FINAL_CLASS {
 private:
  float x_;
  float y_;

 public:
  Vertex2() {
    memset(static_cast<void *>(this), 0, sizeof(Vertex2));
  }
  Vertex2(float _x, float _y)
      : x_(flatbuffers::EndianScalar(_x)),
        y_(flatbuffers::EndianScalar(_y)) {
  }
  float x() const {
    return flatbuffers::EndianScalar(x_);
  }
  float y() const {
    return flatbuffers::EndianScalar(y_);
  }
};
FLATBUFFERS_STRUCT_END(Vertex2, 8);

struct Face FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef FaceBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NUMINDICES = 4,
    VT_INDICES = 6
  };
  uint64_t numIndices() const {
    return GetField<uint64_t>(VT_NUMINDICES, 0);
  }
  const flatbuffers::Vector<uint32_t> *indices() const {
    return GetPointer<const flatbuffers::Vector<uint32_t> *>(VT_INDICES);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_NUMINDICES) &&
           VerifyOffset(verifier, VT_INDICES) &&
           verifier.VerifyVector(indices()) &&
           verifier.EndTable();
  }
};

struct FaceBuilder {
  typedef Face Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_numIndices(uint64_t numIndices) {
    fbb_.AddElement<uint64_t>(Face::VT_NUMINDICES, numIndices, 0);
  }
  void add_indices(flatbuffers::Offset<flatbuffers::Vector<uint32_t>> indices) {
    fbb_.AddOffset(Face::VT_INDICES, indices);
  }
  explicit FaceBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  FaceBuilder &operator=(const FaceBuilder &);
  flatbuffers::Offset<Face> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Face>(end);
    return o;
  }
};

inline flatbuffers::Offset<Face> CreateFace(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t numIndices = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint32_t>> indices = 0) {
  FaceBuilder builder_(_fbb);
  builder_.add_numIndices(numIndices);
  builder_.add_indices(indices);
  return builder_.Finish();
}

inline flatbuffers::Offset<Face> CreateFaceDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t numIndices = 0,
    const std::vector<uint32_t> *indices = nullptr) {
  auto indices__ = indices ? _fbb.CreateVector<uint32_t>(*indices) : 0;
  return flat::CreateFace(
      _fbb,
      numIndices,
      indices__);
}

struct Mesh FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef MeshBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_NUMVERTICES = 6,
    VT_NUMFACES = 8,
    VT_POSITIONS = 10,
    VT_NORMALS = 12,
    VT_TANGENTS = 14,
    VT_BITANGENTS = 16,
    VT_TEXTURECOORDS = 18,
    VT_FACES = 20
  };
  uint64_t name() const {
    return GetField<uint64_t>(VT_NAME, 0);
  }
  uint64_t numVertices() const {
    return GetField<uint64_t>(VT_NUMVERTICES, 0);
  }
  uint64_t numFaces() const {
    return GetField<uint64_t>(VT_NUMFACES, 0);
  }
  const flatbuffers::Vector<const flat::Vertex3 *> *positions() const {
    return GetPointer<const flatbuffers::Vector<const flat::Vertex3 *> *>(VT_POSITIONS);
  }
  const flatbuffers::Vector<const flat::Vertex3 *> *normals() const {
    return GetPointer<const flatbuffers::Vector<const flat::Vertex3 *> *>(VT_NORMALS);
  }
  const flatbuffers::Vector<const flat::Vertex3 *> *tangents() const {
    return GetPointer<const flatbuffers::Vector<const flat::Vertex3 *> *>(VT_TANGENTS);
  }
  const flatbuffers::Vector<const flat::Vertex3 *> *bitangents() const {
    return GetPointer<const flatbuffers::Vector<const flat::Vertex3 *> *>(VT_BITANGENTS);
  }
  const flatbuffers::Vector<const flat::Vertex2 *> *textureCoords() const {
    return GetPointer<const flatbuffers::Vector<const flat::Vertex2 *> *>(VT_TEXTURECOORDS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<flat::Face>> *faces() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<flat::Face>> *>(VT_FACES);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint64_t>(verifier, VT_NAME) &&
           VerifyField<uint64_t>(verifier, VT_NUMVERTICES) &&
           VerifyField<uint64_t>(verifier, VT_NUMFACES) &&
           VerifyOffset(verifier, VT_POSITIONS) &&
           verifier.VerifyVector(positions()) &&
           VerifyOffset(verifier, VT_NORMALS) &&
           verifier.VerifyVector(normals()) &&
           VerifyOffset(verifier, VT_TANGENTS) &&
           verifier.VerifyVector(tangents()) &&
           VerifyOffset(verifier, VT_BITANGENTS) &&
           verifier.VerifyVector(bitangents()) &&
           VerifyOffset(verifier, VT_TEXTURECOORDS) &&
           verifier.VerifyVector(textureCoords()) &&
           VerifyOffset(verifier, VT_FACES) &&
           verifier.VerifyVector(faces()) &&
           verifier.VerifyVectorOfTables(faces()) &&
           verifier.EndTable();
  }
};

struct MeshBuilder {
  typedef Mesh Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(uint64_t name) {
    fbb_.AddElement<uint64_t>(Mesh::VT_NAME, name, 0);
  }
  void add_numVertices(uint64_t numVertices) {
    fbb_.AddElement<uint64_t>(Mesh::VT_NUMVERTICES, numVertices, 0);
  }
  void add_numFaces(uint64_t numFaces) {
    fbb_.AddElement<uint64_t>(Mesh::VT_NUMFACES, numFaces, 0);
  }
  void add_positions(flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex3 *>> positions) {
    fbb_.AddOffset(Mesh::VT_POSITIONS, positions);
  }
  void add_normals(flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex3 *>> normals) {
    fbb_.AddOffset(Mesh::VT_NORMALS, normals);
  }
  void add_tangents(flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex3 *>> tangents) {
    fbb_.AddOffset(Mesh::VT_TANGENTS, tangents);
  }
  void add_bitangents(flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex3 *>> bitangents) {
    fbb_.AddOffset(Mesh::VT_BITANGENTS, bitangents);
  }
  void add_textureCoords(flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex2 *>> textureCoords) {
    fbb_.AddOffset(Mesh::VT_TEXTURECOORDS, textureCoords);
  }
  void add_faces(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::Face>>> faces) {
    fbb_.AddOffset(Mesh::VT_FACES, faces);
  }
  explicit MeshBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  MeshBuilder &operator=(const MeshBuilder &);
  flatbuffers::Offset<Mesh> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Mesh>(end);
    return o;
  }
};

inline flatbuffers::Offset<Mesh> CreateMesh(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t name = 0,
    uint64_t numVertices = 0,
    uint64_t numFaces = 0,
    flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex3 *>> positions = 0,
    flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex3 *>> normals = 0,
    flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex3 *>> tangents = 0,
    flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex3 *>> bitangents = 0,
    flatbuffers::Offset<flatbuffers::Vector<const flat::Vertex2 *>> textureCoords = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<flat::Face>>> faces = 0) {
  MeshBuilder builder_(_fbb);
  builder_.add_numFaces(numFaces);
  builder_.add_numVertices(numVertices);
  builder_.add_name(name);
  builder_.add_faces(faces);
  builder_.add_textureCoords(textureCoords);
  builder_.add_bitangents(bitangents);
  builder_.add_tangents(tangents);
  builder_.add_normals(normals);
  builder_.add_positions(positions);
  return builder_.Finish();
}

inline flatbuffers::Offset<Mesh> CreateMeshDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint64_t name = 0,
    uint64_t numVertices = 0,
    uint64_t numFaces = 0,
    const std::vector<flat::Vertex3> *positions = nullptr,
    const std::vector<flat::Vertex3> *normals = nullptr,
    const std::vector<flat::Vertex3> *tangents = nullptr,
    const std::vector<flat::Vertex3> *bitangents = nullptr,
    const std::vector<flat::Vertex2> *textureCoords = nullptr,
    const std::vector<flatbuffers::Offset<flat::Face>> *faces = nullptr) {
  auto positions__ = positions ? _fbb.CreateVectorOfStructs<flat::Vertex3>(*positions) : 0;
  auto normals__ = normals ? _fbb.CreateVectorOfStructs<flat::Vertex3>(*normals) : 0;
  auto tangents__ = tangents ? _fbb.CreateVectorOfStructs<flat::Vertex3>(*tangents) : 0;
  auto bitangents__ = bitangents ? _fbb.CreateVectorOfStructs<flat::Vertex3>(*bitangents) : 0;
  auto textureCoords__ = textureCoords ? _fbb.CreateVectorOfStructs<flat::Vertex2>(*textureCoords) : 0;
  auto faces__ = faces ? _fbb.CreateVector<flatbuffers::Offset<flat::Face>>(*faces) : 0;
  return flat::CreateMesh(
      _fbb,
      name,
      numVertices,
      numFaces,
      positions__,
      normals__,
      tangents__,
      bitangents__,
      textureCoords__,
      faces__);
}

inline const flat::Mesh *GetMesh(const void *buf) {
  return flatbuffers::GetRoot<flat::Mesh>(buf);
}

inline const flat::Mesh *GetSizePrefixedMesh(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<flat::Mesh>(buf);
}

inline const char *MeshIdentifier() {
  return "mesh";
}

inline bool MeshBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, MeshIdentifier());
}

inline bool VerifyMeshBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<flat::Mesh>(MeshIdentifier());
}

inline bool VerifySizePrefixedMeshBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<flat::Mesh>(MeshIdentifier());
}

inline const char *MeshExtension() {
  return "mesh";
}

inline void FinishMeshBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::Mesh> root) {
  fbb.Finish(root, MeshIdentifier());
}

inline void FinishSizePrefixedMeshBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<flat::Mesh> root) {
  fbb.FinishSizePrefixed(root, MeshIdentifier());
}

}  // namespace flat

#endif  // FLATBUFFERS_GENERATED_MESH_FLAT_H_
