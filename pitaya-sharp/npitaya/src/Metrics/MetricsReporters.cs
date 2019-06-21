using System.Collections.Concurrent;
using System.Collections.Generic;

namespace NPitaya.Metrics
{
    public static class MetricsReporters
    {
        private static BlockingCollection<IMetricsReporter> _reporters = new BlockingCollection<IMetricsReporter>();

        public static void AddMetricReporter(IMetricsReporter mr)
        {
            _reporters.Add(mr);
        }

        public static void ReportTimer(Dictionary<string, string> tags, double value)
        {
            ReportSummary(Constants.ResponseTimeMetricKey, tags, value);
        }

        public static void ReportSummary(string key, Dictionary<string, string> tags, double value)
        {
            foreach (var reporter in _reporters)
            {
                reporter.ReportSummary(Constants.ResponseTimeMetricKey, tags, value);
            }
        }
    }
}