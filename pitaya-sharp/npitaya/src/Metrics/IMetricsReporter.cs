using System.Collections.Generic;

namespace NPitaya.Metrics
{
    public interface IMetricsReporter
    {
        void ReportDistribution(string metricKey, Dictionary<string, string> labels, double value);
    }
}