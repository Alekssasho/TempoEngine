// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_SHADERLIBRARY_TEMPEST_DEFINITION_H_
#define FLATBUFFERS_GENERATED_SHADERLIBRARY_TEMPEST_DEFINITION_H_

#include "flatbuffers/flatbuffers.h"

namespace Tempest {
namespace Definition {

struct Shader;
struct ShaderBuilder;

struct ShaderLibrary;
struct ShaderLibraryBuilder;

enum ShaderType {
  ShaderType_Vertex = 0,
  ShaderType_Pixel = 1,
  ShaderType_MIN = ShaderType_Vertex,
  ShaderType_MAX = ShaderType_Pixel
};

inline const ShaderType (&EnumValuesShaderType())[2] {
  static const ShaderType values[] = {
    ShaderType_Vertex,
    ShaderType_Pixel
  };
  return values;
}

inline const char * const *EnumNamesShaderType() {
  static const char * const names[3] = {
    "Vertex",
    "Pixel",
    nullptr
  };
  return names;
}

inline const char *EnumNameShaderType(ShaderType e) {
  if (flatbuffers::IsOutRange(e, ShaderType_Vertex, ShaderType_Pixel)) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesShaderType()[index];
}

struct Shader FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ShaderBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_TYPE = 6,
    VT_CODE = 8
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  bool KeyCompareLessThan(const Shader *o) const {
    return *name() < *o->name();
  }
  int KeyCompareWithValue(const char *val) const {
    return strcmp(name()->c_str(), val);
  }
  Tempest::Definition::ShaderType type() const {
    return static_cast<Tempest::Definition::ShaderType>(GetField<int8_t>(VT_TYPE, 0));
  }
  const flatbuffers::Vector<uint8_t> *code() const {
    return GetPointer<const flatbuffers::Vector<uint8_t> *>(VT_CODE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffsetRequired(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyField<int8_t>(verifier, VT_TYPE) &&
           VerifyOffset(verifier, VT_CODE) &&
           verifier.VerifyVector(code()) &&
           verifier.EndTable();
  }
};

struct ShaderBuilder {
  typedef Shader Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Shader::VT_NAME, name);
  }
  void add_type(Tempest::Definition::ShaderType type) {
    fbb_.AddElement<int8_t>(Shader::VT_TYPE, static_cast<int8_t>(type), 0);
  }
  void add_code(flatbuffers::Offset<flatbuffers::Vector<uint8_t>> code) {
    fbb_.AddOffset(Shader::VT_CODE, code);
  }
  explicit ShaderBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<Shader> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Shader>(end);
    fbb_.Required(o, Shader::VT_NAME);
    return o;
  }
};

inline flatbuffers::Offset<Shader> CreateShader(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    Tempest::Definition::ShaderType type = Tempest::Definition::ShaderType_Vertex,
    flatbuffers::Offset<flatbuffers::Vector<uint8_t>> code = 0) {
  ShaderBuilder builder_(_fbb);
  builder_.add_code(code);
  builder_.add_name(name);
  builder_.add_type(type);
  return builder_.Finish();
}

inline flatbuffers::Offset<Shader> CreateShaderDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    Tempest::Definition::ShaderType type = Tempest::Definition::ShaderType_Vertex,
    const std::vector<uint8_t> *code = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto code__ = code ? _fbb.CreateVector<uint8_t>(*code) : 0;
  return Tempest::Definition::CreateShader(
      _fbb,
      name__,
      type,
      code__);
}

struct ShaderLibrary FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  typedef ShaderLibraryBuilder Builder;
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_SHADERS = 4
  };
  const flatbuffers::Vector<flatbuffers::Offset<Tempest::Definition::Shader>> *shaders() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Tempest::Definition::Shader>> *>(VT_SHADERS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_SHADERS) &&
           verifier.VerifyVector(shaders()) &&
           verifier.VerifyVectorOfTables(shaders()) &&
           verifier.EndTable();
  }
};

struct ShaderLibraryBuilder {
  typedef ShaderLibrary Table;
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_shaders(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Tempest::Definition::Shader>>> shaders) {
    fbb_.AddOffset(ShaderLibrary::VT_SHADERS, shaders);
  }
  explicit ShaderLibraryBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  flatbuffers::Offset<ShaderLibrary> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<ShaderLibrary>(end);
    return o;
  }
};

inline flatbuffers::Offset<ShaderLibrary> CreateShaderLibrary(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Tempest::Definition::Shader>>> shaders = 0) {
  ShaderLibraryBuilder builder_(_fbb);
  builder_.add_shaders(shaders);
  return builder_.Finish();
}

inline flatbuffers::Offset<ShaderLibrary> CreateShaderLibraryDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    std::vector<flatbuffers::Offset<Tempest::Definition::Shader>> *shaders = nullptr) {
  auto shaders__ = shaders ? _fbb.CreateVectorOfSortedTables<Tempest::Definition::Shader>(shaders) : 0;
  return Tempest::Definition::CreateShaderLibrary(
      _fbb,
      shaders__);
}

inline const Tempest::Definition::ShaderLibrary *GetShaderLibrary(const void *buf) {
  return flatbuffers::GetRoot<Tempest::Definition::ShaderLibrary>(buf);
}

inline const Tempest::Definition::ShaderLibrary *GetSizePrefixedShaderLibrary(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<Tempest::Definition::ShaderLibrary>(buf);
}

inline const char *ShaderLibraryIdentifier() {
  return "TSLB";
}

inline bool ShaderLibraryBufferHasIdentifier(const void *buf) {
  return flatbuffers::BufferHasIdentifier(
      buf, ShaderLibraryIdentifier());
}

inline bool VerifyShaderLibraryBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<Tempest::Definition::ShaderLibrary>(ShaderLibraryIdentifier());
}

inline bool VerifySizePrefixedShaderLibraryBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<Tempest::Definition::ShaderLibrary>(ShaderLibraryIdentifier());
}

inline const char *ShaderLibraryExtension() {
  return "tslb";
}

inline void FinishShaderLibraryBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<Tempest::Definition::ShaderLibrary> root) {
  fbb.Finish(root, ShaderLibraryIdentifier());
}

inline void FinishSizePrefixedShaderLibraryBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<Tempest::Definition::ShaderLibrary> root) {
  fbb.FinishSizePrefixed(root, ShaderLibraryIdentifier());
}

}  // namespace Definition
}  // namespace Tempest

#endif  // FLATBUFFERS_GENERATED_SHADERLIBRARY_TEMPEST_DEFINITION_H_