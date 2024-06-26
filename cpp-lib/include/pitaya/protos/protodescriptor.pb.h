// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: protodescriptor.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_protodescriptor_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_protodescriptor_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021006 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_protodescriptor_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_protodescriptor_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_protodescriptor_2eproto;
namespace protos {
class ProtoDescriptor;
struct ProtoDescriptorDefaultTypeInternal;
extern ProtoDescriptorDefaultTypeInternal _ProtoDescriptor_default_instance_;
class ProtoDescriptors;
struct ProtoDescriptorsDefaultTypeInternal;
extern ProtoDescriptorsDefaultTypeInternal _ProtoDescriptors_default_instance_;
class ProtoName;
struct ProtoNameDefaultTypeInternal;
extern ProtoNameDefaultTypeInternal _ProtoName_default_instance_;
class ProtoNames;
struct ProtoNamesDefaultTypeInternal;
extern ProtoNamesDefaultTypeInternal _ProtoNames_default_instance_;
}  // namespace protos
PROTOBUF_NAMESPACE_OPEN
template<> ::protos::ProtoDescriptor* Arena::CreateMaybeMessage<::protos::ProtoDescriptor>(Arena*);
template<> ::protos::ProtoDescriptors* Arena::CreateMaybeMessage<::protos::ProtoDescriptors>(Arena*);
template<> ::protos::ProtoName* Arena::CreateMaybeMessage<::protos::ProtoName>(Arena*);
template<> ::protos::ProtoNames* Arena::CreateMaybeMessage<::protos::ProtoNames>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace protos {

// ===================================================================

class ProtoDescriptor final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:protos.ProtoDescriptor) */ {
 public:
  inline ProtoDescriptor() : ProtoDescriptor(nullptr) {}
  ~ProtoDescriptor() override;
  explicit PROTOBUF_CONSTEXPR ProtoDescriptor(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ProtoDescriptor(const ProtoDescriptor& from);
  ProtoDescriptor(ProtoDescriptor&& from) noexcept
    : ProtoDescriptor() {
    *this = ::std::move(from);
  }

  inline ProtoDescriptor& operator=(const ProtoDescriptor& from) {
    CopyFrom(from);
    return *this;
  }
  inline ProtoDescriptor& operator=(ProtoDescriptor&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ProtoDescriptor& default_instance() {
    return *internal_default_instance();
  }
  static inline const ProtoDescriptor* internal_default_instance() {
    return reinterpret_cast<const ProtoDescriptor*>(
               &_ProtoDescriptor_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(ProtoDescriptor& a, ProtoDescriptor& b) {
    a.Swap(&b);
  }
  inline void Swap(ProtoDescriptor* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ProtoDescriptor* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ProtoDescriptor* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ProtoDescriptor>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const ProtoDescriptor& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const ProtoDescriptor& from) {
    ProtoDescriptor::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ProtoDescriptor* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "protos.ProtoDescriptor";
  }
  protected:
  explicit ProtoDescriptor(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kDescFieldNumber = 1,
  };
  // bytes desc = 1;
  void clear_desc();
  const std::string& desc() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_desc(ArgT0&& arg0, ArgT... args);
  std::string* mutable_desc();
  PROTOBUF_NODISCARD std::string* release_desc();
  void set_allocated_desc(std::string* desc);
  private:
  const std::string& _internal_desc() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_desc(const std::string& value);
  std::string* _internal_mutable_desc();
  public:

  // @@protoc_insertion_point(class_scope:protos.ProtoDescriptor)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr desc_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_protodescriptor_2eproto;
};
// -------------------------------------------------------------------

class ProtoName final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:protos.ProtoName) */ {
 public:
  inline ProtoName() : ProtoName(nullptr) {}
  ~ProtoName() override;
  explicit PROTOBUF_CONSTEXPR ProtoName(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ProtoName(const ProtoName& from);
  ProtoName(ProtoName&& from) noexcept
    : ProtoName() {
    *this = ::std::move(from);
  }

  inline ProtoName& operator=(const ProtoName& from) {
    CopyFrom(from);
    return *this;
  }
  inline ProtoName& operator=(ProtoName&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ProtoName& default_instance() {
    return *internal_default_instance();
  }
  static inline const ProtoName* internal_default_instance() {
    return reinterpret_cast<const ProtoName*>(
               &_ProtoName_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(ProtoName& a, ProtoName& b) {
    a.Swap(&b);
  }
  inline void Swap(ProtoName* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ProtoName* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ProtoName* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ProtoName>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const ProtoName& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const ProtoName& from) {
    ProtoName::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ProtoName* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "protos.ProtoName";
  }
  protected:
  explicit ProtoName(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kNameFieldNumber = 1,
  };
  // string name = 1;
  void clear_name();
  const std::string& name() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_name(ArgT0&& arg0, ArgT... args);
  std::string* mutable_name();
  PROTOBUF_NODISCARD std::string* release_name();
  void set_allocated_name(std::string* name);
  private:
  const std::string& _internal_name() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_name(const std::string& value);
  std::string* _internal_mutable_name();
  public:

  // @@protoc_insertion_point(class_scope:protos.ProtoName)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_protodescriptor_2eproto;
};
// -------------------------------------------------------------------

class ProtoDescriptors final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:protos.ProtoDescriptors) */ {
 public:
  inline ProtoDescriptors() : ProtoDescriptors(nullptr) {}
  ~ProtoDescriptors() override;
  explicit PROTOBUF_CONSTEXPR ProtoDescriptors(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ProtoDescriptors(const ProtoDescriptors& from);
  ProtoDescriptors(ProtoDescriptors&& from) noexcept
    : ProtoDescriptors() {
    *this = ::std::move(from);
  }

  inline ProtoDescriptors& operator=(const ProtoDescriptors& from) {
    CopyFrom(from);
    return *this;
  }
  inline ProtoDescriptors& operator=(ProtoDescriptors&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ProtoDescriptors& default_instance() {
    return *internal_default_instance();
  }
  static inline const ProtoDescriptors* internal_default_instance() {
    return reinterpret_cast<const ProtoDescriptors*>(
               &_ProtoDescriptors_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    2;

  friend void swap(ProtoDescriptors& a, ProtoDescriptors& b) {
    a.Swap(&b);
  }
  inline void Swap(ProtoDescriptors* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ProtoDescriptors* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ProtoDescriptors* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ProtoDescriptors>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const ProtoDescriptors& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const ProtoDescriptors& from) {
    ProtoDescriptors::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ProtoDescriptors* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "protos.ProtoDescriptors";
  }
  protected:
  explicit ProtoDescriptors(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kDescFieldNumber = 1,
  };
  // repeated bytes desc = 1;
  int desc_size() const;
  private:
  int _internal_desc_size() const;
  public:
  void clear_desc();
  const std::string& desc(int index) const;
  std::string* mutable_desc(int index);
  void set_desc(int index, const std::string& value);
  void set_desc(int index, std::string&& value);
  void set_desc(int index, const char* value);
  void set_desc(int index, const void* value, size_t size);
  std::string* add_desc();
  void add_desc(const std::string& value);
  void add_desc(std::string&& value);
  void add_desc(const char* value);
  void add_desc(const void* value, size_t size);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>& desc() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>* mutable_desc();
  private:
  const std::string& _internal_desc(int index) const;
  std::string* _internal_add_desc();
  public:

  // @@protoc_insertion_point(class_scope:protos.ProtoDescriptors)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string> desc_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_protodescriptor_2eproto;
};
// -------------------------------------------------------------------

class ProtoNames final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:protos.ProtoNames) */ {
 public:
  inline ProtoNames() : ProtoNames(nullptr) {}
  ~ProtoNames() override;
  explicit PROTOBUF_CONSTEXPR ProtoNames(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  ProtoNames(const ProtoNames& from);
  ProtoNames(ProtoNames&& from) noexcept
    : ProtoNames() {
    *this = ::std::move(from);
  }

  inline ProtoNames& operator=(const ProtoNames& from) {
    CopyFrom(from);
    return *this;
  }
  inline ProtoNames& operator=(ProtoNames&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const ProtoNames& default_instance() {
    return *internal_default_instance();
  }
  static inline const ProtoNames* internal_default_instance() {
    return reinterpret_cast<const ProtoNames*>(
               &_ProtoNames_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    3;

  friend void swap(ProtoNames& a, ProtoNames& b) {
    a.Swap(&b);
  }
  inline void Swap(ProtoNames* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(ProtoNames* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  ProtoNames* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<ProtoNames>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const ProtoNames& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const ProtoNames& from) {
    ProtoNames::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(ProtoNames* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "protos.ProtoNames";
  }
  protected:
  explicit ProtoNames(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kNameFieldNumber = 1,
  };
  // repeated string name = 1;
  int name_size() const;
  private:
  int _internal_name_size() const;
  public:
  void clear_name();
  const std::string& name(int index) const;
  std::string* mutable_name(int index);
  void set_name(int index, const std::string& value);
  void set_name(int index, std::string&& value);
  void set_name(int index, const char* value);
  void set_name(int index, const char* value, size_t size);
  std::string* add_name();
  void add_name(const std::string& value);
  void add_name(std::string&& value);
  void add_name(const char* value);
  void add_name(const char* value, size_t size);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>& name() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>* mutable_name();
  private:
  const std::string& _internal_name(int index) const;
  std::string* _internal_add_name();
  public:

  // @@protoc_insertion_point(class_scope:protos.ProtoNames)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string> name_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_protodescriptor_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// ProtoDescriptor

// bytes desc = 1;
inline void ProtoDescriptor::clear_desc() {
  _impl_.desc_.ClearToEmpty();
}
inline const std::string& ProtoDescriptor::desc() const {
  // @@protoc_insertion_point(field_get:protos.ProtoDescriptor.desc)
  return _internal_desc();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void ProtoDescriptor::set_desc(ArgT0&& arg0, ArgT... args) {
 
 _impl_.desc_.SetBytes(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:protos.ProtoDescriptor.desc)
}
inline std::string* ProtoDescriptor::mutable_desc() {
  std::string* _s = _internal_mutable_desc();
  // @@protoc_insertion_point(field_mutable:protos.ProtoDescriptor.desc)
  return _s;
}
inline const std::string& ProtoDescriptor::_internal_desc() const {
  return _impl_.desc_.Get();
}
inline void ProtoDescriptor::_internal_set_desc(const std::string& value) {
  
  _impl_.desc_.Set(value, GetArenaForAllocation());
}
inline std::string* ProtoDescriptor::_internal_mutable_desc() {
  
  return _impl_.desc_.Mutable(GetArenaForAllocation());
}
inline std::string* ProtoDescriptor::release_desc() {
  // @@protoc_insertion_point(field_release:protos.ProtoDescriptor.desc)
  return _impl_.desc_.Release();
}
inline void ProtoDescriptor::set_allocated_desc(std::string* desc) {
  if (desc != nullptr) {
    
  } else {
    
  }
  _impl_.desc_.SetAllocated(desc, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.desc_.IsDefault()) {
    _impl_.desc_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:protos.ProtoDescriptor.desc)
}

// -------------------------------------------------------------------

// ProtoName

// string name = 1;
inline void ProtoName::clear_name() {
  _impl_.name_.ClearToEmpty();
}
inline const std::string& ProtoName::name() const {
  // @@protoc_insertion_point(field_get:protos.ProtoName.name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void ProtoName::set_name(ArgT0&& arg0, ArgT... args) {
 
 _impl_.name_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:protos.ProtoName.name)
}
inline std::string* ProtoName::mutable_name() {
  std::string* _s = _internal_mutable_name();
  // @@protoc_insertion_point(field_mutable:protos.ProtoName.name)
  return _s;
}
inline const std::string& ProtoName::_internal_name() const {
  return _impl_.name_.Get();
}
inline void ProtoName::_internal_set_name(const std::string& value) {
  
  _impl_.name_.Set(value, GetArenaForAllocation());
}
inline std::string* ProtoName::_internal_mutable_name() {
  
  return _impl_.name_.Mutable(GetArenaForAllocation());
}
inline std::string* ProtoName::release_name() {
  // @@protoc_insertion_point(field_release:protos.ProtoName.name)
  return _impl_.name_.Release();
}
inline void ProtoName::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    
  } else {
    
  }
  _impl_.name_.SetAllocated(name, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.name_.IsDefault()) {
    _impl_.name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:protos.ProtoName.name)
}

// -------------------------------------------------------------------

// ProtoDescriptors

// repeated bytes desc = 1;
inline int ProtoDescriptors::_internal_desc_size() const {
  return _impl_.desc_.size();
}
inline int ProtoDescriptors::desc_size() const {
  return _internal_desc_size();
}
inline void ProtoDescriptors::clear_desc() {
  _impl_.desc_.Clear();
}
inline std::string* ProtoDescriptors::add_desc() {
  std::string* _s = _internal_add_desc();
  // @@protoc_insertion_point(field_add_mutable:protos.ProtoDescriptors.desc)
  return _s;
}
inline const std::string& ProtoDescriptors::_internal_desc(int index) const {
  return _impl_.desc_.Get(index);
}
inline const std::string& ProtoDescriptors::desc(int index) const {
  // @@protoc_insertion_point(field_get:protos.ProtoDescriptors.desc)
  return _internal_desc(index);
}
inline std::string* ProtoDescriptors::mutable_desc(int index) {
  // @@protoc_insertion_point(field_mutable:protos.ProtoDescriptors.desc)
  return _impl_.desc_.Mutable(index);
}
inline void ProtoDescriptors::set_desc(int index, const std::string& value) {
  _impl_.desc_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set:protos.ProtoDescriptors.desc)
}
inline void ProtoDescriptors::set_desc(int index, std::string&& value) {
  _impl_.desc_.Mutable(index)->assign(std::move(value));
  // @@protoc_insertion_point(field_set:protos.ProtoDescriptors.desc)
}
inline void ProtoDescriptors::set_desc(int index, const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.desc_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set_char:protos.ProtoDescriptors.desc)
}
inline void ProtoDescriptors::set_desc(int index, const void* value, size_t size) {
  _impl_.desc_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:protos.ProtoDescriptors.desc)
}
inline std::string* ProtoDescriptors::_internal_add_desc() {
  return _impl_.desc_.Add();
}
inline void ProtoDescriptors::add_desc(const std::string& value) {
  _impl_.desc_.Add()->assign(value);
  // @@protoc_insertion_point(field_add:protos.ProtoDescriptors.desc)
}
inline void ProtoDescriptors::add_desc(std::string&& value) {
  _impl_.desc_.Add(std::move(value));
  // @@protoc_insertion_point(field_add:protos.ProtoDescriptors.desc)
}
inline void ProtoDescriptors::add_desc(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.desc_.Add()->assign(value);
  // @@protoc_insertion_point(field_add_char:protos.ProtoDescriptors.desc)
}
inline void ProtoDescriptors::add_desc(const void* value, size_t size) {
  _impl_.desc_.Add()->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_add_pointer:protos.ProtoDescriptors.desc)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>&
ProtoDescriptors::desc() const {
  // @@protoc_insertion_point(field_list:protos.ProtoDescriptors.desc)
  return _impl_.desc_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>*
ProtoDescriptors::mutable_desc() {
  // @@protoc_insertion_point(field_mutable_list:protos.ProtoDescriptors.desc)
  return &_impl_.desc_;
}

// -------------------------------------------------------------------

// ProtoNames

// repeated string name = 1;
inline int ProtoNames::_internal_name_size() const {
  return _impl_.name_.size();
}
inline int ProtoNames::name_size() const {
  return _internal_name_size();
}
inline void ProtoNames::clear_name() {
  _impl_.name_.Clear();
}
inline std::string* ProtoNames::add_name() {
  std::string* _s = _internal_add_name();
  // @@protoc_insertion_point(field_add_mutable:protos.ProtoNames.name)
  return _s;
}
inline const std::string& ProtoNames::_internal_name(int index) const {
  return _impl_.name_.Get(index);
}
inline const std::string& ProtoNames::name(int index) const {
  // @@protoc_insertion_point(field_get:protos.ProtoNames.name)
  return _internal_name(index);
}
inline std::string* ProtoNames::mutable_name(int index) {
  // @@protoc_insertion_point(field_mutable:protos.ProtoNames.name)
  return _impl_.name_.Mutable(index);
}
inline void ProtoNames::set_name(int index, const std::string& value) {
  _impl_.name_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set:protos.ProtoNames.name)
}
inline void ProtoNames::set_name(int index, std::string&& value) {
  _impl_.name_.Mutable(index)->assign(std::move(value));
  // @@protoc_insertion_point(field_set:protos.ProtoNames.name)
}
inline void ProtoNames::set_name(int index, const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.name_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set_char:protos.ProtoNames.name)
}
inline void ProtoNames::set_name(int index, const char* value, size_t size) {
  _impl_.name_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:protos.ProtoNames.name)
}
inline std::string* ProtoNames::_internal_add_name() {
  return _impl_.name_.Add();
}
inline void ProtoNames::add_name(const std::string& value) {
  _impl_.name_.Add()->assign(value);
  // @@protoc_insertion_point(field_add:protos.ProtoNames.name)
}
inline void ProtoNames::add_name(std::string&& value) {
  _impl_.name_.Add(std::move(value));
  // @@protoc_insertion_point(field_add:protos.ProtoNames.name)
}
inline void ProtoNames::add_name(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.name_.Add()->assign(value);
  // @@protoc_insertion_point(field_add_char:protos.ProtoNames.name)
}
inline void ProtoNames::add_name(const char* value, size_t size) {
  _impl_.name_.Add()->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_add_pointer:protos.ProtoNames.name)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>&
ProtoNames::name() const {
  // @@protoc_insertion_point(field_list:protos.ProtoNames.name)
  return _impl_.name_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>*
ProtoNames::mutable_name() {
  // @@protoc_insertion_point(field_mutable_list:protos.ProtoNames.name)
  return &_impl_.name_;
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------

// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace protos

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_protodescriptor_2eproto
