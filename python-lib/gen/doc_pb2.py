# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: doc.proto

import sys
_b=sys.version_info[0]<3 and (lambda x:x) or (lambda x:x.encode('latin1'))
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='doc.proto',
  package='protos',
  syntax='proto3',
  serialized_options=None,
  serialized_pb=_b('\n\tdoc.proto\x12\x06protos\"\x12\n\x03\x44oc\x12\x0b\n\x03\x64oc\x18\x01 \x01(\tb\x06proto3')
)




_DOC = _descriptor.Descriptor(
  name='Doc',
  full_name='protos.Doc',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  fields=[
    _descriptor.FieldDescriptor(
      name='doc', full_name='protos.Doc.doc', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=_b("").decode('utf-8'),
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
  serialized_start=21,
  serialized_end=39,
)

DESCRIPTOR.message_types_by_name['Doc'] = _DOC
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

Doc = _reflection.GeneratedProtocolMessageType('Doc', (_message.Message,), dict(
  DESCRIPTOR = _DOC,
  __module__ = 'doc_pb2'
  # @@protoc_insertion_point(class_scope:protos.Doc)
  ))
_sym_db.RegisterMessage(Doc)


# @@protoc_insertion_point(module_scope)
