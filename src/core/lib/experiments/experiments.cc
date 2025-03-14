// Copyright 2022 gRPC authors.
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

// Automatically generated by tools/codegen/core/gen_experiments.py

#include <grpc/support/port_platform.h>

#include "src/core/lib/experiments/experiments.h"

#ifndef GRPC_EXPERIMENTS_ARE_FINAL
namespace {
const char* const description_tcp_frame_size_tuning =
    "If set, enables TCP to use RPC size estimation made by higher layers. TCP "
    "would not indicate completion of a read operation until a specified "
    "number of bytes have been read over the socket. Buffers are also "
    "allocated according to estimated RPC sizes.";
const char* const description_tcp_rcv_lowat =
    "Use SO_RCVLOWAT to avoid wakeups on the read path.";
const char* const description_peer_state_based_framing =
    "If set, the max sizes of frames sent to lower layers is controlled based "
    "on the peer's memory pressure which is reflected in its max http2 frame "
    "size.";
const char* const description_flow_control_fixes =
    "Various fixes for flow control, max frame size setting.";
const char* const description_memory_pressure_controller =
    "New memory pressure controller";
const char* const description_unconstrained_max_quota_buffer_size =
    "Discard the cap on the max free pool size for one memory allocator";
const char* const description_new_hpack_huffman_decoder =
    "New HPACK huffman decoder - should be much faster than the existing "
    "implementation.";
const char* const description_event_engine_client =
    "Use EventEngine clients instead of iomgr's grpc_tcp_client";
const char* const description_monitoring_experiment =
    "Placeholder experiment to prove/disprove our monitoring is working";
const char* const description_promise_based_client_call =
    "If set, use the new gRPC promise based call code when it's appropriate "
    "(ie when all filters in a stack are promise based)";
const char* const description_free_large_allocator =
    "If set, return all free bytes from a \042big\042 allocator";
const char* const description_promise_based_server_call =
    "If set, use the new gRPC promise based call code when it's appropriate "
    "(ie when all filters in a stack are promise based)";
const char* const description_transport_supplies_client_latency =
    "If set, use the transport represented value for client latency in "
    "opencensus";
const char* const description_event_engine_listener =
    "Use EventEngine listeners instead of iomgr's grpc_tcp_server";
}  // namespace

namespace grpc_core {

const ExperimentMetadata g_experiment_metadata[] = {
    {"tcp_frame_size_tuning", description_tcp_frame_size_tuning, false},
    {"tcp_rcv_lowat", description_tcp_rcv_lowat, false},
    {"peer_state_based_framing", description_peer_state_based_framing, false},
    {"flow_control_fixes", description_flow_control_fixes, true},
    {"memory_pressure_controller", description_memory_pressure_controller,
     false},
    {"unconstrained_max_quota_buffer_size",
     description_unconstrained_max_quota_buffer_size, false},
    {"new_hpack_huffman_decoder", description_new_hpack_huffman_decoder, true},
    {"event_engine_client", description_event_engine_client, false},
    {"monitoring_experiment", description_monitoring_experiment, true},
    {"promise_based_client_call", description_promise_based_client_call, false},
    {"free_large_allocator", description_free_large_allocator, false},
    {"promise_based_server_call", description_promise_based_server_call, false},
    {"transport_supplies_client_latency",
     description_transport_supplies_client_latency, false},
    {"event_engine_listener", description_event_engine_listener, false},
};

}  // namespace grpc_core
#endif
