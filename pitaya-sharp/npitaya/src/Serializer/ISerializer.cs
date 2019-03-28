namespace NPitaya.Serializer
{
    public interface ISerializer
    {
        byte[] Marshal(object o);
        void Unmarshal(byte[] bytes, ref object o);
    }
}