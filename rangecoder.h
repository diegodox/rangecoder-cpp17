#pragma once
#ifndef RANGECODER_H_
#define RANGECODER_H_
#include <stdint.h>
#include <limits>
#include <optional>
#include <vector>
#include <queue>

namespace rangecoder
{
    using range_t = uint64_t;
    using byte_t = uint8_t;

    const range_t TOP8 = range_t(1) << (64 - 8);
    const range_t TOP16 = range_t(1) << (64 - 16);

    auto hex_zero_filled(range_t bytes) -> std::string
    {
        std::stringstream sformatter;
        sformatter << std::setfill('0') << std::setw(sizeof(range_t) * 2) << std::hex << bytes;
        return sformatter.str();
    }
    auto hex_zero_filled(byte_t byte) -> std::string
    {
        std::stringstream sformatter;
        sformatter << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(byte);
        return sformatter.str();
    }

    class PModel
    {
    public:
        // Accumulated frequency of index, i.e. sum of frequency of range [min_index, index).
        virtual range_t cum_freq(int index) const = 0;
        // Frequency of index
        virtual range_t c_freq(int index) const = 0;
        range_t total_freq() const
        {
            return cum_freq(max_index()) + c_freq(max_index());
        };
        // Returns min index, the first valid index.
        // All index 'i', that satisfy 'pmodel.min_index() <= i <= pmodel.max_index()' must be valid index.
        virtual int min_index() const = 0;
        // Returns max index, the last valid index.
        // All index 'i', that satisfy 'pmodel.min_index() <= i <= pmodel.max_index()' must be valid index.
        virtual int max_index() const = 0;
        bool index_is_valid(int index)
        {
            return min_index() <= index && index <= max_index();
        }
    };

    class RangeCoder
    {
    public:
        RangeCoder()
        {
            m_lower_bound = 0;
            m_range = std::numeric_limits<range_t>::max();
        };

        auto update_param(range_t c_freq, range_t cum_freq, range_t total_freq) -> std::vector<byte_t>
        {
            auto bytes = std::vector<byte_t>();

            auto range_per_total = m_range / total_freq;
            m_range = range_per_total * c_freq;
            m_lower_bound += range_per_total * cum_freq;

#ifdef RANGECODER_VERBOSE
            std::cout << "  range, lower bound updated" << std::endl;
            print_status();
#endif
            for (auto byte = no_carry_expansion(); byte.has_value(); byte = no_carry_expansion())
            {
                bytes.push_back(byte.value());
            }
            for (auto byte = range_reduction_expansion(); byte.has_value(); byte = range_reduction_expansion())
            {
                bytes.push_back(byte.value());
            }
#ifdef RANGECODER_VERBOSE
            std::cout << "  " << bytes.size() << " byte shifted" << std::endl;
#endif
            return bytes;
        };

        auto shift_byte() -> byte_t
        {
            auto tmp = static_cast<byte_t>(m_lower_bound >> (64 - 8));
            m_range <<= 8;
            m_lower_bound <<= 8;
#ifdef RANGECODER_VERBOSE
            std::cout << "  shifted out byte: "
                      << "0x"
                      << hex_zero_filled(tmp)
                      << std::endl;
#endif
            return tmp;
        };

        void print_status()
        {
            std::cout << "        range: "
                      << "0x" << hex_zero_filled(range()) << std::endl;
            std::cout << "  lower bound: "
                      << "0x" << hex_zero_filled(lower_bound()) << std::endl;
        }

    protected:
        auto
        lower_bound() -> range_t const
        {
            return m_lower_bound;
        };

        auto range() -> range_t const
        {
            return m_range;
        };

    private:
        auto no_carry_expansion() -> std::optional<byte_t>
        {
            if ((m_lower_bound ^ upper_bound()) < TOP8)
            {
#ifdef RANGECODER_VERBOSE
                std::cout << "  no carry expansion" << std::endl;
#endif
                return std::make_optional(shift_byte());
            }
            else
            {
                return std::nullopt;
            }
        };

        auto range_reduction_expansion() -> std::optional<uint8_t>
        {
            if (m_range < TOP16)
            {
#ifdef RANGECODER_VERBOSE
                std::cout << "  range reduction expansion" << std::endl;
#endif
                m_range = !(m_lower_bound & (TOP16 - 1));
                return std::make_optional(shift_byte());
            }
            else
            {
                return std::nullopt;
            }
        };

        auto upper_bound() -> uint64_t const
        {
            return m_lower_bound + m_range;
        };

        uint64_t m_lower_bound;
        uint64_t m_range;
    };

    class RangeEncoder : RangeCoder
    {
    public:
        // Returns number of bytes stabled.
        auto encode(PModel &pmodel, int index) -> int
        {
            auto bytes = update_param(pmodel.c_freq(index), pmodel.cum_freq(index), pmodel.total_freq());
#ifdef RANGECODER_VERBOSE
            std::cout << "  " << bytes.size() << " byte shifted" << std::endl;
#endif
            for (auto byte : bytes)
            {
                m_bytes.push_back(byte);
            }
            return bytes.size();
        };

        auto finish() -> std::vector<byte_t>
        {
            for (auto i = 0; i < 8; i++)
            {
                m_bytes.push_back(shift_byte());
            }
            return m_bytes;
        }

        void print_status()
        {
            std::cout << "        range: "
                      << "0x" << hex_zero_filled(range()) << std::endl;
            std::cout << "  lower bound: "
                      << "0x" << hex_zero_filled(lower_bound()) << std::endl;
            std::cout << "        bytes: ";
            if (m_bytes.size() > 0)
            {
                std::cout << "0x" << hex_zero_filled(m_bytes[0]);
                for (auto i = 1; i < m_bytes.size(); i++)
                {
                    std::cout << hex_zero_filled(m_bytes[i]);
                }
                std::cout << std::endl;
            }
            else
            {
                std::cout << "NULL" << std::endl;
                ;
            }
        }

    private:
        std::vector<uint8_t> m_bytes;
    };

    class RangeDecoder : RangeCoder
    {
    public:
        RangeDecoder() = delete;

        RangeDecoder(std::queue<byte_t> bytes)
        {
            m_bytes = bytes;

            for (auto i = 0; i < 8; i++)
            {
                shift_byte_buffer();
            }
        };

        // Returns index of pmodel used to encode.
        // pmodel **must** be same as used to encode.
        auto decode(PModel &pmodel) -> int
        {
            auto index = binary_search_encoded_index(pmodel);
            auto n = update_param(pmodel.c_freq(index), pmodel.cum_freq(index), pmodel.total_freq()).size();
            for (int i = 0; i < n; i++)
            {
                shift_byte_buffer();
            }
            return static_cast<int>(index);
        };

        void print_status()
        {
            std::cout << "        range: "
                      << "0x" << hex_zero_filled(range()) << std::endl;
            std::cout << "  lower bound: "
                      << "0x" << hex_zero_filled(lower_bound()) << std::endl;
            std::cout << "         data: "
                      << "0x" << hex_zero_filled(m_data) << std::endl;
        }

    private:
        // binary search encoded index
        auto binary_search_encoded_index(PModel &pmodel) -> const int
        {
            auto left = pmodel.min_index();
            auto right = pmodel.max_index();
            auto range_per_total = range() / pmodel.total_freq();
            auto f = (m_data - lower_bound()) / range_per_total;
            while (left < right)
            {
                auto mid = (left + right) / 2;
                auto mid_cum = pmodel.cum_freq(mid + 1);
                if (mid_cum <= f)
                {
                    left = mid + 1;
                }
                else
                {
                    right = mid;
                }
            }
            return left;
        };

        void shift_byte_buffer()
        {
            auto front_byte = m_bytes.front();
            m_data = (m_data << 8) | static_cast<range_t>(front_byte);
            m_bytes.pop();
        };

        std::queue<byte_t> m_bytes;
        range_t m_data;
    };

    template<int N = 256>
    class UniformDistribution : public PModel
    {
    public:
        constexpr UniformDistribution()
        {
        }
        range_t c_freq(int index) const
        {
            return 1;
        }
        range_t cum_freq(int index) const
        {
            return index;
        }
        int min_index() const
        {
            return 0;
        }
        int max_index() const
        {
            return N - 1;
        }
        void print()
        {
            std::cout << std::endl;
            std::cout << "UNIFORM DIST" << std::endl;
            for (auto i = min_index(); i <= max_index(); i++)
            {
                std::cout << "idx: " << i << ", c: " << c_freq(i) << ", cum: " << cum_freq(i) << std::endl;
            }
            std::cout << std::endl;
        }
    };
}// namespace rangecoder
#endif
