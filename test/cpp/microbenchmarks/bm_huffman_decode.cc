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

#include <cstdint>
#include <random>

#include <benchmark/benchmark.h>

#include "src/core/ext/transport/chttp2/transport/bin_encoder.h"
#include "src/core/ext/transport/chttp2/transport/decode_huff.h"
#include "src/core/lib/gprpp/no_destruct.h"
#include "src/core/lib/slice/slice.h"
#include "test/core/util/test_config.h"

const std::vector<uint8_t>* Input() {
  static const grpc_core::NoDestruct<std::vector<uint8_t>> v([]() {
    std::vector<uint8_t> v;
    std::mt19937 rd(0);
    std::uniform_int_distribution<> dist_ty(0, 100);
    std::uniform_int_distribution<> dist_byte(0, 255);
    std::uniform_int_distribution<> dist_normal(32, 126);
    for (int i = 0; i < 1024 * 1024; i++) {
      if (dist_ty(rd) == 1) {
        v.push_back(dist_byte(rd));
      } else {
        v.push_back(dist_normal(rd));
      }
    }
    grpc_core::Slice s = grpc_core::Slice::FromCopiedBuffer(v);
    grpc_core::Slice c(grpc_chttp2_huffman_compress(s.c_slice()));
    return std::vector<uint8_t>(c.begin(), c.end());
  }());
  return v.get();
}

static void BM_Decode(benchmark::State& state) {
  std::vector<uint8_t> output;
  auto add = [&output](uint8_t c) { output.push_back(c); };
  for (auto _ : state) {
    output.clear();
    grpc_core::HuffDecoder<decltype(add)>(add, Input()->data(),
                                          Input()->data() + Input()->size())
        .Run();
  }
}
BENCHMARK(BM_Decode);

// Legacy huffman decoder
static void BM_LegacyDecode(benchmark::State& state) {
  // state table for huffman decoding: given a state, gives an index/16 into
  // next_sub_tbl. Taking that index and adding the value of the nibble being
  // considered returns the next state.
  // generated by gen_hpack_tables.c
  static const uint8_t next_tbl[256] = {
      0,  1,  2,  3,  4,  1,  2,  5,  6,  1,  7,  8,  1,  3,  3,  9,  10, 11,
      1,  1,  1,  12, 1,  2,  13, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  2,  14, 1,  15, 16, 1,  17, 1,  15, 2,  7,  3,  18, 19, 1,
      1,  1,  1,  20, 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  15, 2,  2,  7,
      21, 1,  22, 1,  1,  1,  1,  1,  1,  1,  1,  15, 2,  2,  2,  2,  2,  2,
      23, 24, 25, 1,  1,  1,  1,  2,  2,  2,  26, 3,  3,  27, 10, 28, 1,  1,
      1,  1,  1,  1,  2,  3,  29, 10, 30, 1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  1,  31, 1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,
      2,  2,  2,  32, 1,  1,  15, 33, 1,  34, 35, 9,  36, 1,  1,  1,  1,  1,
      1,  1,  37, 1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  26, 9,
      38, 1,  1,  1,  1,  1,  1,  1,  15, 2,  2,  2,  2,  26, 3,  3,  39, 1,
      1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  7,  3,
      3,  3,  40, 2,  41, 1,  1,  1,  42, 43, 1,  1,  44, 1,  1,  1,  1,  15,
      2,  2,  2,  2,  2,  2,  3,  3,  3,  45, 46, 1,  1,  2,  2,  2,  35, 3,
      3,  18, 47, 2,
  };

  // next state, based upon current state and the current nibble: see above.
  // generated by gen_hpack_tables.c
  static const int16_t next_sub_tbl[48 * 16] = {
      1,   204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
      218, 2,   6,   10,  13,  14,  15,  16,  17,  2,   6,   10,  13,  14,  15,
      16,  17,  3,   7,   11,  24,  3,   7,   11,  24,  3,   7,   11,  24,  3,
      7,   11,  24,  4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   4,   8,
      4,   8,   4,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   5,
      199, 200, 201, 202, 203, 4,   8,   4,   8,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   9,   133, 134, 135, 136, 137, 138, 139, 140,
      141, 142, 143, 144, 145, 146, 147, 3,   7,   11,  24,  3,   7,   11,  24,
      4,   8,   4,   8,   4,   8,   4,   8,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   12,  132, 4,   8,   4,   8,   4,   8,
      4,   8,   4,   8,   4,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   18,  19,  20,  21,  4,   8,   4,
      8,   4,   8,   4,   8,   4,   8,   0,   0,   0,   22,  23,  91,  25,  26,
      27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  3,
      7,   11,  24,  3,   7,   11,  24,  0,   0,   0,   0,   0,   41,  42,  43,
      2,   6,   10,  13,  14,  15,  16,  17,  3,   7,   11,  24,  3,   7,   11,
      24,  4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   0,   0,
      44,  45,  2,   6,   10,  13,  14,  15,  16,  17,  46,  47,  48,  49,  50,
      51,  52,  57,  4,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   53,  54,  55,  56,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,
      68,  69,  70,  71,  72,  74,  0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   73,  75,  76,  77,  78,  79,  80,  81,  82,
      83,  84,  85,  86,  87,  88,  89,  90,  3,   7,   11,  24,  3,   7,   11,
      24,  3,   7,   11,  24,  0,   0,   0,   0,   3,   7,   11,  24,  3,   7,
      11,  24,  4,   8,   4,   8,   0,   0,   0,   92,  0,   0,   0,   93,  94,
      95,  96,  97,  98,  99,  100, 101, 102, 103, 104, 105, 3,   7,   11,  24,
      4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   4,
      8,   4,   8,   4,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 4,
      8,   4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   0,   0,
      0,   117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
      131, 2,   6,   10,  13,  14,  15,  16,  17,  4,   8,   4,   8,   4,   8,
      4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   148,
      149, 150, 151, 3,   7,   11,  24,  4,   8,   4,   8,   0,   0,   0,   0,
      0,   0,   152, 153, 3,   7,   11,  24,  3,   7,   11,  24,  3,   7,   11,
      24,  154, 155, 156, 164, 3,   7,   11,  24,  3,   7,   11,  24,  3,   7,
      11,  24,  4,   8,   4,   8,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      157, 158, 159, 160, 161, 162, 163, 165, 166, 167, 168, 169, 170, 171, 172,
      173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
      188, 189, 190, 191, 192, 193, 194, 195, 196, 4,   8,   4,   8,   4,   8,
      4,   8,   4,   8,   4,   8,   4,   8,   197, 198, 4,   8,   4,   8,   4,
      8,   4,   8,   0,   0,   0,   0,   0,   0,   219, 220, 3,   7,   11,  24,
      4,   8,   4,   8,   4,   8,   0,   0,   221, 222, 223, 224, 3,   7,   11,
      24,  3,   7,   11,  24,  4,   8,   4,   8,   4,   8,   225, 228, 4,   8,
      4,   8,   4,   8,   0,   0,   0,   0,   0,   0,   0,   0,   226, 227, 229,
      230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244,
      4,   8,   4,   8,   4,   8,   4,   8,   4,   8,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   245, 246, 247, 248, 249, 250, 251, 252,
      253, 254, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   255,
  };

  // emission table: indexed like next_tbl, ultimately gives the byte to be
  // emitted, or -1 for no byte, or 256 for end of stream
  // generated by gen_hpack_tables.c
  static const uint16_t emit_tbl[256] = {
      0,   1,   2,   3,   4,   5,   6,   7,   0,   8,   9,   10,  11,  12,  13,
      14,  15,  16,  17,  18,  19,  20,  21,  22,  0,   23,  24,  25,  26,  27,
      28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
      43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  0,   55,  56,
      57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  0,
      71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,
      86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100,
      101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
      116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130,
      131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145,
      146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 0,
      160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174,
      0,   175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188,
      189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203,
      204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218,
      219, 220, 221, 0,   222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232,
      233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
      248,
  };

  // generated by gen_hpack_tables.c
  static const int16_t emit_sub_tbl[249 * 16] = {
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  48,  48,  48,  48,  48,  48,  48,  48,  49,  49,  49,  49,  49,  49,
      49,  49,  48,  48,  48,  48,  49,  49,  49,  49,  50,  50,  50,  50,  97,
      97,  97,  97,  48,  48,  49,  49,  50,  50,  97,  97,  99,  99,  101, 101,
      105, 105, 111, 111, 48,  49,  50,  97,  99,  101, 105, 111, 115, 116, -1,
      -1,  -1,  -1,  -1,  -1,  32,  32,  32,  32,  32,  32,  32,  32,  37,  37,
      37,  37,  37,  37,  37,  37,  99,  99,  99,  99,  101, 101, 101, 101, 105,
      105, 105, 105, 111, 111, 111, 111, 115, 115, 116, 116, 32,  37,  45,  46,
      47,  51,  52,  53,  54,  55,  56,  57,  61,  61,  61,  61,  61,  61,  61,
      61,  65,  65,  65,  65,  65,  65,  65,  65,  115, 115, 115, 115, 116, 116,
      116, 116, 32,  32,  37,  37,  45,  45,  46,  46,  61,  65,  95,  98,  100,
      102, 103, 104, 108, 109, 110, 112, 114, 117, -1,  -1,  58,  58,  58,  58,
      58,  58,  58,  58,  66,  66,  66,  66,  66,  66,  66,  66,  47,  47,  51,
      51,  52,  52,  53,  53,  54,  54,  55,  55,  56,  56,  57,  57,  61,  61,
      65,  65,  95,  95,  98,  98,  100, 100, 102, 102, 103, 103, 104, 104, 108,
      108, 109, 109, 110, 110, 112, 112, 114, 114, 117, 117, 58,  66,  67,  68,
      69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,
      84,  85,  86,  87,  89,  106, 107, 113, 118, 119, 120, 121, 122, -1,  -1,
      -1,  -1,  38,  38,  38,  38,  38,  38,  38,  38,  42,  42,  42,  42,  42,
      42,  42,  42,  44,  44,  44,  44,  44,  44,  44,  44,  59,  59,  59,  59,
      59,  59,  59,  59,  88,  88,  88,  88,  88,  88,  88,  88,  90,  90,  90,
      90,  90,  90,  90,  90,  33,  33,  34,  34,  40,  40,  41,  41,  63,  63,
      39,  43,  124, -1,  -1,  -1,  35,  35,  35,  35,  35,  35,  35,  35,  62,
      62,  62,  62,  62,  62,  62,  62,  0,   0,   0,   0,   36,  36,  36,  36,
      64,  64,  64,  64,  91,  91,  91,  91,  69,  69,  69,  69,  69,  69,  69,
      69,  70,  70,  70,  70,  70,  70,  70,  70,  71,  71,  71,  71,  71,  71,
      71,  71,  72,  72,  72,  72,  72,  72,  72,  72,  73,  73,  73,  73,  73,
      73,  73,  73,  74,  74,  74,  74,  74,  74,  74,  74,  75,  75,  75,  75,
      75,  75,  75,  75,  76,  76,  76,  76,  76,  76,  76,  76,  77,  77,  77,
      77,  77,  77,  77,  77,  78,  78,  78,  78,  78,  78,  78,  78,  79,  79,
      79,  79,  79,  79,  79,  79,  80,  80,  80,  80,  80,  80,  80,  80,  81,
      81,  81,  81,  81,  81,  81,  81,  82,  82,  82,  82,  82,  82,  82,  82,
      83,  83,  83,  83,  83,  83,  83,  83,  84,  84,  84,  84,  84,  84,  84,
      84,  85,  85,  85,  85,  85,  85,  85,  85,  86,  86,  86,  86,  86,  86,
      86,  86,  87,  87,  87,  87,  87,  87,  87,  87,  89,  89,  89,  89,  89,
      89,  89,  89,  106, 106, 106, 106, 106, 106, 106, 106, 107, 107, 107, 107,
      107, 107, 107, 107, 113, 113, 113, 113, 113, 113, 113, 113, 118, 118, 118,
      118, 118, 118, 118, 118, 119, 119, 119, 119, 119, 119, 119, 119, 120, 120,
      120, 120, 120, 120, 120, 120, 121, 121, 121, 121, 121, 121, 121, 121, 122,
      122, 122, 122, 122, 122, 122, 122, 38,  38,  38,  38,  42,  42,  42,  42,
      44,  44,  44,  44,  59,  59,  59,  59,  88,  88,  88,  88,  90,  90,  90,
      90,  33,  34,  40,  41,  63,  -1,  -1,  -1,  39,  39,  39,  39,  39,  39,
      39,  39,  43,  43,  43,  43,  43,  43,  43,  43,  124, 124, 124, 124, 124,
      124, 124, 124, 35,  35,  35,  35,  62,  62,  62,  62,  0,   0,   36,  36,
      64,  64,  91,  91,  93,  93,  126, 126, 94,  125, -1,  -1,  60,  60,  60,
      60,  60,  60,  60,  60,  96,  96,  96,  96,  96,  96,  96,  96,  123, 123,
      123, 123, 123, 123, 123, 123, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  92,
      92,  92,  92,  92,  92,  92,  92,  195, 195, 195, 195, 195, 195, 195, 195,
      208, 208, 208, 208, 208, 208, 208, 208, 128, 128, 128, 128, 130, 130, 130,
      130, 131, 131, 131, 131, 162, 162, 162, 162, 184, 184, 184, 184, 194, 194,
      194, 194, 224, 224, 224, 224, 226, 226, 226, 226, 153, 153, 161, 161, 167,
      167, 172, 172, 176, 176, 177, 177, 179, 179, 209, 209, 216, 216, 217, 217,
      227, 227, 229, 229, 230, 230, 129, 132, 133, 134, 136, 146, 154, 156, 160,
      163, 164, 169, 170, 173, 178, 181, 185, 186, 187, 189, 190, 196, 198, 228,
      232, 233, -1,  -1,  -1,  -1,  1,   1,   1,   1,   1,   1,   1,   1,   135,
      135, 135, 135, 135, 135, 135, 135, 137, 137, 137, 137, 137, 137, 137, 137,
      138, 138, 138, 138, 138, 138, 138, 138, 139, 139, 139, 139, 139, 139, 139,
      139, 140, 140, 140, 140, 140, 140, 140, 140, 141, 141, 141, 141, 141, 141,
      141, 141, 143, 143, 143, 143, 143, 143, 143, 143, 147, 147, 147, 147, 147,
      147, 147, 147, 149, 149, 149, 149, 149, 149, 149, 149, 150, 150, 150, 150,
      150, 150, 150, 150, 151, 151, 151, 151, 151, 151, 151, 151, 152, 152, 152,
      152, 152, 152, 152, 152, 155, 155, 155, 155, 155, 155, 155, 155, 157, 157,
      157, 157, 157, 157, 157, 157, 158, 158, 158, 158, 158, 158, 158, 158, 165,
      165, 165, 165, 165, 165, 165, 165, 166, 166, 166, 166, 166, 166, 166, 166,
      168, 168, 168, 168, 168, 168, 168, 168, 174, 174, 174, 174, 174, 174, 174,
      174, 175, 175, 175, 175, 175, 175, 175, 175, 180, 180, 180, 180, 180, 180,
      180, 180, 182, 182, 182, 182, 182, 182, 182, 182, 183, 183, 183, 183, 183,
      183, 183, 183, 188, 188, 188, 188, 188, 188, 188, 188, 191, 191, 191, 191,
      191, 191, 191, 191, 197, 197, 197, 197, 197, 197, 197, 197, 231, 231, 231,
      231, 231, 231, 231, 231, 239, 239, 239, 239, 239, 239, 239, 239, 9,   9,
      9,   9,   142, 142, 142, 142, 144, 144, 144, 144, 145, 145, 145, 145, 148,
      148, 148, 148, 159, 159, 159, 159, 171, 171, 171, 171, 206, 206, 206, 206,
      215, 215, 215, 215, 225, 225, 225, 225, 236, 236, 236, 236, 237, 237, 237,
      237, 199, 199, 207, 207, 234, 234, 235, 235, 192, 193, 200, 201, 202, 205,
      210, 213, 218, 219, 238, 240, 242, 243, 255, -1,  203, 203, 203, 203, 203,
      203, 203, 203, 204, 204, 204, 204, 204, 204, 204, 204, 211, 211, 211, 211,
      211, 211, 211, 211, 212, 212, 212, 212, 212, 212, 212, 212, 214, 214, 214,
      214, 214, 214, 214, 214, 221, 221, 221, 221, 221, 221, 221, 221, 222, 222,
      222, 222, 222, 222, 222, 222, 223, 223, 223, 223, 223, 223, 223, 223, 241,
      241, 241, 241, 241, 241, 241, 241, 244, 244, 244, 244, 244, 244, 244, 244,
      245, 245, 245, 245, 245, 245, 245, 245, 246, 246, 246, 246, 246, 246, 246,
      246, 247, 247, 247, 247, 247, 247, 247, 247, 248, 248, 248, 248, 248, 248,
      248, 248, 250, 250, 250, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251,
      251, 251, 251, 252, 252, 252, 252, 252, 252, 252, 252, 253, 253, 253, 253,
      253, 253, 253, 253, 254, 254, 254, 254, 254, 254, 254, 254, 2,   2,   2,
      2,   3,   3,   3,   3,   4,   4,   4,   4,   5,   5,   5,   5,   6,   6,
      6,   6,   7,   7,   7,   7,   8,   8,   8,   8,   11,  11,  11,  11,  12,
      12,  12,  12,  14,  14,  14,  14,  15,  15,  15,  15,  16,  16,  16,  16,
      17,  17,  17,  17,  18,  18,  18,  18,  19,  19,  19,  19,  20,  20,  20,
      20,  21,  21,  21,  21,  23,  23,  23,  23,  24,  24,  24,  24,  25,  25,
      25,  25,  26,  26,  26,  26,  27,  27,  27,  27,  28,  28,  28,  28,  29,
      29,  29,  29,  30,  30,  30,  30,  31,  31,  31,  31,  127, 127, 127, 127,
      220, 220, 220, 220, 249, 249, 249, 249, 10,  13,  22,  256, 93,  93,  93,
      93,  126, 126, 126, 126, 94,  94,  125, 125, 60,  96,  123, -1,  92,  195,
      208, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  128,
      128, 128, 128, 128, 128, 128, 128, 130, 130, 130, 130, 130, 130, 130, 130,
      131, 131, 131, 131, 131, 131, 131, 131, 162, 162, 162, 162, 162, 162, 162,
      162, 184, 184, 184, 184, 184, 184, 184, 184, 194, 194, 194, 194, 194, 194,
      194, 194, 224, 224, 224, 224, 224, 224, 224, 224, 226, 226, 226, 226, 226,
      226, 226, 226, 153, 153, 153, 153, 161, 161, 161, 161, 167, 167, 167, 167,
      172, 172, 172, 172, 176, 176, 176, 176, 177, 177, 177, 177, 179, 179, 179,
      179, 209, 209, 209, 209, 216, 216, 216, 216, 217, 217, 217, 217, 227, 227,
      227, 227, 229, 229, 229, 229, 230, 230, 230, 230, 129, 129, 132, 132, 133,
      133, 134, 134, 136, 136, 146, 146, 154, 154, 156, 156, 160, 160, 163, 163,
      164, 164, 169, 169, 170, 170, 173, 173, 178, 178, 181, 181, 185, 185, 186,
      186, 187, 187, 189, 189, 190, 190, 196, 196, 198, 198, 228, 228, 232, 232,
      233, 233, 1,   135, 137, 138, 139, 140, 141, 143, 147, 149, 150, 151, 152,
      155, 157, 158, 165, 166, 168, 174, 175, 180, 182, 183, 188, 191, 197, 231,
      239, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  9,   9,   9,
      9,   9,   9,   9,   9,   142, 142, 142, 142, 142, 142, 142, 142, 144, 144,
      144, 144, 144, 144, 144, 144, 145, 145, 145, 145, 145, 145, 145, 145, 148,
      148, 148, 148, 148, 148, 148, 148, 159, 159, 159, 159, 159, 159, 159, 159,
      171, 171, 171, 171, 171, 171, 171, 171, 206, 206, 206, 206, 206, 206, 206,
      206, 215, 215, 215, 215, 215, 215, 215, 215, 225, 225, 225, 225, 225, 225,
      225, 225, 236, 236, 236, 236, 236, 236, 236, 236, 237, 237, 237, 237, 237,
      237, 237, 237, 199, 199, 199, 199, 207, 207, 207, 207, 234, 234, 234, 234,
      235, 235, 235, 235, 192, 192, 193, 193, 200, 200, 201, 201, 202, 202, 205,
      205, 210, 210, 213, 213, 218, 218, 219, 219, 238, 238, 240, 240, 242, 242,
      243, 243, 255, 255, 203, 204, 211, 212, 214, 221, 222, 223, 241, 244, 245,
      246, 247, 248, 250, 251, 252, 253, 254, -1,  -1,  -1,  -1,  -1,  -1,  -1,
      -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  2,   2,   2,   2,   2,   2,   2,
      2,   3,   3,   3,   3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,
      4,   4,   5,   5,   5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,
      6,   6,   6,   7,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,
      8,   8,   8,   8,   11,  11,  11,  11,  11,  11,  11,  11,  12,  12,  12,
      12,  12,  12,  12,  12,  14,  14,  14,  14,  14,  14,  14,  14,  15,  15,
      15,  15,  15,  15,  15,  15,  16,  16,  16,  16,  16,  16,  16,  16,  17,
      17,  17,  17,  17,  17,  17,  17,  18,  18,  18,  18,  18,  18,  18,  18,
      19,  19,  19,  19,  19,  19,  19,  19,  20,  20,  20,  20,  20,  20,  20,
      20,  21,  21,  21,  21,  21,  21,  21,  21,  23,  23,  23,  23,  23,  23,
      23,  23,  24,  24,  24,  24,  24,  24,  24,  24,  25,  25,  25,  25,  25,
      25,  25,  25,  26,  26,  26,  26,  26,  26,  26,  26,  27,  27,  27,  27,
      27,  27,  27,  27,  28,  28,  28,  28,  28,  28,  28,  28,  29,  29,  29,
      29,  29,  29,  29,  29,  30,  30,  30,  30,  30,  30,  30,  30,  31,  31,
      31,  31,  31,  31,  31,  31,  127, 127, 127, 127, 127, 127, 127, 127, 220,
      220, 220, 220, 220, 220, 220, 220, 249, 249, 249, 249, 249, 249, 249, 249,
      10,  10,  13,  13,  22,  22,  256, 256, 67,  67,  67,  67,  67,  67,  67,
      67,  68,  68,  68,  68,  68,  68,  68,  68,  95,  95,  95,  95,  95,  95,
      95,  95,  98,  98,  98,  98,  98,  98,  98,  98,  100, 100, 100, 100, 100,
      100, 100, 100, 102, 102, 102, 102, 102, 102, 102, 102, 103, 103, 103, 103,
      103, 103, 103, 103, 104, 104, 104, 104, 104, 104, 104, 104, 108, 108, 108,
      108, 108, 108, 108, 108, 109, 109, 109, 109, 109, 109, 109, 109, 110, 110,
      110, 110, 110, 110, 110, 110, 112, 112, 112, 112, 112, 112, 112, 112, 114,
      114, 114, 114, 114, 114, 114, 114, 117, 117, 117, 117, 117, 117, 117, 117,
      58,  58,  58,  58,  66,  66,  66,  66,  67,  67,  67,  67,  68,  68,  68,
      68,  69,  69,  69,  69,  70,  70,  70,  70,  71,  71,  71,  71,  72,  72,
      72,  72,  73,  73,  73,  73,  74,  74,  74,  74,  75,  75,  75,  75,  76,
      76,  76,  76,  77,  77,  77,  77,  78,  78,  78,  78,  79,  79,  79,  79,
      80,  80,  80,  80,  81,  81,  81,  81,  82,  82,  82,  82,  83,  83,  83,
      83,  84,  84,  84,  84,  85,  85,  85,  85,  86,  86,  86,  86,  87,  87,
      87,  87,  89,  89,  89,  89,  106, 106, 106, 106, 107, 107, 107, 107, 113,
      113, 113, 113, 118, 118, 118, 118, 119, 119, 119, 119, 120, 120, 120, 120,
      121, 121, 121, 121, 122, 122, 122, 122, 38,  38,  42,  42,  44,  44,  59,
      59,  88,  88,  90,  90,  -1,  -1,  -1,  -1,  33,  33,  33,  33,  33,  33,
      33,  33,  34,  34,  34,  34,  34,  34,  34,  34,  40,  40,  40,  40,  40,
      40,  40,  40,  41,  41,  41,  41,  41,  41,  41,  41,  63,  63,  63,  63,
      63,  63,  63,  63,  39,  39,  39,  39,  43,  43,  43,  43,  124, 124, 124,
      124, 35,  35,  62,  62,  0,   36,  64,  91,  93,  126, -1,  -1,  94,  94,
      94,  94,  94,  94,  94,  94,  125, 125, 125, 125, 125, 125, 125, 125, 60,
      60,  60,  60,  96,  96,  96,  96,  123, 123, 123, 123, -1,  -1,  -1,  -1,
      92,  92,  92,  92,  195, 195, 195, 195, 208, 208, 208, 208, 128, 128, 130,
      130, 131, 131, 162, 162, 184, 184, 194, 194, 224, 224, 226, 226, 153, 161,
      167, 172, 176, 177, 179, 209, 216, 217, 227, 229, 230, -1,  -1,  -1,  -1,
      -1,  -1,  -1,  129, 129, 129, 129, 129, 129, 129, 129, 132, 132, 132, 132,
      132, 132, 132, 132, 133, 133, 133, 133, 133, 133, 133, 133, 134, 134, 134,
      134, 134, 134, 134, 134, 136, 136, 136, 136, 136, 136, 136, 136, 146, 146,
      146, 146, 146, 146, 146, 146, 154, 154, 154, 154, 154, 154, 154, 154, 156,
      156, 156, 156, 156, 156, 156, 156, 160, 160, 160, 160, 160, 160, 160, 160,
      163, 163, 163, 163, 163, 163, 163, 163, 164, 164, 164, 164, 164, 164, 164,
      164, 169, 169, 169, 169, 169, 169, 169, 169, 170, 170, 170, 170, 170, 170,
      170, 170, 173, 173, 173, 173, 173, 173, 173, 173, 178, 178, 178, 178, 178,
      178, 178, 178, 181, 181, 181, 181, 181, 181, 181, 181, 185, 185, 185, 185,
      185, 185, 185, 185, 186, 186, 186, 186, 186, 186, 186, 186, 187, 187, 187,
      187, 187, 187, 187, 187, 189, 189, 189, 189, 189, 189, 189, 189, 190, 190,
      190, 190, 190, 190, 190, 190, 196, 196, 196, 196, 196, 196, 196, 196, 198,
      198, 198, 198, 198, 198, 198, 198, 228, 228, 228, 228, 228, 228, 228, 228,
      232, 232, 232, 232, 232, 232, 232, 232, 233, 233, 233, 233, 233, 233, 233,
      233, 1,   1,   1,   1,   135, 135, 135, 135, 137, 137, 137, 137, 138, 138,
      138, 138, 139, 139, 139, 139, 140, 140, 140, 140, 141, 141, 141, 141, 143,
      143, 143, 143, 147, 147, 147, 147, 149, 149, 149, 149, 150, 150, 150, 150,
      151, 151, 151, 151, 152, 152, 152, 152, 155, 155, 155, 155, 157, 157, 157,
      157, 158, 158, 158, 158, 165, 165, 165, 165, 166, 166, 166, 166, 168, 168,
      168, 168, 174, 174, 174, 174, 175, 175, 175, 175, 180, 180, 180, 180, 182,
      182, 182, 182, 183, 183, 183, 183, 188, 188, 188, 188, 191, 191, 191, 191,
      197, 197, 197, 197, 231, 231, 231, 231, 239, 239, 239, 239, 9,   9,   142,
      142, 144, 144, 145, 145, 148, 148, 159, 159, 171, 171, 206, 206, 215, 215,
      225, 225, 236, 236, 237, 237, 199, 207, 234, 235, 192, 192, 192, 192, 192,
      192, 192, 192, 193, 193, 193, 193, 193, 193, 193, 193, 200, 200, 200, 200,
      200, 200, 200, 200, 201, 201, 201, 201, 201, 201, 201, 201, 202, 202, 202,
      202, 202, 202, 202, 202, 205, 205, 205, 205, 205, 205, 205, 205, 210, 210,
      210, 210, 210, 210, 210, 210, 213, 213, 213, 213, 213, 213, 213, 213, 218,
      218, 218, 218, 218, 218, 218, 218, 219, 219, 219, 219, 219, 219, 219, 219,
      238, 238, 238, 238, 238, 238, 238, 238, 240, 240, 240, 240, 240, 240, 240,
      240, 242, 242, 242, 242, 242, 242, 242, 242, 243, 243, 243, 243, 243, 243,
      243, 243, 255, 255, 255, 255, 255, 255, 255, 255, 203, 203, 203, 203, 204,
      204, 204, 204, 211, 211, 211, 211, 212, 212, 212, 212, 214, 214, 214, 214,
      221, 221, 221, 221, 222, 222, 222, 222, 223, 223, 223, 223, 241, 241, 241,
      241, 244, 244, 244, 244, 245, 245, 245, 245, 246, 246, 246, 246, 247, 247,
      247, 247, 248, 248, 248, 248, 250, 250, 250, 250, 251, 251, 251, 251, 252,
      252, 252, 252, 253, 253, 253, 253, 254, 254, 254, 254, 2,   2,   3,   3,
      4,   4,   5,   5,   6,   6,   7,   7,   8,   8,   11,  11,  12,  12,  14,
      14,  15,  15,  16,  16,  17,  17,  18,  18,  19,  19,  20,  20,  21,  21,
      23,  23,  24,  24,  25,  25,  26,  26,  27,  27,  28,  28,  29,  29,  30,
      30,  31,  31,  127, 127, 220, 220, 249, 249, -1,  -1,  10,  10,  10,  10,
      10,  10,  10,  10,  13,  13,  13,  13,  13,  13,  13,  13,  22,  22,  22,
      22,  22,  22,  22,  22,  256, 256, 256, 256, 256, 256, 256, 256, 45,  45,
      45,  45,  45,  45,  45,  45,  46,  46,  46,  46,  46,  46,  46,  46,  47,
      47,  47,  47,  47,  47,  47,  47,  51,  51,  51,  51,  51,  51,  51,  51,
      52,  52,  52,  52,  52,  52,  52,  52,  53,  53,  53,  53,  53,  53,  53,
      53,  54,  54,  54,  54,  54,  54,  54,  54,  55,  55,  55,  55,  55,  55,
      55,  55,  56,  56,  56,  56,  56,  56,  56,  56,  57,  57,  57,  57,  57,
      57,  57,  57,  50,  50,  50,  50,  50,  50,  50,  50,  97,  97,  97,  97,
      97,  97,  97,  97,  99,  99,  99,  99,  99,  99,  99,  99,  101, 101, 101,
      101, 101, 101, 101, 101, 105, 105, 105, 105, 105, 105, 105, 105, 111, 111,
      111, 111, 111, 111, 111, 111, 115, 115, 115, 115, 115, 115, 115, 115, 116,
      116, 116, 116, 116, 116, 116, 116, 32,  32,  32,  32,  37,  37,  37,  37,
      45,  45,  45,  45,  46,  46,  46,  46,  47,  47,  47,  47,  51,  51,  51,
      51,  52,  52,  52,  52,  53,  53,  53,  53,  54,  54,  54,  54,  55,  55,
      55,  55,  56,  56,  56,  56,  57,  57,  57,  57,  61,  61,  61,  61,  65,
      65,  65,  65,  95,  95,  95,  95,  98,  98,  98,  98,  100, 100, 100, 100,
      102, 102, 102, 102, 103, 103, 103, 103, 104, 104, 104, 104, 108, 108, 108,
      108, 109, 109, 109, 109, 110, 110, 110, 110, 112, 112, 112, 112, 114, 114,
      114, 114, 117, 117, 117, 117, 58,  58,  66,  66,  67,  67,  68,  68,  69,
      69,  70,  70,  71,  71,  72,  72,  73,  73,  74,  74,  75,  75,  76,  76,
      77,  77,  78,  78,  79,  79,  80,  80,  81,  81,  82,  82,  83,  83,  84,
      84,  85,  85,  86,  86,  87,  87,  89,  89,  106, 106, 107, 107, 113, 113,
      118, 118, 119, 119, 120, 120, 121, 121, 122, 122, 38,  42,  44,  59,  88,
      90,  -1,  -1,  33,  33,  33,  33,  34,  34,  34,  34,  40,  40,  40,  40,
      41,  41,  41,  41,  63,  63,  63,  63,  39,  39,  43,  43,  124, 124, 35,
      62,  -1,  -1,  -1,  -1,  0,   0,   0,   0,   0,   0,   0,   0,   36,  36,
      36,  36,  36,  36,  36,  36,  64,  64,  64,  64,  64,  64,  64,  64,  91,
      91,  91,  91,  91,  91,  91,  91,  93,  93,  93,  93,  93,  93,  93,  93,
      126, 126, 126, 126, 126, 126, 126, 126, 94,  94,  94,  94,  125, 125, 125,
      125, 60,  60,  96,  96,  123, 123, -1,  -1,  92,  92,  195, 195, 208, 208,
      128, 130, 131, 162, 184, 194, 224, 226, -1,  -1,  153, 153, 153, 153, 153,
      153, 153, 153, 161, 161, 161, 161, 161, 161, 161, 161, 167, 167, 167, 167,
      167, 167, 167, 167, 172, 172, 172, 172, 172, 172, 172, 172, 176, 176, 176,
      176, 176, 176, 176, 176, 177, 177, 177, 177, 177, 177, 177, 177, 179, 179,
      179, 179, 179, 179, 179, 179, 209, 209, 209, 209, 209, 209, 209, 209, 216,
      216, 216, 216, 216, 216, 216, 216, 217, 217, 217, 217, 217, 217, 217, 217,
      227, 227, 227, 227, 227, 227, 227, 227, 229, 229, 229, 229, 229, 229, 229,
      229, 230, 230, 230, 230, 230, 230, 230, 230, 129, 129, 129, 129, 132, 132,
      132, 132, 133, 133, 133, 133, 134, 134, 134, 134, 136, 136, 136, 136, 146,
      146, 146, 146, 154, 154, 154, 154, 156, 156, 156, 156, 160, 160, 160, 160,
      163, 163, 163, 163, 164, 164, 164, 164, 169, 169, 169, 169, 170, 170, 170,
      170, 173, 173, 173, 173, 178, 178, 178, 178, 181, 181, 181, 181, 185, 185,
      185, 185, 186, 186, 186, 186, 187, 187, 187, 187, 189, 189, 189, 189, 190,
      190, 190, 190, 196, 196, 196, 196, 198, 198, 198, 198, 228, 228, 228, 228,
      232, 232, 232, 232, 233, 233, 233, 233, 1,   1,   135, 135, 137, 137, 138,
      138, 139, 139, 140, 140, 141, 141, 143, 143, 147, 147, 149, 149, 150, 150,
      151, 151, 152, 152, 155, 155, 157, 157, 158, 158, 165, 165, 166, 166, 168,
      168, 174, 174, 175, 175, 180, 180, 182, 182, 183, 183, 188, 188, 191, 191,
      197, 197, 231, 231, 239, 239, 9,   142, 144, 145, 148, 159, 171, 206, 215,
      225, 236, 237, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  199, 199,
      199, 199, 199, 199, 199, 199, 207, 207, 207, 207, 207, 207, 207, 207, 234,
      234, 234, 234, 234, 234, 234, 234, 235, 235, 235, 235, 235, 235, 235, 235,
      192, 192, 192, 192, 193, 193, 193, 193, 200, 200, 200, 200, 201, 201, 201,
      201, 202, 202, 202, 202, 205, 205, 205, 205, 210, 210, 210, 210, 213, 213,
      213, 213, 218, 218, 218, 218, 219, 219, 219, 219, 238, 238, 238, 238, 240,
      240, 240, 240, 242, 242, 242, 242, 243, 243, 243, 243, 255, 255, 255, 255,
      203, 203, 204, 204, 211, 211, 212, 212, 214, 214, 221, 221, 222, 222, 223,
      223, 241, 241, 244, 244, 245, 245, 246, 246, 247, 247, 248, 248, 250, 250,
      251, 251, 252, 252, 253, 253, 254, 254, 2,   3,   4,   5,   6,   7,   8,
      11,  12,  14,  15,  16,  17,  18,  19,  20,  21,  23,  24,  25,  26,  27,
      28,  29,  30,  31,  127, 220, 249, -1,  10,  10,  10,  10,  13,  13,  13,
      13,  22,  22,  22,  22,  256, 256, 256, 256,
  };

  std::vector<uint8_t> output;
  int16_t decode_state;
  // Parse one half byte... we leverage some lookup tables to keep the logic
  // here really simple.
  auto nibble = [&output, &decode_state](uint8_t nibble) {
    int16_t emit = emit_sub_tbl[16 * emit_tbl[decode_state] + nibble];
    int16_t next = next_sub_tbl[16 * next_tbl[decode_state] + nibble];
    if (emit != -1) {
      if (emit >= 0 && emit < 256) {
        output.push_back(static_cast<uint8_t>(emit));
      } else {
        assert(emit == 256);
      }
    }
    decode_state = next;
  };
  for (auto _ : state) {
    output.clear();
    decode_state = 0;
    for (auto c : *Input()) {
      nibble(c >> 4);
      nibble(c & 0xf);
    }
  }
}
BENCHMARK(BM_LegacyDecode);

// Some distros have RunSpecifiedBenchmarks under the benchmark namespace,
// and others do not. This allows us to support both modes.
namespace benchmark {
void RunTheBenchmarksNamespaced() { RunSpecifiedBenchmarks(); }
}  // namespace benchmark

int main(int argc, char** argv) {
  grpc::testing::TestEnvironment env(&argc, argv);
  benchmark::Initialize(&argc, argv);
  Input();  // Force initialization of input data.
  benchmark::RunTheBenchmarksNamespaced();
  return 0;
}
