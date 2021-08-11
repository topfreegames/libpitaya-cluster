using System;

namespace NPitaya
{
    public class PitayaRouteNotFoundException : PitayaException
    {
        public PitayaRouteNotFoundException()
            : base() { }

        public PitayaRouteNotFoundException(string message)
            : base(message) { }

        public PitayaRouteNotFoundException(string format, params object[] args)
            : base(string.Format(format, args)) { }

        public PitayaRouteNotFoundException(string message, Exception innerException)
            : base(message, innerException) { }

        public PitayaRouteNotFoundException(string format, Exception innerException, params object[] args)
            : base(string.Format(format, args), innerException) { }
    }
}