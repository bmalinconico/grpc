//
//
// Copyright 2018 gRPC authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//

#include <grpc/support/port_platform.h>

#include "src/cpp/ext/filters/census/grpc_plugin.h"

#include <limits.h>

#include <atomic>

#include "absl/base/attributes.h"
#include "absl/strings/string_view.h"
#include "opencensus/tags/tag_key.h"
#include "opencensus/trace/span.h"

#include <grpcpp/opencensus.h>
#include <grpcpp/server_context.h>

#include "src/core/lib/surface/channel_stack_type.h"
#include "src/cpp/common/channel_filter.h"
#include "src/cpp/ext/filters/census/channel_filter.h"
#include "src/cpp/ext/filters/census/client_filter.h"
#include "src/cpp/ext/filters/census/measures.h"
#include "src/cpp/ext/filters/census/server_filter.h"

namespace grpc {

void RegisterOpenCensusPlugin() {
  RegisterChannelFilter<
      internal::OpenCensusClientChannelData,
      internal::OpenCensusClientChannelData::OpenCensusClientCallData>(
      "opencensus_client", GRPC_CLIENT_CHANNEL, INT_MAX /* priority */,
      nullptr /* condition function */);
  RegisterChannelFilter<internal::OpenCensusChannelData,
                        internal::OpenCensusServerCallData>(
      "opencensus_server", GRPC_SERVER_CHANNEL, INT_MAX /* priority */,
      nullptr /* condition function */);

  // Access measures to ensure they are initialized. Otherwise, creating a view
  // before the first RPC would cause an error.
  RpcClientSentBytesPerRpc();
  RpcClientReceivedBytesPerRpc();
  RpcClientRoundtripLatency();
  RpcClientServerLatency();
  RpcClientStartedRpcs();
  RpcClientSentMessagesPerRpc();
  RpcClientReceivedMessagesPerRpc();
  RpcClientRetriesPerCall();
  RpcClientTransparentRetriesPerCall();
  RpcClientRetryDelayPerCall();
  RpcClientTransportLatency();

  RpcServerSentBytesPerRpc();
  RpcServerReceivedBytesPerRpc();
  RpcServerServerLatency();
  RpcServerStartedRpcs();
  RpcServerSentMessagesPerRpc();
  RpcServerReceivedMessagesPerRpc();
}

::opencensus::trace::Span GetSpanFromServerContext(
    grpc::ServerContext* context) {
  if (context == nullptr) return opencensus::trace::Span::BlankSpan();

  return reinterpret_cast<const grpc::experimental::CensusContext*>(
             context->census_context())
      ->Span();
}

namespace experimental {

// These measure definitions should be kept in sync across opencensus
// implementations--see
// https://github.com/census-instrumentation/opencensus-java/blob/master/contrib/grpc_metrics/src/main/java/io/opencensus/contrib/grpc/metrics/RpcMeasureConstants.java.
::opencensus::tags::TagKey ClientMethodTagKey() {
  static const auto method_tag_key =
      ::opencensus::tags::TagKey::Register("grpc_client_method");
  return method_tag_key;
}

::opencensus::tags::TagKey ClientStatusTagKey() {
  static const auto status_tag_key =
      ::opencensus::tags::TagKey::Register("grpc_client_status");
  return status_tag_key;
}

::opencensus::tags::TagKey ServerMethodTagKey() {
  static const auto method_tag_key =
      ::opencensus::tags::TagKey::Register("grpc_server_method");
  return method_tag_key;
}

::opencensus::tags::TagKey ServerStatusTagKey() {
  static const auto status_tag_key =
      ::opencensus::tags::TagKey::Register("grpc_server_status");
  return status_tag_key;
}

// Client
ABSL_CONST_INIT const absl::string_view
    kRpcClientSentMessagesPerRpcMeasureName =
        "grpc.io/client/sent_messages_per_rpc";

ABSL_CONST_INIT const absl::string_view kRpcClientSentBytesPerRpcMeasureName =
    "grpc.io/client/sent_bytes_per_rpc";

ABSL_CONST_INIT const absl::string_view
    kRpcClientReceivedMessagesPerRpcMeasureName =
        "grpc.io/client/received_messages_per_rpc";

ABSL_CONST_INIT const absl::string_view
    kRpcClientReceivedBytesPerRpcMeasureName =
        "grpc.io/client/received_bytes_per_rpc";

ABSL_CONST_INIT const absl::string_view kRpcClientRoundtripLatencyMeasureName =
    "grpc.io/client/roundtrip_latency";

ABSL_CONST_INIT const absl::string_view kRpcClientServerLatencyMeasureName =
    "grpc.io/client/server_latency";

ABSL_CONST_INIT const absl::string_view kRpcClientStartedRpcsMeasureName =
    "grpc.io/client/started_rpcs";

ABSL_CONST_INIT const absl::string_view kRpcClientRetriesPerCallMeasureName =
    "grpc.io/client/retries_per_call";

ABSL_CONST_INIT const absl::string_view
    kRpcClientTransparentRetriesPerCallMeasureName =
        "grpc.io/client/transparent_retries_per_call";

ABSL_CONST_INIT const absl::string_view kRpcClientRetryDelayPerCallMeasureName =
    "grpc.io/client/retry_delay_per_call";

ABSL_CONST_INIT const absl::string_view kRpcClientTransportLatencyMeasureName =
    "grpc.io/client/transport_latency";

// Server
ABSL_CONST_INIT const absl::string_view
    kRpcServerSentMessagesPerRpcMeasureName =
        "grpc.io/server/sent_messages_per_rpc";

ABSL_CONST_INIT const absl::string_view kRpcServerSentBytesPerRpcMeasureName =
    "grpc.io/server/sent_bytes_per_rpc";

ABSL_CONST_INIT const absl::string_view
    kRpcServerReceivedMessagesPerRpcMeasureName =
        "grpc.io/server/received_messages_per_rpc";

ABSL_CONST_INIT const absl::string_view
    kRpcServerReceivedBytesPerRpcMeasureName =
        "grpc.io/server/received_bytes_per_rpc";

ABSL_CONST_INIT const absl::string_view kRpcServerServerLatencyMeasureName =
    "grpc.io/server/server_latency";

ABSL_CONST_INIT const absl::string_view kRpcServerStartedRpcsMeasureName =
    "grpc.io/server/started_rpcs";

}  // namespace experimental

namespace internal {

namespace {
std::atomic<bool> g_open_census_stats_enabled(true);
std::atomic<bool> g_open_census_tracing_enabled(true);
}  // namespace

//
// OpenCensusRegistry
//

OpenCensusRegistry& OpenCensusRegistry::Get() {
  static OpenCensusRegistry* registry = new OpenCensusRegistry;
  return *registry;
}

::opencensus::tags::TagMap OpenCensusRegistry::PopulateTagMapWithConstantLabels(
    const ::opencensus::tags::TagMap& tag_map) {
  std::vector<std::pair<::opencensus::tags::TagKey, std::string>> tags =
      tag_map.tags();
  for (const auto& label : constant_labels_) {
    tags.emplace_back(label.tag_key, label.value);
  }
  return ::opencensus::tags::TagMap(std::move(tags));
}

void OpenCensusRegistry::PopulateCensusContextWithConstantAttributes(
    grpc::experimental::CensusContext* context) {
  // We reuse the constant labels for the attributes
  for (const auto& label : constant_labels_) {
    context->AddSpanAttribute(label.key, label.value);
  }
}

void EnableOpenCensusStats(bool enable) {
  g_open_census_stats_enabled = enable;
}

void EnableOpenCensusTracing(bool enable) {
  g_open_census_tracing_enabled = enable;
}

bool OpenCensusStatsEnabled() {
  return g_open_census_stats_enabled.load(std::memory_order_relaxed);
}

bool OpenCensusTracingEnabled() {
  return g_open_census_tracing_enabled.load(std::memory_order_relaxed);
}

}  // namespace internal

}  // namespace grpc
