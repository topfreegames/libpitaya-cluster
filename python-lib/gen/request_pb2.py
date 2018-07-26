# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: request.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf.internal import enum_type_wrapper
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import session_pb2 as session__pb2
import msg_pb2 as msg__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='request.proto',
  package='protos',
  syntax='proto3',
  serialized_options=None,
  serialized_pb=_b('\n\rrequest.proto\x12\x06protos\x1a\rsession.proto\x1a\tmsg.proto\"\x8a\x01\n\x07Request\x12\x1d\n\x04type\x18\x01 \x01(\x0e\x32\x0f.protos.RPCType\x12 \n\x07session\x18\x02 \x01(\x0b\x32\x0f.protos.Session\x12\x18\n\x03msg\x18\x03 \x01(\x0b\x32\x0b.protos.Msg\x12\x12\n\nfrontendID\x18\x04 \x01(\t\x12\x10\n\x08metadata\x18\x05 \x01(\x0c*\x1c\n\x07RPCType\x12\x07\n\x03Sys\x10\x00\x12\x08\n\x04User\x10\x01\x62\x06proto3')
  ,
  dependencies=[session__pb2.DESCRIPTOR,msg__pb2.DESCRIPTOR,])

_RPCTYPE = _descriptor.EnumDescriptor(
  name='RPCType',
  full_name='protos.RPCType',
  filename=None,
  file=DESCRIPTOR,
  values=[
    _descriptor.EnumValueDescriptor(
      name='Sys', index=0, number=0,
      serialized_options=None,
      type=None),
    _descriptor.EnumValueDescriptor(
      name='User', index=1, number=1,
      serialized_options=None,
      type=None),
  ],
  containing_type=None,
  serialized_options=None,
  serialized_start=192,
  serialized_end=220,
)
_sym_db.RegisterEnumDescriptor(_RPCTYPE)

RPCType = enum_type_wrapper.EnumTypeWrapper(_RPCTYPE)
Sys = 0
User = 1



_REQUEST = _descriptor.Descriptor(
  name='Request',
  full_name='protos.Request',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='type', full_name='protos.Request.type', index=0,
      number=1, type=14, cpp_type=8, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='session', full_name='protos.Request.session', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='msg', full_name='protos.Request.msg', index=2,
      number=3, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='frontendID', full_name='protos.Request.frontendID', index=3,
      number=4, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
    _descriptor.FieldDescriptor(
      name='metadata', full_name='protos.Request.metadata', index=4,
      number=5, type=12, cpp_type=9, label=1,
      has_default_value=False, default_value=_b(""),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=52,
  serialized_end=190,
)

_REQUEST.fields_by_name['type'].enum_type = _RPCTYPE
_REQUEST.fields_by_name['session'].message_type = session__pb2._SESSION
_REQUEST.fields_by_name['msg'].message_type = msg__pb2._MSG
DESCRIPTOR.message_types_by_name['Request'] = _REQUEST
DESCRIPTOR.enum_types_by_name['RPCType'] = _RPCTYPE
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

Request = _reflection.GeneratedProtocolMessageType('Request', (_message.Message,), dict(
  DESCRIPTOR = _REQUEST,
  __module__ = 'request_pb2'
  # @@protoc_insertion_point(class_scope:protos.Request)
  ))
_sym_db.RegisterMessage(Request)


# @@protoc_insertion_point(module_scope)
