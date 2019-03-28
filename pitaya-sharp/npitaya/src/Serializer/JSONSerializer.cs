using System.Text;
using NPitaya.Serializer;
using PitayaSimpleJson;

namespace NPitaya.Serializer
{
    public class JSONSerializer: ISerializer
    {
        public byte[] Marshal(object o)
        {
            return Encoding.UTF8.GetBytes(SimpleJson.SerializeObject(o));
        }

        public void Unmarshal(byte[] bytes, ref object o)
        {
            if (bytes.Length == 0) bytes = Encoding.UTF8.GetBytes("{}");
            o = SimpleJson.DeserializeObject(Encoding.UTF8.GetString(bytes), o.GetType());
        }
    }
}