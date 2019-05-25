using System.Collections.Generic;

namespace NPitaya.Metrics
{
    public static class MetricsReporters
    {
        private static List<IMetricsReporter> reporters = new List<IMetricsReporter>();

        public static void AddMetricReporter(IMetricsReporter mr)
        {
            reporters.Add(mr);
        }

        public static void ReportTimer(Dictionary<string, string> tags, double value)
        {
                ReportSummary(Constants.ResponseTimeMetricKey, tags, value);
        }

        public static void ReportSummary(string key, Dictionary<string, string> tags, double value)
        {
            foreach (var reporter in reporters)
            {
                reporter.ReportDistribution(Constants.ResponseTimeMetricKey, tags, value);
            }
        }
    }
}