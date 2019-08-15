#ifndef OPENTRACING_VERSION_H
#define OPENTRACING_VERSION_H

#define OPENTRACING_VERSION "1.5.1"
#define OPENTRACING_ABI_VERSION "2"

// clang-format off
#define BEGIN_OPENTRACING_ABI_NAMESPACE \
  inline namespace v2 {
#define END_OPENTRACING_ABI_NAMESPACE \
  }  // namespace v2
// clang-format on

#endif // OPENTRACING_VERSION_H
