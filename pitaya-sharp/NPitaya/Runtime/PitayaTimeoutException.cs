using System;

namespace NPitaya
{
    public class PitayaTimeoutException : PitayaException
    {
        public PitayaTimeoutException()
            : base() { }

        public PitayaTimeoutException(string message)
            : base(message) { }

        public PitayaTimeoutException(string format, params object[] args)
            : base(string.Format(format, args)) { }

        public PitayaTimeoutException(string message, Exception innerException)
            : base(message, innerException) { }

        public PitayaTimeoutException(string format, Exception innerException, params object[] args)
            : base(string.Format(format, args), innerException) { }
    }
}