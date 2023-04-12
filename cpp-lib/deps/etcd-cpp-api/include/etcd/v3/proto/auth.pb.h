// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: auth.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_auth_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_auth_2eproto

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
#include <google/protobuf/generated_enum_reflection.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_auth_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_auth_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_auth_2eproto;
namespace authpb {
class Permission;
struct PermissionDefaultTypeInternal;
extern PermissionDefaultTypeInternal _Permission_default_instance_;
class Role;
struct RoleDefaultTypeInternal;
extern RoleDefaultTypeInternal _Role_default_instance_;
class User;
struct UserDefaultTypeInternal;
extern UserDefaultTypeInternal _User_default_instance_;
}  // namespace authpb
PROTOBUF_NAMESPACE_OPEN
template<> ::authpb::Permission* Arena::CreateMaybeMessage<::authpb::Permission>(Arena*);
template<> ::authpb::Role* Arena::CreateMaybeMessage<::authpb::Role>(Arena*);
template<> ::authpb::User* Arena::CreateMaybeMessage<::authpb::User>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace authpb {

enum Permission_Type : int {
  Permission_Type_READ = 0,
  Permission_Type_WRITE = 1,
  Permission_Type_READWRITE = 2,
  Permission_Type_Permission_Type_INT_MIN_SENTINEL_DO_NOT_USE_ = std::numeric_limits<int32_t>::min(),
  Permission_Type_Permission_Type_INT_MAX_SENTINEL_DO_NOT_USE_ = std::numeric_limits<int32_t>::max()
};
bool Permission_Type_IsValid(int value);
constexpr Permission_Type Permission_Type_Type_MIN = Permission_Type_READ;
constexpr Permission_Type Permission_Type_Type_MAX = Permission_Type_READWRITE;
constexpr int Permission_Type_Type_ARRAYSIZE = Permission_Type_Type_MAX + 1;

const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* Permission_Type_descriptor();
template<typename T>
inline const std::string& Permission_Type_Name(T enum_t_value) {
  static_assert(::std::is_same<T, Permission_Type>::value ||
    ::std::is_integral<T>::value,
    "Incorrect type passed to function Permission_Type_Name.");
  return ::PROTOBUF_NAMESPACE_ID::internal::NameOfEnum(
    Permission_Type_descriptor(), enum_t_value);
}
inline bool Permission_Type_Parse(
    ::PROTOBUF_NAMESPACE_ID::ConstStringParam name, Permission_Type* value) {
  return ::PROTOBUF_NAMESPACE_ID::internal::ParseNamedEnum<Permission_Type>(
    Permission_Type_descriptor(), name, value);
}
// ===================================================================

class User final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:authpb.User) */ {
 public:
  inline User() : User(nullptr) {}
  ~User() override;
  explicit PROTOBUF_CONSTEXPR User(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  User(const User& from);
  User(User&& from) noexcept
    : User() {
    *this = ::std::move(from);
  }

  inline User& operator=(const User& from) {
    CopyFrom(from);
    return *this;
  }
  inline User& operator=(User&& from) noexcept {
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
  static const User& default_instance() {
    return *internal_default_instance();
  }
  static inline const User* internal_default_instance() {
    return reinterpret_cast<const User*>(
               &_User_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(User& a, User& b) {
    a.Swap(&b);
  }
  inline void Swap(User* other) {
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
  void UnsafeArenaSwap(User* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  User* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<User>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const User& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const User& from) {
    User::MergeImpl(*this, from);
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
  void InternalSwap(User* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "authpb.User";
  }
  protected:
  explicit User(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kRolesFieldNumber = 3,
    kNameFieldNumber = 1,
    kPasswordFieldNumber = 2,
  };
  // repeated string roles = 3;
  int roles_size() const;
  private:
  int _internal_roles_size() const;
  public:
  void clear_roles();
  const std::string& roles(int index) const;
  std::string* mutable_roles(int index);
  void set_roles(int index, const std::string& value);
  void set_roles(int index, std::string&& value);
  void set_roles(int index, const char* value);
  void set_roles(int index, const char* value, size_t size);
  std::string* add_roles();
  void add_roles(const std::string& value);
  void add_roles(std::string&& value);
  void add_roles(const char* value);
  void add_roles(const char* value, size_t size);
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>& roles() const;
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>* mutable_roles();
  private:
  const std::string& _internal_roles(int index) const;
  std::string* _internal_add_roles();
  public:

  // bytes name = 1;
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

  // bytes password = 2;
  void clear_password();
  const std::string& password() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_password(ArgT0&& arg0, ArgT... args);
  std::string* mutable_password();
  PROTOBUF_NODISCARD std::string* release_password();
  void set_allocated_password(std::string* password);
  private:
  const std::string& _internal_password() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_password(const std::string& value);
  std::string* _internal_mutable_password();
  public:

  // @@protoc_insertion_point(class_scope:authpb.User)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string> roles_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr password_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_auth_2eproto;
};
// -------------------------------------------------------------------

class Permission final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:authpb.Permission) */ {
 public:
  inline Permission() : Permission(nullptr) {}
  ~Permission() override;
  explicit PROTOBUF_CONSTEXPR Permission(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  Permission(const Permission& from);
  Permission(Permission&& from) noexcept
    : Permission() {
    *this = ::std::move(from);
  }

  inline Permission& operator=(const Permission& from) {
    CopyFrom(from);
    return *this;
  }
  inline Permission& operator=(Permission&& from) noexcept {
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
  static const Permission& default_instance() {
    return *internal_default_instance();
  }
  static inline const Permission* internal_default_instance() {
    return reinterpret_cast<const Permission*>(
               &_Permission_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    1;

  friend void swap(Permission& a, Permission& b) {
    a.Swap(&b);
  }
  inline void Swap(Permission* other) {
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
  void UnsafeArenaSwap(Permission* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  Permission* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<Permission>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const Permission& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const Permission& from) {
    Permission::MergeImpl(*this, from);
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
  void InternalSwap(Permission* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "authpb.Permission";
  }
  protected:
  explicit Permission(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  typedef Permission_Type Type;
  static constexpr Type READ =
    Permission_Type_READ;
  static constexpr Type WRITE =
    Permission_Type_WRITE;
  static constexpr Type READWRITE =
    Permission_Type_READWRITE;
  static inline bool Type_IsValid(int value) {
    return Permission_Type_IsValid(value);
  }
  static constexpr Type Type_MIN =
    Permission_Type_Type_MIN;
  static constexpr Type Type_MAX =
    Permission_Type_Type_MAX;
  static constexpr int Type_ARRAYSIZE =
    Permission_Type_Type_ARRAYSIZE;
  static inline const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor*
  Type_descriptor() {
    return Permission_Type_descriptor();
  }
  template<typename T>
  static inline const std::string& Type_Name(T enum_t_value) {
    static_assert(::std::is_same<T, Type>::value ||
      ::std::is_integral<T>::value,
      "Incorrect type passed to function Type_Name.");
    return Permission_Type_Name(enum_t_value);
  }
  static inline bool Type_Parse(::PROTOBUF_NAMESPACE_ID::ConstStringParam name,
      Type* value) {
    return Permission_Type_Parse(name, value);
  }

  // accessors -------------------------------------------------------

  enum : int {
    kKeyFieldNumber = 2,
    kRangeEndFieldNumber = 3,
    kPermTypeFieldNumber = 1,
  };
  // bytes key = 2;
  void clear_key();
  const std::string& key() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_key(ArgT0&& arg0, ArgT... args);
  std::string* mutable_key();
  PROTOBUF_NODISCARD std::string* release_key();
  void set_allocated_key(std::string* key);
  private:
  const std::string& _internal_key() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_key(const std::string& value);
  std::string* _internal_mutable_key();
  public:

  // bytes range_end = 3;
  void clear_range_end();
  const std::string& range_end() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_range_end(ArgT0&& arg0, ArgT... args);
  std::string* mutable_range_end();
  PROTOBUF_NODISCARD std::string* release_range_end();
  void set_allocated_range_end(std::string* range_end);
  private:
  const std::string& _internal_range_end() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_range_end(const std::string& value);
  std::string* _internal_mutable_range_end();
  public:

  // .authpb.Permission.Type permType = 1;
  void clear_permtype();
  ::authpb::Permission_Type permtype() const;
  void set_permtype(::authpb::Permission_Type value);
  private:
  ::authpb::Permission_Type _internal_permtype() const;
  void _internal_set_permtype(::authpb::Permission_Type value);
  public:

  // @@protoc_insertion_point(class_scope:authpb.Permission)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr key_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr range_end_;
    int permtype_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_auth_2eproto;
};
// -------------------------------------------------------------------

class Role final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:authpb.Role) */ {
 public:
  inline Role() : Role(nullptr) {}
  ~Role() override;
  explicit PROTOBUF_CONSTEXPR Role(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  Role(const Role& from);
  Role(Role&& from) noexcept
    : Role() {
    *this = ::std::move(from);
  }

  inline Role& operator=(const Role& from) {
    CopyFrom(from);
    return *this;
  }
  inline Role& operator=(Role&& from) noexcept {
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
  static const Role& default_instance() {
    return *internal_default_instance();
  }
  static inline const Role* internal_default_instance() {
    return reinterpret_cast<const Role*>(
               &_Role_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    2;

  friend void swap(Role& a, Role& b) {
    a.Swap(&b);
  }
  inline void Swap(Role* other) {
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
  void UnsafeArenaSwap(Role* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  Role* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<Role>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const Role& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const Role& from) {
    Role::MergeImpl(*this, from);
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
  void InternalSwap(Role* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "authpb.Role";
  }
  protected:
  explicit Role(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kKeyPermissionFieldNumber = 2,
    kNameFieldNumber = 1,
  };
  // repeated .authpb.Permission keyPermission = 2;
  int keypermission_size() const;
  private:
  int _internal_keypermission_size() const;
  public:
  void clear_keypermission();
  ::authpb::Permission* mutable_keypermission(int index);
  ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::authpb::Permission >*
      mutable_keypermission();
  private:
  const ::authpb::Permission& _internal_keypermission(int index) const;
  ::authpb::Permission* _internal_add_keypermission();
  public:
  const ::authpb::Permission& keypermission(int index) const;
  ::authpb::Permission* add_keypermission();
  const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::authpb::Permission >&
      keypermission() const;

  // bytes name = 1;
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

  // @@protoc_insertion_point(class_scope:authpb.Role)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::authpb::Permission > keypermission_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_auth_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// User

// bytes name = 1;
inline void User::clear_name() {
  _impl_.name_.ClearToEmpty();
}
inline const std::string& User::name() const {
  // @@protoc_insertion_point(field_get:authpb.User.name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void User::set_name(ArgT0&& arg0, ArgT... args) {
 
 _impl_.name_.SetBytes(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:authpb.User.name)
}
inline std::string* User::mutable_name() {
  std::string* _s = _internal_mutable_name();
  // @@protoc_insertion_point(field_mutable:authpb.User.name)
  return _s;
}
inline const std::string& User::_internal_name() const {
  return _impl_.name_.Get();
}
inline void User::_internal_set_name(const std::string& value) {
  
  _impl_.name_.Set(value, GetArenaForAllocation());
}
inline std::string* User::_internal_mutable_name() {
  
  return _impl_.name_.Mutable(GetArenaForAllocation());
}
inline std::string* User::release_name() {
  // @@protoc_insertion_point(field_release:authpb.User.name)
  return _impl_.name_.Release();
}
inline void User::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    
  } else {
    
  }
  _impl_.name_.SetAllocated(name, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.name_.IsDefault()) {
    _impl_.name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:authpb.User.name)
}

// bytes password = 2;
inline void User::clear_password() {
  _impl_.password_.ClearToEmpty();
}
inline const std::string& User::password() const {
  // @@protoc_insertion_point(field_get:authpb.User.password)
  return _internal_password();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void User::set_password(ArgT0&& arg0, ArgT... args) {
 
 _impl_.password_.SetBytes(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:authpb.User.password)
}
inline std::string* User::mutable_password() {
  std::string* _s = _internal_mutable_password();
  // @@protoc_insertion_point(field_mutable:authpb.User.password)
  return _s;
}
inline const std::string& User::_internal_password() const {
  return _impl_.password_.Get();
}
inline void User::_internal_set_password(const std::string& value) {
  
  _impl_.password_.Set(value, GetArenaForAllocation());
}
inline std::string* User::_internal_mutable_password() {
  
  return _impl_.password_.Mutable(GetArenaForAllocation());
}
inline std::string* User::release_password() {
  // @@protoc_insertion_point(field_release:authpb.User.password)
  return _impl_.password_.Release();
}
inline void User::set_allocated_password(std::string* password) {
  if (password != nullptr) {
    
  } else {
    
  }
  _impl_.password_.SetAllocated(password, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.password_.IsDefault()) {
    _impl_.password_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:authpb.User.password)
}

// repeated string roles = 3;
inline int User::_internal_roles_size() const {
  return _impl_.roles_.size();
}
inline int User::roles_size() const {
  return _internal_roles_size();
}
inline void User::clear_roles() {
  _impl_.roles_.Clear();
}
inline std::string* User::add_roles() {
  std::string* _s = _internal_add_roles();
  // @@protoc_insertion_point(field_add_mutable:authpb.User.roles)
  return _s;
}
inline const std::string& User::_internal_roles(int index) const {
  return _impl_.roles_.Get(index);
}
inline const std::string& User::roles(int index) const {
  // @@protoc_insertion_point(field_get:authpb.User.roles)
  return _internal_roles(index);
}
inline std::string* User::mutable_roles(int index) {
  // @@protoc_insertion_point(field_mutable:authpb.User.roles)
  return _impl_.roles_.Mutable(index);
}
inline void User::set_roles(int index, const std::string& value) {
  _impl_.roles_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set:authpb.User.roles)
}
inline void User::set_roles(int index, std::string&& value) {
  _impl_.roles_.Mutable(index)->assign(std::move(value));
  // @@protoc_insertion_point(field_set:authpb.User.roles)
}
inline void User::set_roles(int index, const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.roles_.Mutable(index)->assign(value);
  // @@protoc_insertion_point(field_set_char:authpb.User.roles)
}
inline void User::set_roles(int index, const char* value, size_t size) {
  _impl_.roles_.Mutable(index)->assign(
    reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_set_pointer:authpb.User.roles)
}
inline std::string* User::_internal_add_roles() {
  return _impl_.roles_.Add();
}
inline void User::add_roles(const std::string& value) {
  _impl_.roles_.Add()->assign(value);
  // @@protoc_insertion_point(field_add:authpb.User.roles)
}
inline void User::add_roles(std::string&& value) {
  _impl_.roles_.Add(std::move(value));
  // @@protoc_insertion_point(field_add:authpb.User.roles)
}
inline void User::add_roles(const char* value) {
  GOOGLE_DCHECK(value != nullptr);
  _impl_.roles_.Add()->assign(value);
  // @@protoc_insertion_point(field_add_char:authpb.User.roles)
}
inline void User::add_roles(const char* value, size_t size) {
  _impl_.roles_.Add()->assign(reinterpret_cast<const char*>(value), size);
  // @@protoc_insertion_point(field_add_pointer:authpb.User.roles)
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>&
User::roles() const {
  // @@protoc_insertion_point(field_list:authpb.User.roles)
  return _impl_.roles_;
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField<std::string>*
User::mutable_roles() {
  // @@protoc_insertion_point(field_mutable_list:authpb.User.roles)
  return &_impl_.roles_;
}

// -------------------------------------------------------------------

// Permission

// .authpb.Permission.Type permType = 1;
inline void Permission::clear_permtype() {
  _impl_.permtype_ = 0;
}
inline ::authpb::Permission_Type Permission::_internal_permtype() const {
  return static_cast< ::authpb::Permission_Type >(_impl_.permtype_);
}
inline ::authpb::Permission_Type Permission::permtype() const {
  // @@protoc_insertion_point(field_get:authpb.Permission.permType)
  return _internal_permtype();
}
inline void Permission::_internal_set_permtype(::authpb::Permission_Type value) {
  
  _impl_.permtype_ = value;
}
inline void Permission::set_permtype(::authpb::Permission_Type value) {
  _internal_set_permtype(value);
  // @@protoc_insertion_point(field_set:authpb.Permission.permType)
}

// bytes key = 2;
inline void Permission::clear_key() {
  _impl_.key_.ClearToEmpty();
}
inline const std::string& Permission::key() const {
  // @@protoc_insertion_point(field_get:authpb.Permission.key)
  return _internal_key();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void Permission::set_key(ArgT0&& arg0, ArgT... args) {
 
 _impl_.key_.SetBytes(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:authpb.Permission.key)
}
inline std::string* Permission::mutable_key() {
  std::string* _s = _internal_mutable_key();
  // @@protoc_insertion_point(field_mutable:authpb.Permission.key)
  return _s;
}
inline const std::string& Permission::_internal_key() const {
  return _impl_.key_.Get();
}
inline void Permission::_internal_set_key(const std::string& value) {
  
  _impl_.key_.Set(value, GetArenaForAllocation());
}
inline std::string* Permission::_internal_mutable_key() {
  
  return _impl_.key_.Mutable(GetArenaForAllocation());
}
inline std::string* Permission::release_key() {
  // @@protoc_insertion_point(field_release:authpb.Permission.key)
  return _impl_.key_.Release();
}
inline void Permission::set_allocated_key(std::string* key) {
  if (key != nullptr) {
    
  } else {
    
  }
  _impl_.key_.SetAllocated(key, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.key_.IsDefault()) {
    _impl_.key_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:authpb.Permission.key)
}

// bytes range_end = 3;
inline void Permission::clear_range_end() {
  _impl_.range_end_.ClearToEmpty();
}
inline const std::string& Permission::range_end() const {
  // @@protoc_insertion_point(field_get:authpb.Permission.range_end)
  return _internal_range_end();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void Permission::set_range_end(ArgT0&& arg0, ArgT... args) {
 
 _impl_.range_end_.SetBytes(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:authpb.Permission.range_end)
}
inline std::string* Permission::mutable_range_end() {
  std::string* _s = _internal_mutable_range_end();
  // @@protoc_insertion_point(field_mutable:authpb.Permission.range_end)
  return _s;
}
inline const std::string& Permission::_internal_range_end() const {
  return _impl_.range_end_.Get();
}
inline void Permission::_internal_set_range_end(const std::string& value) {
  
  _impl_.range_end_.Set(value, GetArenaForAllocation());
}
inline std::string* Permission::_internal_mutable_range_end() {
  
  return _impl_.range_end_.Mutable(GetArenaForAllocation());
}
inline std::string* Permission::release_range_end() {
  // @@protoc_insertion_point(field_release:authpb.Permission.range_end)
  return _impl_.range_end_.Release();
}
inline void Permission::set_allocated_range_end(std::string* range_end) {
  if (range_end != nullptr) {
    
  } else {
    
  }
  _impl_.range_end_.SetAllocated(range_end, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.range_end_.IsDefault()) {
    _impl_.range_end_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:authpb.Permission.range_end)
}

// -------------------------------------------------------------------

// Role

// bytes name = 1;
inline void Role::clear_name() {
  _impl_.name_.ClearToEmpty();
}
inline const std::string& Role::name() const {
  // @@protoc_insertion_point(field_get:authpb.Role.name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void Role::set_name(ArgT0&& arg0, ArgT... args) {
 
 _impl_.name_.SetBytes(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:authpb.Role.name)
}
inline std::string* Role::mutable_name() {
  std::string* _s = _internal_mutable_name();
  // @@protoc_insertion_point(field_mutable:authpb.Role.name)
  return _s;
}
inline const std::string& Role::_internal_name() const {
  return _impl_.name_.Get();
}
inline void Role::_internal_set_name(const std::string& value) {
  
  _impl_.name_.Set(value, GetArenaForAllocation());
}
inline std::string* Role::_internal_mutable_name() {
  
  return _impl_.name_.Mutable(GetArenaForAllocation());
}
inline std::string* Role::release_name() {
  // @@protoc_insertion_point(field_release:authpb.Role.name)
  return _impl_.name_.Release();
}
inline void Role::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    
  } else {
    
  }
  _impl_.name_.SetAllocated(name, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.name_.IsDefault()) {
    _impl_.name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:authpb.Role.name)
}

// repeated .authpb.Permission keyPermission = 2;
inline int Role::_internal_keypermission_size() const {
  return _impl_.keypermission_.size();
}
inline int Role::keypermission_size() const {
  return _internal_keypermission_size();
}
inline void Role::clear_keypermission() {
  _impl_.keypermission_.Clear();
}
inline ::authpb::Permission* Role::mutable_keypermission(int index) {
  // @@protoc_insertion_point(field_mutable:authpb.Role.keyPermission)
  return _impl_.keypermission_.Mutable(index);
}
inline ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::authpb::Permission >*
Role::mutable_keypermission() {
  // @@protoc_insertion_point(field_mutable_list:authpb.Role.keyPermission)
  return &_impl_.keypermission_;
}
inline const ::authpb::Permission& Role::_internal_keypermission(int index) const {
  return _impl_.keypermission_.Get(index);
}
inline const ::authpb::Permission& Role::keypermission(int index) const {
  // @@protoc_insertion_point(field_get:authpb.Role.keyPermission)
  return _internal_keypermission(index);
}
inline ::authpb::Permission* Role::_internal_add_keypermission() {
  return _impl_.keypermission_.Add();
}
inline ::authpb::Permission* Role::add_keypermission() {
  ::authpb::Permission* _add = _internal_add_keypermission();
  // @@protoc_insertion_point(field_add:authpb.Role.keyPermission)
  return _add;
}
inline const ::PROTOBUF_NAMESPACE_ID::RepeatedPtrField< ::authpb::Permission >&
Role::keypermission() const {
  // @@protoc_insertion_point(field_list:authpb.Role.keyPermission)
  return _impl_.keypermission_;
}

#ifdef __GNUC__
  #pragma GCC diagnostic pop
#endif  // __GNUC__
// -------------------------------------------------------------------

// -------------------------------------------------------------------


// @@protoc_insertion_point(namespace_scope)

}  // namespace authpb

PROTOBUF_NAMESPACE_OPEN

template <> struct is_proto_enum< ::authpb::Permission_Type> : ::std::true_type {};
template <>
inline const EnumDescriptor* GetEnumDescriptor< ::authpb::Permission_Type>() {
  return ::authpb::Permission_Type_descriptor();
}

PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_auth_2eproto
