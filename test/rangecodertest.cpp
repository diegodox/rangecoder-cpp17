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
    FreqTable(std::vector<int> &data, int max_index)
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
    void print()
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

auto helper_enc_dec_freqtable(std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    const auto max = *std::max_element(data.begin(), data.end());
    auto pmodel = FreqTable(data, max);
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
    auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder(que);
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

auto test_uniform(std::vector<int> &data) -> std::vector<int>
{
    // pmodel
    std::cout << "create pmodel" << std::endl;
    auto pmodel = rangecoder::UniformDistribution();
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
    auto bytes = enc.finish();

    std::cout << "encoded bytes: "
              << "0x" << rangecoder::hex_zero_filled(bytes[0]);
    for (auto byte : bytes)
    {
        std::cout << rangecoder::hex_zero_filled(byte);
    }
    std::cout << std::endl;

    // decode
    auto que = std::queue<rangecoder::byte_t>();
    for (auto byte : bytes)
    {
        que.push(byte);
    }
    std::cout << "decode" << std::endl;
    auto dec = rangecoder::RangeDecoder(que);
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

TEST(RangeCoderTest, EncDecTest)
{
    auto data =
        std::vector<int>{1, 2, 3, 4, 5, 8, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(helper_enc_dec_freqtable(data), data);
    std::cout << "finish" << std::endl;
}

TEST(RangeCoderTest, UniformDistributionTest)
{
    auto data =
        std::vector<int>{1, 2, 3, 4, 5, 8, 3, 2, 1, 0, 3, 7};
    EXPECT_EQ(test_uniform(data), data);
    std::cout << "finish" << std::endl;
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
