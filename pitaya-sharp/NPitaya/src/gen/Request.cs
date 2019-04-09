// <auto-generated>
//     Generated by the protocol buffer compiler.  DO NOT EDIT!
//     source: request.proto
// </auto-generated>
#pragma warning disable 1591, 0612, 3021
#region Designer generated code

using pb = global::Google.Protobuf;
using pbc = global::Google.Protobuf.Collections;
using pbr = global::Google.Protobuf.Reflection;
using scg = global::System.Collections.Generic;
namespace Protos {

  /// <summary>Holder for reflection information generated from request.proto</summary>
  public static partial class RequestReflection {

    #region Descriptor
    /// <summary>File descriptor for request.proto</summary>
    public static pbr::FileDescriptor Descriptor {
      get { return descriptor; }
    }
    private static pbr::FileDescriptor descriptor;

    static RequestReflection() {
      byte[] descriptorData = global::System.Convert.FromBase64String(
          string.Concat(
            "Cg1yZXF1ZXN0LnByb3RvEgZwcm90b3MaDXNlc3Npb24ucHJvdG8aCW1zZy5w",
            "cm90byKKAQoHUmVxdWVzdBIdCgR0eXBlGAEgASgOMg8ucHJvdG9zLlJQQ1R5",
            "cGUSIAoHc2Vzc2lvbhgCIAEoCzIPLnByb3Rvcy5TZXNzaW9uEhgKA21zZxgD",
            "IAEoCzILLnByb3Rvcy5Nc2cSEgoKZnJvbnRlbmRJRBgEIAEoCRIQCghtZXRh",
            "ZGF0YRgFIAEoDCocCgdSUENUeXBlEgcKA1N5cxAAEggKBFVzZXIQAWIGcHJv",
            "dG8z"));
      descriptor = pbr::FileDescriptor.FromGeneratedCode(descriptorData,
          new pbr::FileDescriptor[] { global::Protos.SessionReflection.Descriptor, global::Protos.MsgReflection.Descriptor, },
          new pbr::GeneratedClrTypeInfo(new[] {typeof(global::Protos.RPCType), }, new pbr::GeneratedClrTypeInfo[] {
            new pbr::GeneratedClrTypeInfo(typeof(global::Protos.Request), global::Protos.Request.Parser, new[]{ "Type", "Session", "Msg", "FrontendID", "Metadata" }, null, null, null)
          }));
    }
    #endregion

  }
  #region Enums
  public enum RPCType {
    [pbr::OriginalName("Sys")] Sys = 0,
    [pbr::OriginalName("User")] User = 1,
  }

  #endregion

  #region Messages
  public sealed partial class Request : pb::IMessage<Request> {
    private static readonly pb::MessageParser<Request> _parser = new pb::MessageParser<Request>(() => new Request());
    private pb::UnknownFieldSet _unknownFields;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pb::MessageParser<Request> Parser { get { return _parser; } }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public static pbr::MessageDescriptor Descriptor {
      get { return global::Protos.RequestReflection.Descriptor.MessageTypes[0]; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    pbr::MessageDescriptor pb::IMessage.Descriptor {
      get { return Descriptor; }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Request() {
      OnConstruction();
    }

    partial void OnConstruction();

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Request(Request other) : this() {
      type_ = other.type_;
      session_ = other.session_ != null ? other.session_.Clone() : null;
      msg_ = other.msg_ != null ? other.msg_.Clone() : null;
      frontendID_ = other.frontendID_;
      metadata_ = other.metadata_;
      _unknownFields = pb::UnknownFieldSet.Clone(other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public Request Clone() {
      return new Request(this);
    }

    /// <summary>Field number for the "type" field.</summary>
    public const int TypeFieldNumber = 1;
    private global::Protos.RPCType type_ = 0;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Protos.RPCType Type {
      get { return type_; }
      set {
        type_ = value;
      }
    }

    /// <summary>Field number for the "session" field.</summary>
    public const int SessionFieldNumber = 2;
    private global::Protos.Session session_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Protos.Session Session {
      get { return session_; }
      set {
        session_ = value;
      }
    }

    /// <summary>Field number for the "msg" field.</summary>
    public const int MsgFieldNumber = 3;
    private global::Protos.Msg msg_;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public global::Protos.Msg Msg {
      get { return msg_; }
      set {
        msg_ = value;
      }
    }

    /// <summary>Field number for the "frontendID" field.</summary>
    public const int FrontendIDFieldNumber = 4;
    private string frontendID_ = "";
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public string FrontendID {
      get { return frontendID_; }
      set {
        frontendID_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    /// <summary>Field number for the "metadata" field.</summary>
    public const int MetadataFieldNumber = 5;
    private pb::ByteString metadata_ = pb::ByteString.Empty;
    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public pb::ByteString Metadata {
      get { return metadata_; }
      set {
        metadata_ = pb::ProtoPreconditions.CheckNotNull(value, "value");
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override bool Equals(object other) {
      return Equals(other as Request);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public bool Equals(Request other) {
      if (ReferenceEquals(other, null)) {
        return false;
      }
      if (ReferenceEquals(other, this)) {
        return true;
      }
      if (Type != other.Type) return false;
      if (!object.Equals(Session, other.Session)) return false;
      if (!object.Equals(Msg, other.Msg)) return false;
      if (FrontendID != other.FrontendID) return false;
      if (Metadata != other.Metadata) return false;
      return Equals(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override int GetHashCode() {
      int hash = 1;
      if (Type != 0) hash ^= Type.GetHashCode();
      if (session_ != null) hash ^= Session.GetHashCode();
      if (msg_ != null) hash ^= Msg.GetHashCode();
      if (FrontendID.Length != 0) hash ^= FrontendID.GetHashCode();
      if (Metadata.Length != 0) hash ^= Metadata.GetHashCode();
      if (_unknownFields != null) {
        hash ^= _unknownFields.GetHashCode();
      }
      return hash;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public override string ToString() {
      return pb::JsonFormatter.ToDiagnosticString(this);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void WriteTo(pb::CodedOutputStream output) {
      if (Type != 0) {
        output.WriteRawTag(8);
        output.WriteEnum((int) Type);
      }
      if (session_ != null) {
        output.WriteRawTag(18);
        output.WriteMessage(Session);
      }
      if (msg_ != null) {
        output.WriteRawTag(26);
        output.WriteMessage(Msg);
      }
      if (FrontendID.Length != 0) {
        output.WriteRawTag(34);
        output.WriteString(FrontendID);
      }
      if (Metadata.Length != 0) {
        output.WriteRawTag(42);
        output.WriteBytes(Metadata);
      }
      if (_unknownFields != null) {
        _unknownFields.WriteTo(output);
      }
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public int CalculateSize() {
      int size = 0;
      if (Type != 0) {
        size += 1 + pb::CodedOutputStream.ComputeEnumSize((int) Type);
      }
      if (session_ != null) {
        size += 1 + pb::CodedOutputStream.ComputeMessageSize(Session);
      }
      if (msg_ != null) {
        size += 1 + pb::CodedOutputStream.ComputeMessageSize(Msg);
      }
      if (FrontendID.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeStringSize(FrontendID);
      }
      if (Metadata.Length != 0) {
        size += 1 + pb::CodedOutputStream.ComputeBytesSize(Metadata);
      }
      if (_unknownFields != null) {
        size += _unknownFields.CalculateSize();
      }
      return size;
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(Request other) {
      if (other == null) {
        return;
      }
      if (other.Type != 0) {
        Type = other.Type;
      }
      if (other.session_ != null) {
        if (session_ == null) {
          session_ = new global::Protos.Session();
        }
        Session.MergeFrom(other.Session);
      }
      if (other.msg_ != null) {
        if (msg_ == null) {
          msg_ = new global::Protos.Msg();
        }
        Msg.MergeFrom(other.Msg);
      }
      if (other.FrontendID.Length != 0) {
        FrontendID = other.FrontendID;
      }
      if (other.Metadata.Length != 0) {
        Metadata = other.Metadata;
      }
      _unknownFields = pb::UnknownFieldSet.MergeFrom(_unknownFields, other._unknownFields);
    }

    [global::System.Diagnostics.DebuggerNonUserCodeAttribute]
    public void MergeFrom(pb::CodedInputStream input) {
      uint tag;
      while ((tag = input.ReadTag()) != 0) {
        switch(tag) {
          default:
            _unknownFields = pb::UnknownFieldSet.MergeFieldFrom(_unknownFields, input);
            break;
          case 8: {
            type_ = (global::Protos.RPCType) input.ReadEnum();
            break;
          }
          case 18: {
            if (session_ == null) {
              session_ = new global::Protos.Session();
            }
            input.ReadMessage(session_);
            break;
          }
          case 26: {
            if (msg_ == null) {
              msg_ = new global::Protos.Msg();
            }
            input.ReadMessage(msg_);
            break;
          }
          case 34: {
            FrontendID = input.ReadString();
            break;
          }
          case 42: {
            Metadata = input.ReadBytes();
            break;
          }
        }
      }
    }

  }

  #endregion

}

#endregion Designer generated code
