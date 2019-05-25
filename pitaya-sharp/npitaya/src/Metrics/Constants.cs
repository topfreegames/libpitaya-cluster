using System;

namespace NPitaya.Metrics
{
    public class Constants
    {
        internal static string ResponseTimeMetricKey = "response_time_ns";

        public enum Status
        {
            fail,
            success
        }

        public class MetricNotFoundException : Exception
        {
            public MetricNotFoundException()
            {
            }

            public MetricNotFoundException(string message) : base(message)
            {
            }

            public MetricNotFoundException(string message, Exception inner) : base(message, inner)
            {
            }
        }
    }
}