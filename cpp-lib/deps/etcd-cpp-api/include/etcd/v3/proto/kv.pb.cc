// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: kv.proto

#include "kv.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace mvccpb {
PROTOBUF_CONSTEXPR KeyValue::KeyValue(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.key_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.value_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.create_revision_)*/int64_t{0}
  , /*decltype(_impl_.mod_revision_)*/int64_t{0}
  , /*decltype(_impl_.version_)*/int64_t{0}
  , /*decltype(_impl_.lease_)*/int64_t{0}
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct KeyValueDefaultTypeInternal {
  PROTOBUF_CONSTEXPR KeyValueDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~KeyValueDefaultTypeInternal() {}
  union {
    KeyValue _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 KeyValueDefaultTypeInternal _KeyValue_default_instance_;
PROTOBUF_CONSTEXPR Event::Event(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.kv_)*/nullptr
  , /*decltype(_impl_.prev_kv_)*/nullptr
  , /*decltype(_impl_.type_)*/0
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct EventDefaultTypeInternal {
  PROTOBUF_CONSTEXPR EventDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~EventDefaultTypeInternal() {}
  union {
    Event _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 EventDefaultTypeInternal _Event_default_instance_;
}  // namespace mvccpb
static ::_pb::Metadata file_level_metadata_kv_2eproto[2];
static const ::_pb::EnumDescriptor* file_level_enum_descriptors_kv_2eproto[1];
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_kv_2eproto = nullptr;

const uint32_t TableStruct_kv_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::mvccpb::KeyValue, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::mvccpb::KeyValue, _impl_.key_),
  PROTOBUF_FIELD_OFFSET(::mvccpb::KeyValue, _impl_.create_revision_),
  PROTOBUF_FIELD_OFFSET(::mvccpb::KeyValue, _impl_.mod_revision_),
  PROTOBUF_FIELD_OFFSET(::mvccpb::KeyValue, _impl_.version_),
  PROTOBUF_FIELD_OFFSET(::mvccpb::KeyValue, _impl_.value_),
  PROTOBUF_FIELD_OFFSET(::mvccpb::KeyValue, _impl_.lease_),
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::mvccpb::Event, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::mvccpb::Event, _impl_.type_),
  PROTOBUF_FIELD_OFFSET(::mvccpb::Event, _impl_.kv_),
  PROTOBUF_FIELD_OFFSET(::mvccpb::Event, _impl_.prev_kv_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::mvccpb::KeyValue)},
  { 12, -1, -1, sizeof(::mvccpb::Event)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::mvccpb::_KeyValue_default_instance_._instance,
  &::mvccpb::_Event_default_instance_._instance,
};

const char descriptor_table_protodef_kv_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\010kv.proto\022\006mvccpb\"u\n\010KeyValue\022\013\n\003key\030\001 "
  "\001(\014\022\027\n\017create_revision\030\002 \001(\003\022\024\n\014mod_revi"
  "sion\030\003 \001(\003\022\017\n\007version\030\004 \001(\003\022\r\n\005value\030\005 \001"
  "(\014\022\r\n\005lease\030\006 \001(\003\"\221\001\n\005Event\022%\n\004type\030\001 \001("
  "\0162\027.mvccpb.Event.EventType\022\034\n\002kv\030\002 \001(\0132\020"
  ".mvccpb.KeyValue\022!\n\007prev_kv\030\003 \001(\0132\020.mvcc"
  "pb.KeyValue\" \n\tEventType\022\007\n\003PUT\020\000\022\n\n\006DEL"
  "ETE\020\001b\006proto3"
  ;
static ::_pbi::once_flag descriptor_table_kv_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_kv_2eproto = {
    false, false, 293, descriptor_table_protodef_kv_2eproto,
    "kv.proto",
    &descriptor_table_kv_2eproto_once, nullptr, 0, 2,
    schemas, file_default_instances, TableStruct_kv_2eproto::offsets,
    file_level_metadata_kv_2eproto, file_level_enum_descriptors_kv_2eproto,
    file_level_service_descriptors_kv_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_kv_2eproto_getter() {
  return &descriptor_table_kv_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_kv_2eproto(&descriptor_table_kv_2eproto);
namespace mvccpb {
const ::PROTOBUF_NAMESPACE_ID::EnumDescriptor* Event_EventType_descriptor() {
  ::PROTOBUF_NAMESPACE_ID::internal::AssignDescriptors(&descriptor_table_kv_2eproto);
  return file_level_enum_descriptors_kv_2eproto[0];
}
bool Event_EventType_IsValid(int value) {
  switch (value) {
    case 0:
    case 1:
      return true;
    default:
      return false;
  }
}

#if (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))
constexpr Event_EventType Event::PUT;
constexpr Event_EventType Event::DELETE;
constexpr Event_EventType Event::EventType_MIN;
constexpr Event_EventType Event::EventType_MAX;
constexpr int Event::EventType_ARRAYSIZE;
#endif  // (__cplusplus < 201703) && (!defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912))

// ===================================================================

class KeyValue::_Internal {
 public:
};

KeyValue::KeyValue(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:mvccpb.KeyValue)
}
KeyValue::KeyValue(const KeyValue& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  KeyValue* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.key_){}
    , decltype(_impl_.value_){}
    , decltype(_impl_.create_revision_){}
    , decltype(_impl_.mod_revision_){}
    , decltype(_impl_.version_){}
    , decltype(_impl_.lease_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _impl_.key_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.key_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_key().empty()) {
    _this->_impl_.key_.Set(from._internal_key(), 
      _this->GetArenaForAllocation());
  }
  _impl_.value_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.value_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_value().empty()) {
    _this->_impl_.value_.Set(from._internal_value(), 
      _this->GetArenaForAllocation());
  }
  ::memcpy(&_impl_.create_revision_, &from._impl_.create_revision_,
    static_cast<size_t>(reinterpret_cast<char*>(&_impl_.lease_) -
    reinterpret_cast<char*>(&_impl_.create_revision_)) + sizeof(_impl_.lease_));
  // @@protoc_insertion_point(copy_constructor:mvccpb.KeyValue)
}

inline void KeyValue::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.key_){}
    , decltype(_impl_.value_){}
    , decltype(_impl_.create_revision_){int64_t{0}}
    , decltype(_impl_.mod_revision_){int64_t{0}}
    , decltype(_impl_.version_){int64_t{0}}
    , decltype(_impl_.lease_){int64_t{0}}
    , /*decltype(_impl_._cached_size_)*/{}
  };
  _impl_.key_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.key_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  _impl_.value_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.value_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

KeyValue::~KeyValue() {
  // @@protoc_insertion_point(destructor:mvccpb.KeyValue)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void KeyValue::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.key_.Destroy();
  _impl_.value_.Destroy();
}

void KeyValue::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void KeyValue::Clear() {
// @@protoc_insertion_point(message_clear_start:mvccpb.KeyValue)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.key_.ClearToEmpty();
  _impl_.value_.ClearToEmpty();
  ::memset(&_impl_.create_revision_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&_impl_.lease_) -
      reinterpret_cast<char*>(&_impl_.create_revision_)) + sizeof(_impl_.lease_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* KeyValue::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // bytes key = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_key();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int64 create_revision = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          _impl_.create_revision_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int64 mod_revision = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 24)) {
          _impl_.mod_revision_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int64 version = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 32)) {
          _impl_.version_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // bytes value = 5;
      case 5:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 42)) {
          auto str = _internal_mutable_value();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // int64 lease = 6;
      case 6:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 48)) {
          _impl_.lease_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* KeyValue::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:mvccpb.KeyValue)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // bytes key = 1;
  if (!this->_internal_key().empty()) {
    target = stream->WriteBytesMaybeAliased(
        1, this->_internal_key(), target);
  }

  // int64 create_revision = 2;
  if (this->_internal_create_revision() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt64ToArray(2, this->_internal_create_revision(), target);
  }

  // int64 mod_revision = 3;
  if (this->_internal_mod_revision() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt64ToArray(3, this->_internal_mod_revision(), target);
  }

  // int64 version = 4;
  if (this->_internal_version() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt64ToArray(4, this->_internal_version(), target);
  }

  // bytes value = 5;
  if (!this->_internal_value().empty()) {
    target = stream->WriteBytesMaybeAliased(
        5, this->_internal_value(), target);
  }

  // int64 lease = 6;
  if (this->_internal_lease() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt64ToArray(6, this->_internal_lease(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:mvccpb.KeyValue)
  return target;
}

size_t KeyValue::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:mvccpb.KeyValue)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // bytes key = 1;
  if (!this->_internal_key().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::BytesSize(
        this->_internal_key());
  }

  // bytes value = 5;
  if (!this->_internal_value().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::BytesSize(
        this->_internal_value());
  }

  // int64 create_revision = 2;
  if (this->_internal_create_revision() != 0) {
    total_size += ::_pbi::WireFormatLite::Int64SizePlusOne(this->_internal_create_revision());
  }

  // int64 mod_revision = 3;
  if (this->_internal_mod_revision() != 0) {
    total_size += ::_pbi::WireFormatLite::Int64SizePlusOne(this->_internal_mod_revision());
  }

  // int64 version = 4;
  if (this->_internal_version() != 0) {
    total_size += ::_pbi::WireFormatLite::Int64SizePlusOne(this->_internal_version());
  }

  // int64 lease = 6;
  if (this->_internal_lease() != 0) {
    total_size += ::_pbi::WireFormatLite::Int64SizePlusOne(this->_internal_lease());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData KeyValue::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    KeyValue::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*KeyValue::GetClassData() const { return &_class_data_; }


void KeyValue::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<KeyValue*>(&to_msg);
  auto& from = static_cast<const KeyValue&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:mvccpb.KeyValue)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_key().empty()) {
    _this->_internal_set_key(from._internal_key());
  }
  if (!from._internal_value().empty()) {
    _this->_internal_set_value(from._internal_value());
  }
  if (from._internal_create_revision() != 0) {
    _this->_internal_set_create_revision(from._internal_create_revision());
  }
  if (from._internal_mod_revision() != 0) {
    _this->_internal_set_mod_revision(from._internal_mod_revision());
  }
  if (from._internal_version() != 0) {
    _this->_internal_set_version(from._internal_version());
  }
  if (from._internal_lease() != 0) {
    _this->_internal_set_lease(from._internal_lease());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void KeyValue::CopyFrom(const KeyValue& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:mvccpb.KeyValue)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool KeyValue::IsInitialized() const {
  return true;
}

void KeyValue::InternalSwap(KeyValue* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.key_, lhs_arena,
      &other->_impl_.key_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.value_, lhs_arena,
      &other->_impl_.value_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(KeyValue, _impl_.lease_)
      + sizeof(KeyValue::_impl_.lease_)
      - PROTOBUF_FIELD_OFFSET(KeyValue, _impl_.create_revision_)>(
          reinterpret_cast<char*>(&_impl_.create_revision_),
          reinterpret_cast<char*>(&other->_impl_.create_revision_));
}

::PROTOBUF_NAMESPACE_ID::Metadata KeyValue::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_kv_2eproto_getter, &descriptor_table_kv_2eproto_once,
      file_level_metadata_kv_2eproto[0]);
}

// ===================================================================

class Event::_Internal {
 public:
  static const ::mvccpb::KeyValue& kv(const Event* msg);
  static const ::mvccpb::KeyValue& prev_kv(const Event* msg);
};

const ::mvccpb::KeyValue&
Event::_Internal::kv(const Event* msg) {
  return *msg->_impl_.kv_;
}
const ::mvccpb::KeyValue&
Event::_Internal::prev_kv(const Event* msg) {
  return *msg->_impl_.prev_kv_;
}
Event::Event(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:mvccpb.Event)
}
Event::Event(const Event& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  Event* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.kv_){nullptr}
    , decltype(_impl_.prev_kv_){nullptr}
    , decltype(_impl_.type_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  if (from._internal_has_kv()) {
    _this->_impl_.kv_ = new ::mvccpb::KeyValue(*from._impl_.kv_);
  }
  if (from._internal_has_prev_kv()) {
    _this->_impl_.prev_kv_ = new ::mvccpb::KeyValue(*from._impl_.prev_kv_);
  }
  _this->_impl_.type_ = from._impl_.type_;
  // @@protoc_insertion_point(copy_constructor:mvccpb.Event)
}

inline void Event::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.kv_){nullptr}
    , decltype(_impl_.prev_kv_){nullptr}
    , decltype(_impl_.type_){0}
    , /*decltype(_impl_._cached_size_)*/{}
  };
}

Event::~Event() {
  // @@protoc_insertion_point(destructor:mvccpb.Event)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void Event::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  if (this != internal_default_instance()) delete _impl_.kv_;
  if (this != internal_default_instance()) delete _impl_.prev_kv_;
}

void Event::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void Event::Clear() {
// @@protoc_insertion_point(message_clear_start:mvccpb.Event)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  if (GetArenaForAllocation() == nullptr && _impl_.kv_ != nullptr) {
    delete _impl_.kv_;
  }
  _impl_.kv_ = nullptr;
  if (GetArenaForAllocation() == nullptr && _impl_.prev_kv_ != nullptr) {
    delete _impl_.prev_kv_;
  }
  _impl_.prev_kv_ = nullptr;
  _impl_.type_ = 0;
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* Event::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // .mvccpb.Event.EventType type = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 8)) {
          uint64_t val = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
          _internal_set_type(static_cast<::mvccpb::Event_EventType>(val));
        } else
          goto handle_unusual;
        continue;
      // .mvccpb.KeyValue kv = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          ptr = ctx->ParseMessage(_internal_mutable_kv(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // .mvccpb.KeyValue prev_kv = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 26)) {
          ptr = ctx->ParseMessage(_internal_mutable_prev_kv(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* Event::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:mvccpb.Event)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // .mvccpb.Event.EventType type = 1;
  if (this->_internal_type() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteEnumToArray(
      1, this->_internal_type(), target);
  }

  // .mvccpb.KeyValue kv = 2;
  if (this->_internal_has_kv()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(2, _Internal::kv(this),
        _Internal::kv(this).GetCachedSize(), target, stream);
  }

  // .mvccpb.KeyValue prev_kv = 3;
  if (this->_internal_has_prev_kv()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(3, _Internal::prev_kv(this),
        _Internal::prev_kv(this).GetCachedSize(), target, stream);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:mvccpb.Event)
  return target;
}

size_t Event::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:mvccpb.Event)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // .mvccpb.KeyValue kv = 2;
  if (this->_internal_has_kv()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
        *_impl_.kv_);
  }

  // .mvccpb.KeyValue prev_kv = 3;
  if (this->_internal_has_prev_kv()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
        *_impl_.prev_kv_);
  }

  // .mvccpb.Event.EventType type = 1;
  if (this->_internal_type() != 0) {
    total_size += 1 +
      ::_pbi::WireFormatLite::EnumSize(this->_internal_type());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData Event::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    Event::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*Event::GetClassData() const { return &_class_data_; }


void Event::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<Event*>(&to_msg);
  auto& from = static_cast<const Event&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:mvccpb.Event)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_has_kv()) {
    _this->_internal_mutable_kv()->::mvccpb::KeyValue::MergeFrom(
        from._internal_kv());
  }
  if (from._internal_has_prev_kv()) {
    _this->_internal_mutable_prev_kv()->::mvccpb::KeyValue::MergeFrom(
        from._internal_prev_kv());
  }
  if (from._internal_type() != 0) {
    _this->_internal_set_type(from._internal_type());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void Event::CopyFrom(const Event& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:mvccpb.Event)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Event::IsInitialized() const {
  return true;
}

void Event::InternalSwap(Event* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(Event, _impl_.type_)
      + sizeof(Event::_impl_.type_)
      - PROTOBUF_FIELD_OFFSET(Event, _impl_.kv_)>(
          reinterpret_cast<char*>(&_impl_.kv_),
          reinterpret_cast<char*>(&other->_impl_.kv_));
}

::PROTOBUF_NAMESPACE_ID::Metadata Event::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_kv_2eproto_getter, &descriptor_table_kv_2eproto_once,
      file_level_metadata_kv_2eproto[1]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace mvccpb
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::mvccpb::KeyValue*
Arena::CreateMaybeMessage< ::mvccpb::KeyValue >(Arena* arena) {
  return Arena::CreateMessageInternal< ::mvccpb::KeyValue >(arena);
}
template<> PROTOBUF_NOINLINE ::mvccpb::Event*
Arena::CreateMaybeMessage< ::mvccpb::Event >(Arena* arena) {
  return Arena::CreateMessageInternal< ::mvccpb::Event >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
