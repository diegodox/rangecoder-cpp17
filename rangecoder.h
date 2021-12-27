#pragma once
#ifndef RANGECODER_H_
#define RANGECODER_H_

#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <queue>
#include <stdint.h>
#include <string>
#include <vector>

namespace rangecoder
{
    using range_t = uint64_t;
    using byte_t = uint8_t;

    namespace local
    {
        const auto TOP8 = range_t(1) << (64 - 8);
        const auto TOP16 = range_t(1) << (64 - 16);

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

        class RangeCoder
        {
        public:
            RangeCoder()
            {
                m_lower_bound = 0;
                m_range = std::numeric_limits<range_t>::max();
            };

            auto update_param(
                const PModel &pmodel, const int index, const std::function<void(byte_t)> &f = [](byte_t) {}) -> int
            {
                const auto c_freq = pmodel.c_freq(index);
                const auto cum_freq = pmodel.cum_freq(index);
                const auto total_freq = pmodel.total_freq();

                auto num_bytes = 0;

                auto range_per_total = m_range / total_freq;
                m_range = range_per_total * c_freq;
                m_lower_bound += range_per_total * cum_freq;

#ifdef RANGECODER_VERBOSE
                std::cout << "  range, lower bound updated" << std::endl;
                print_status();
#endif
                while (is_no_carry_expansion_needed())
                {
                    f(do_no_carry_expansion());
                    num_bytes++;
                }
                while (is_range_reduction_expansion_needed())
                {
                    f(do_range_reduction_expansion());
                    num_bytes++;
                }
#ifdef RANGECODER_VERBOSE
                std::cout << "  " << bytes.size() << " byte shifted" << std::endl;
#endif
                return num_bytes;
            };

            auto shift_byte() -> byte_t
            {
                auto tmp = static_cast<byte_t>(m_lower_bound >> (64 - 8));
                m_range <<= 8;
                m_lower_bound <<= 8;
#ifdef RANGECODER_VERBOSE
                std::cout << "  shifted out byte: "
                          << "0x"
                          << local::hex_zero_filled(tmp)
                          << std::endl;
#endif
                return tmp;
            };

            void print_status() const
            {
                std::cout << "        range: "
                          << "0x" << local::hex_zero_filled(range()) << std::endl;
                std::cout << "  lower bound: "
                          << "0x" << local::hex_zero_filled(lower_bound()) << std::endl;
            }

        protected:
            void lower_bound(const range_t lower_bound)
            {
                m_lower_bound = lower_bound;
            };

            void range(const range_t range)
            {
                m_range = range;
            };

            auto lower_bound() const -> range_t
            {
                return m_lower_bound;
            };

            auto range() const -> range_t
            {
                return m_range;
            };

        private:
            auto is_no_carry_expansion_needed() -> bool
            {
                return (m_lower_bound ^ upper_bound()) < local::TOP8;
            };

            auto do_no_carry_expansion() -> byte_t
            {
#ifdef RANGECODER_VERBOSE
                std::cout << "  no carry expansion" << std::endl;
#endif
                return shift_byte();
            };

            auto is_range_reduction_expansion_needed() -> bool
            {
                return m_range < local::TOP16;
            };

            auto do_range_reduction_expansion() -> byte_t
            {
#ifdef RANGECODER_VERBOSE
                std::cout << "  range reduction expansion" << std::endl;
#endif
                m_range = !(m_lower_bound & (local::TOP16 - 1));
                return shift_byte();
            };

            auto upper_bound() const -> uint64_t
            {
                return m_lower_bound + m_range;
            };

            uint64_t m_lower_bound;
            uint64_t m_range;
        };
    }// namespace local

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

    class RangeEncoder : local::RangeCoder
    {
    public:
        // Returns number of bytes stabled.
        auto encode(const PModel &pmodel, int index) -> int
        {
            auto n = update_param(pmodel, index, [this](auto byte) { m_bytes.push_back(byte); });
#ifdef RANGECODER_VERBOSE
            std::cout << "  " << n << " byte shifted" << std::endl;
#endif
            return n;
        };

        auto finish() -> std::vector<byte_t>
        {
            for (auto i = 0; i < 8; i++)
            {
                m_bytes.push_back(shift_byte());
            }
            return m_bytes;
        }

        void print_status() const
        {
            std::cout << "        range: "
                      << "0x" << local::hex_zero_filled(range()) << std::endl;
            std::cout << "  lower bound: "
                      << "0x" << local::hex_zero_filled(lower_bound()) << std::endl;
            std::cout << "        bytes: ";
            if (m_bytes.size() > 0)
            {
                std::cout << "0x" << local::hex_zero_filled(m_bytes[0]);
                for (auto i = 1; i < m_bytes.size(); i++)
                {
                    std::cout << local::hex_zero_filled(m_bytes[i]);
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

    class RangeDecoder : local::RangeCoder
    {
    public:
        void start(std::queue<byte_t> bytes)
        {
            m_bytes = std::move(bytes);
            lower_bound(0);
            range(std::numeric_limits<range_t>::max());

            for (auto i = 0; i < 8; i++)
            {
                shift_byte_buffer();
            }
        };

        // Returns index of pmodel used to encode.
        // pmodel **must** be same as used to encode.
        auto decode(const PModel &pmodel) -> int
        {
            auto index = binary_search_encoded_index(pmodel);
            auto n = update_param(pmodel, index);
            for (int i = 0; i < n; i++)
            {
                shift_byte_buffer();
            }
            return static_cast<int>(index);
        };

        void print_status() const
        {
            std::cout << "        range: "
                      << "0x" << local::hex_zero_filled(range()) << std::endl;
            std::cout << "  lower bound: "
                      << "0x" << local::hex_zero_filled(lower_bound()) << std::endl;
            std::cout << "         data: "
                      << "0x" << local::hex_zero_filled(m_data) << std::endl;
        }

    private:
        // binary search encoded index
        auto binary_search_encoded_index(const PModel &pmodel) const -> int
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

        void print() const
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
