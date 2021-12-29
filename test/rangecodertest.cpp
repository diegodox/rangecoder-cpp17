#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#define RANGECODER_VERBOSE
#include "../rangecoder.h"

class FreqTable : public rangecoder::PModel
{
public:
    FreqTable(const std::vector<int> &data, int max_index)
    {
        m_max_index = max_index;
        m_c_freq = std::vector<rangecoder::range_t>(max_index + 1, 0);
        m_cum_freq = std::vector<rangecoder::range_t>(max_index + 1, 0);
        for (const auto &i : data)
        {
            m_c_freq[i] += 1;
        }
        for (int i = 0; i < max_index; i++)
        {
            m_cum_freq[i + 1] = m_cum_freq[i] + m_c_freq[i];
        }
    }
    void print() const
    {
        std::cout << std::endl;
        std::cout << "FREQ TABLE" << std::endl;
        for (auto i = min_index(); i <= max_index(); i++)
        {
            std::cout << "idx: " << i << ", c: " << c_freq(i) << ", cum: " << cum_freq(i) << std::endl;
        }
        std::cout << std::endl;
    }
    rangecoder::range_t c_freq(int index) const
    {
        return m_c_freq[index];
    }
    rangecoder::range_t cum_freq(int index) const
    {
        return m_cum_freq[index];
    }
    int min_index() const
    {
        return 0;
    }
    int max_index() const
    {
        return m_max_index;
    }

private:
    int m_max_index;
    std::vector<rangecoder::range_t> m_c_freq;
    std::vector<rangecoder::range_t> m_cum_freq;
};

auto helper_enc_dec_freqtable(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto max = *std::max_element(data.begin(), data.end());
    const auto pmodel = FreqTable(data, max);
    pmodel.print();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform_big(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution<65536>();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform_binary(const std::vector<bool> &data) -> std::vector<bool>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution<2>();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<bool>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform_4(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution<4>();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

auto test_uniform_16(const std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto pmodel = rangecoder::UniformDistribution<16>();
    // encode
    std::cout << "encode" << std::endl;
    auto enc = rangecoder::RangeEncoder();
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << std::dec << i << "  encode: " << data[i] << std::endl;
        enc.print_status();
        enc.encode(pmodel, data[i]);
    }
    enc.print_status();
    const auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::local::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::local::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder();
    dec.start(que);
    auto decoded = std::vector<int>();
    for (int i = 0; i < data.size(); i++)
    {
        dec.print_status();
        auto d = dec.decode(pmodel);
        std::cout << std::dec << i << "  decode: " << d << std::endl;
        decoded.push_back(d);
    }
    dec.print_status();
    std::cout << "finish" << std::endl;
    return decoded;
}

// test rangecoder with frequency table.
TEST(RangeCoderTest, EncDecTest)
{
    const auto data = std::vector<int>{1, 2, 3, 4, 5, 8, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(helper_enc_dec_freqtable(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 256 level (8bit) uniform distribution.
TEST(RangeCoderTest, UniformDistributionTest)
{
    const auto data = std::vector<int>{1, 2, 3, 4, 5, 8, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(test_uniform(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 2 level (1bit) uniform distribution.
TEST(RangeCoderTest, UniformBinaryDistributionTest)
{
    const auto data = std::vector<bool>{true, false, true, true, false, true, false, false, true, true, true, true};
    EXPECT_EQ(test_uniform_binary(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 4 level uniform distribution.
TEST(RangeCoderTest, UniformDistributionTest4)
{
    const auto data = std::vector<int>{1, 2, 3, 2, 3, 2, 3, 2, 1, 0, 3, 1};
    EXPECT_EQ(test_uniform_4(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 16 level (4bit) uniform distribution.
TEST(RangeCoderTest, UniformDistributionTest16)
{
    const auto data = std::vector<int>{1, 5, 3, 15, 2, 7, 9, 2, 1, 0, 3, 1};
    EXPECT_EQ(test_uniform_16(data), data);
    std::cout << "finish" << std::endl;
}

// test rangecoder with 65536 level (16bit) uniform distribution.
TEST(RangeCoderTest, UniformDistributionBigTest)
{
    const auto data = std::vector<int>{1, 2, 3, 4, 5, 65533, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(test_uniform_big(data), data);
    std::cout << "finish" << std::endl;
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
