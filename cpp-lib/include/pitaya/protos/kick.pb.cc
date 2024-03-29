// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: kick.proto

#include "kick.pb.h"

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

namespace protos {
PROTOBUF_CONSTEXPR KickMsg::KickMsg(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.userid_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct KickMsgDefaultTypeInternal {
  PROTOBUF_CONSTEXPR KickMsgDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~KickMsgDefaultTypeInternal() {}
  union {
    KickMsg _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 KickMsgDefaultTypeInternal _KickMsg_default_instance_;
PROTOBUF_CONSTEXPR KickAnswer::KickAnswer(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.kicked_)*/false
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct KickAnswerDefaultTypeInternal {
  PROTOBUF_CONSTEXPR KickAnswerDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~KickAnswerDefaultTypeInternal() {}
  union {
    KickAnswer _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 KickAnswerDefaultTypeInternal _KickAnswer_default_instance_;
}  // namespace protos
static ::_pb::Metadata file_level_metadata_kick_2eproto[2];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_kick_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_kick_2eproto = nullptr;

const uint32_t TableStruct_kick_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::protos::KickMsg, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::protos::KickMsg, _impl_.userid_),
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::protos::KickAnswer, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::protos::KickAnswer, _impl_.kicked_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::protos::KickMsg)},
  { 7, -1, -1, sizeof(::protos::KickAnswer)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::protos::_KickMsg_default_instance_._instance,
  &::protos::_KickAnswer_default_instance_._instance,
};

const char descriptor_table_protodef_kick_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\nkick.proto\022\006protos\"\031\n\007KickMsg\022\016\n\006userI"
  "d\030\001 \001(\t\"\034\n\nKickAnswer\022\016\n\006kicked\030\001 \001(\010B<Z"
  ")github.com/topfreegames/pitaya/pkg/prot"
  "os\252\002\016NPitaya.Protosb\006proto3"
  ;
static ::_pbi::once_flag descriptor_table_kick_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_kick_2eproto = {
    false, false, 147, descriptor_table_protodef_kick_2eproto,
    "kick.proto",
    &descriptor_table_kick_2eproto_once, nullptr, 0, 2,
    schemas, file_default_instances, TableStruct_kick_2eproto::offsets,
    file_level_metadata_kick_2eproto, file_level_enum_descriptors_kick_2eproto,
    file_level_service_descriptors_kick_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_kick_2eproto_getter() {
  return &descriptor_table_kick_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_kick_2eproto(&descriptor_table_kick_2eproto);
namespace protos {

// ===================================================================

class KickMsg::_Internal {
 public:
};

KickMsg::KickMsg(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:protos.KickMsg)
}
KickMsg::KickMsg(const KickMsg& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  KickMsg* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.userid_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _impl_.userid_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.userid_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_userid().empty()) {
    _this->_impl_.userid_.Set(from._internal_userid(), 
      _this->GetArenaForAllocation());
  }
  // @@protoc_insertion_point(copy_constructor:protos.KickMsg)
}

inline void KickMsg::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.userid_){}
    , /*decltype(_impl_._cached_size_)*/{}
  };
  _impl_.userid_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.userid_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

KickMsg::~KickMsg() {
  // @@protoc_insertion_point(destructor:protos.KickMsg)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void KickMsg::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.userid_.Destroy();
}

void KickMsg::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void KickMsg::Clear() {
// @@protoc_insertion_point(message_clear_start:protos.KickMsg)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.userid_.ClearToEmpty();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* KickMsg::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // string userId = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_userid();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "protos.KickMsg.userId"));
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

uint8_t* KickMsg::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:protos.KickMsg)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // string userId = 1;
  if (!this->_internal_userid().empty()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_userid().data(), static_cast<int>(this->_internal_userid().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "protos.KickMsg.userId");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_userid(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:protos.KickMsg)
  return target;
}

size_t KickMsg::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:protos.KickMsg)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string userId = 1;
  if (!this->_internal_userid().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_userid());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData KickMsg::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    KickMsg::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*KickMsg::GetClassData() const { return &_class_data_; }


void KickMsg::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<KickMsg*>(&to_msg);
  auto& from = static_cast<const KickMsg&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:protos.KickMsg)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_userid().empty()) {
    _this->_internal_set_userid(from._internal_userid());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void KickMsg::CopyFrom(const KickMsg& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:protos.KickMsg)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool KickMsg::IsInitialized() const {
  return true;
}

void KickMsg::InternalSwap(KickMsg* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.userid_, lhs_arena,
      &other->_impl_.userid_, rhs_arena
  );
}

::PROTOBUF_NAMESPACE_ID::Metadata KickMsg::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_kick_2eproto_getter, &descriptor_table_kick_2eproto_once,
      file_level_metadata_kick_2eproto[0]);
}

// ===================================================================

class KickAnswer::_Internal {
 public:
};

KickAnswer::KickAnswer(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:protos.KickAnswer)
}
KickAnswer::KickAnswer(const KickAnswer& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  KickAnswer* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.kicked_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _this->_impl_.kicked_ = from._impl_.kicked_;
  // @@protoc_insertion_point(copy_constructor:protos.KickAnswer)
}

inline void KickAnswer::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.kicked_){false}
    , /*decltype(_impl_._cached_size_)*/{}
  };
}

KickAnswer::~KickAnswer() {
  // @@protoc_insertion_point(destructor:protos.KickAnswer)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void KickAnswer::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void KickAnswer::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void KickAnswer::Clear() {
// @@protoc_insertion_point(message_clear_start:protos.KickAnswer)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.kicked_ = false;
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* KickAnswer::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // bool kicked = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 8)) {
          _impl_.kicked_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
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

uint8_t* KickAnswer::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:protos.KickAnswer)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // bool kicked = 1;
  if (this->_internal_kicked() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteBoolToArray(1, this->_internal_kicked(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:protos.KickAnswer)
  return target;
}

size_t KickAnswer::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:protos.KickAnswer)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // bool kicked = 1;
  if (this->_internal_kicked() != 0) {
    total_size += 1 + 1;
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData KickAnswer::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    KickAnswer::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*KickAnswer::GetClassData() const { return &_class_data_; }


void KickAnswer::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<KickAnswer*>(&to_msg);
  auto& from = static_cast<const KickAnswer&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:protos.KickAnswer)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_kicked() != 0) {
    _this->_internal_set_kicked(from._internal_kicked());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void KickAnswer::CopyFrom(const KickAnswer& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:protos.KickAnswer)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool KickAnswer::IsInitialized() const {
  return true;
}

void KickAnswer::InternalSwap(KickAnswer* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_.kicked_, other->_impl_.kicked_);
}

::PROTOBUF_NAMESPACE_ID::Metadata KickAnswer::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_kick_2eproto_getter, &descriptor_table_kick_2eproto_once,
      file_level_metadata_kick_2eproto[1]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace protos
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::protos::KickMsg*
Arena::CreateMaybeMessage< ::protos::KickMsg >(Arena* arena) {
  return Arena::CreateMessageInternal< ::protos::KickMsg >(arena);
}
template<> PROTOBUF_NOINLINE ::protos::KickAnswer*
Arena::CreateMaybeMessage< ::protos::KickAnswer >(Arena* arena) {
  return Arena::CreateMessageInternal< ::protos::KickAnswer >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
