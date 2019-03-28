using Google.Protobuf;
using NPitaya.Serializer;

namespace NPitaya.Serializer
{
    public class ProtobufSerializer: ISerializer
    {
        public byte[] Marshal(object o)
        {
            if (o == null) return new byte[]{};
            if (!(o is IMessage))
            {
                throw new PitayaException("Cannot serialize a type that doesn't implement IMessage");
            }

            if (o == null) return new byte[] { };

            return ((IMessage) o).ToByteArray();
        }

        public void Unmarshal(byte[] bytes, ref object o)
        {
            if (!(o is IMessage))
            {
                throw new PitayaException("Cannot deserialize to a type that doesn't implement IMessage");
                
            }

            ((IMessage)o).MergeFrom(bytes);
        }
    }
}
