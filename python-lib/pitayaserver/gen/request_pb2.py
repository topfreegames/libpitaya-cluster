# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: request.proto
"""Generated protocol buffer code."""
from google.protobuf.internal import enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import descriptor_pool as _descriptor_pool
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from . import session_pb2 as session__pb2
from . import msg_pb2 as msg__pb2


DESCRIPTOR = _descriptor_pool.Default().AddSerializedFile(b'\n\rrequest.proto\x12\x06protos\x1a\rsession.proto\x1a\tmsg.proto\"\x8a\x01\n\x07Request\x12\x1d\n\x04type\x18\x01 \x01(\x0e\x32\x0f.protos.RPCType\x12 \n\x07session\x18\x02 \x01(\x0b\x32\x0f.protos.Session\x12\x18\n\x03msg\x18\x03 \x01(\x0b\x32\x0b.protos.Msg\x12\x12\n\nfrontendID\x18\x04 \x01(\t\x12\x10\n\x08metadata\x18\x05 \x01(\x0c*\x1c\n\x07RPCType\x12\x07\n\x03Sys\x10\x00\x12\x08\n\x04User\x10\x01\x42<Z)github.com/topfreegames/pitaya/pkg/protos\xaa\x02\x0eNPitaya.Protosb\x06proto3')

_RPCTYPE = DESCRIPTOR.enum_types_by_name['RPCType']
RPCType = enum_type_wrapper.EnumTypeWrapper(_RPCTYPE)
Sys = 0
User = 1


_REQUEST = DESCRIPTOR.message_types_by_name['Request']
Request = _reflection.GeneratedProtocolMessageType('Request', (_message.Message,), {
  'DESCRIPTOR' : _REQUEST,
  '__module__' : 'request_pb2'
  # @@protoc_insertion_point(class_scope:protos.Request)
  })
_sym_db.RegisterMessage(Request)

if _descriptor._USE_C_DESCRIPTORS == False:

  DESCRIPTOR._options = None
  DESCRIPTOR._serialized_options = b'Z)github.com/topfreegames/pitaya/pkg/protos\252\002\016NPitaya.Protos'
  _RPCTYPE._serialized_start=192
  _RPCTYPE._serialized_end=220
  _REQUEST._serialized_start=52
  _REQUEST._serialized_end=190
# @@protoc_insertion_point(module_scope)
