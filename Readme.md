# Range Coder

![test](https://github.com/diegodox/rangecoder-cpp17/actions/workflows/test.yml/badge.svg)

Header-only range coder library.

## Usage

```c++
#include "rangecoder.h"
#include <queue>
#include <vector>

// Probability model used to encode/decode.
// Note this inherit rangecoder::PModel and implement their virtual method.
// Basic idea is frequency table.
// (Sample implementation is available in test directory)
class PModel : public rangecoder::PModel {
    public:
    // Accumrate frequency of index, i.e. sum of frequency of range [min_index, index).
    auto c_freq(int index) -> range_t const { /* omit */ }
    auto cum_freq(int index) -> range_t const { /* omit */ }
    auto min_index() -> int const { /* omit */ }
    auto max_index() -> int const { /* omit */ }

    // omit
}

int main() {
    // We have data to encode.
    std::vector<int> sequence_of_data = get_data();
    // Some PModel,　compatible to data, is required to both encode and decode.
    auto pmodel = PModel();

    // Encode
    auto encoder = rangecoder::RangeEncoder();
    for (auto data: sequence_of_data) {
        encoder.encode(pmodel, data);
    }
    auto bytes = encoder.finish();

    // Convert vector to queue.
    auto bytes_queue = std::queue<rangecoder::byte_t>();
    for (auto byte: bytes) {
        bytes_queue.push(byte);
    }

    // Decode
    auto decoded = std::vector<int>();
    auto decoder = rangecoder::RangeDecoder();
    decoder.start(bytes_queue);
    for (int i = 0; i < sequence_of_data.size(); i++){
        decoded.push_back(decoder.decode(pmodel));
    }

    assert(sequence_of_data == decoded);
}
```
