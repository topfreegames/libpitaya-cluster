using System.Collections.Generic;
using System.Linq;
using Prometheus;

namespace NPitaya.Metrics
{
    public class PrometheusMetricsReporter: IMetricsReporter
    {
        private Dictionary<string, string> _additionalLabels;
        private Dictionary<string, string> _constantLabels;
        private Dictionary<string, Counter> _countReportersMap;
        private Dictionary<string, Summary> _summaryReportersMap;
        private Dictionary<string, Gauge> _gaugeReportersMap;

        public PrometheusMetricsReporter(string serverType, string game, int port, Dictionary<string, string> constantLabels = null, Dictionary<string, string> additionalLabels = null)
        {
            _constantLabels = constantLabels;
            if (constantLabels is null) { _constantLabels = new Dictionary<string, string>(); }
            _constantLabels["game"] = game;
            _constantLabels["serverType"] = serverType;
            _additionalLabels = additionalLabels;
            if (additionalLabels is null) { _additionalLabels = new Dictionary<string, string>(); }
            _countReportersMap = new Dictionary<string, Counter>();
            _summaryReportersMap = new Dictionary<string, Summary>();
            _gaugeReportersMap = new Dictionary<string, Gauge>();

            _registerMetrics();

            var prometheusServer = new MetricServer(port: port);
            prometheusServer.Start();
        }

        private void _registerMetrics()
        {
            _addSummaryReporter(
                Constants.PitayaKey,
                "handler",
                Constants.ResponseTimeMetricKey,
                "the time to process a msg in nanoseconds",
                new[]{"route", "status", "type", "code"});
        }

        private void _addSummaryReporter(string metricNamespace, string metricSubsystem, string metricName, string metricHelp, string[] metricLabels)
        {
            var allLabels = new List<string>();
            allLabels.AddRange(_constantLabels.Keys.ToArray());
            allLabels.AddRange(_additionalLabels.Keys.ToArray());
            allLabels.AddRange(metricLabels);
            var summary = Prometheus.Metrics.CreateSummary(metricNamespace + "_" + metricSubsystem + "_" + metricName, metricHelp, new SummaryConfiguration
            {
                Objectives = new []
                {
                    new QuantileEpsilonPair(0.7, 0.02),
                    new QuantileEpsilonPair(0.95, 0.005),
                    new QuantileEpsilonPair(0.99, 0.001),
                },
                LabelNames = allLabels.ToArray(),
            });
            _summaryReportersMap[metricName] = summary;
        }

        private void _addCounterReporter(string metricNamespace, string metricSubsystem, string metricName, string metricHelp, string[] metricLabels)
        {
            var allLabels = new List<string>();
            allLabels.AddRange(_constantLabels.Keys.ToArray());
            allLabels.AddRange(_additionalLabels.Keys.ToArray());
            allLabels.AddRange(metricLabels);
            var counter = Prometheus.Metrics.CreateCounter(metricNamespace + "_" + metricSubsystem + "_" + metricName, metricHelp, new CounterConfiguration
            {
                LabelNames = allLabels.ToArray(),
            });
            _countReportersMap[metricName] = counter;
        }

        public void ReportCount(string metricKey, Dictionary<string, string> tags, double value)
        {
            if (!_countReportersMap.TryGetValue(metricKey, out var counter)) return;
            var labelValues = _ensureLabels(tags, counter.LabelNames);
            counter.WithLabels(labelValues).Inc(value);
        }
        public void ReportSummary(string metricKey, Dictionary<string, string> tags, double value)
        {
            if (!_summaryReportersMap.TryGetValue(metricKey, out var summary)) return;
            var labelValues = _ensureLabels(tags, summary.LabelNames);
            summary.WithLabels(labelValues).Observe(value);
        }

        private string[] _ensureLabels(Dictionary<string, string> labels, string[] labelNames)
        {
            var labelValues = new List<string>();
            foreach (var lName in labelNames)
            {
                if (labels.TryGetValue(lName, out var lValue))
                {
                    labelValues.Add(lValue);
                    continue;
                }
                if (_additionalLabels.TryGetValue(lName, out lValue))
                {
                    labelValues.Add(lValue);
                    continue;
                }
                if (_constantLabels.TryGetValue(lName, out lValue))
                {
                    labelValues.Add(lValue);
                    continue;
                }

                labelValues.Add("");
            }

            return labelValues.ToArray();
        }
    }
}