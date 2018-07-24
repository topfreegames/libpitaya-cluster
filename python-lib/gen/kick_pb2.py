# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: kick.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
from google.protobuf import descriptor_pb2
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='kick.proto',
  package='protos',
  syntax='proto3',
  serialized_pb=_b('\n\nkick.proto\x12\x06protos\"\x19\n\x07KickMsg\x12\x0e\n\x06userId\x18\x01 \x01(\t\"\x1c\n\nKickAnswer\x12\x0e\n\x06kicked\x18\x01 \x01(\x08\x62\x06proto3')
)




_KICKMSG = _descriptor.Descriptor(
  name='KickMsg',
  full_name='protos.KickMsg',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='userId', full_name='protos.KickMsg.userId', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=22,
  serialized_end=47,
)


_KICKANSWER = _descriptor.Descriptor(
  name='KickAnswer',
  full_name='protos.KickAnswer',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='kicked', full_name='protos.KickAnswer.kicked', index=0,
      number=1, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      options=None, file=DESCRIPTOR),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=49,
  serialized_end=77,
)

DESCRIPTOR.message_types_by_name['KickMsg'] = _KICKMSG
DESCRIPTOR.message_types_by_name['KickAnswer'] = _KICKANSWER
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

KickMsg = _reflection.GeneratedProtocolMessageType('KickMsg', (_message.Message,), dict(
  DESCRIPTOR = _KICKMSG,
  __module__ = 'kick_pb2'
  # @@protoc_insertion_point(class_scope:protos.KickMsg)
  ))
_sym_db.RegisterMessage(KickMsg)

KickAnswer = _reflection.GeneratedProtocolMessageType('KickAnswer', (_message.Message,), dict(
  DESCRIPTOR = _KICKANSWER,
  __module__ = 'kick_pb2'
  # @@protoc_insertion_point(class_scope:protos.KickAnswer)
  ))
_sym_db.RegisterMessage(KickAnswer)


# @@protoc_insertion_point(module_scope)
