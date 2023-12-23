// MIT License
//
// Copyright 2021 Tyler Coy
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cmath>
#include <tuple>
#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
#include <gtest/gtest.h>
#include "quadra/decoder.h"
#include "unit_tests/util.h"

namespace quadra::test::decoder
{

constexpr uint32_t kSampleRate = 48000;
constexpr uint32_t kCRCSeed = 0;
constexpr uint8_t kFillByte = 0xFF;
constexpr float kFlashWriteTime = 0.025f;

using Signal = std::vector<float>;

template <int symbol_duration, int noise_dB, bool invert,
    int samplerate_mismatch_ppm>
using ParamType = std::tuple<
    std::integral_constant<int, symbol_duration>,
    std::integral_constant<int, noise_dB>,
    std::integral_constant<int, invert>,
    std::integral_constant<int, samplerate_mismatch_ppm>
    >;
using ParamTypeList = ::testing::Types<
    /*[[[cog
    import itertools
    lines = []
    symbol_duration = (6, 8, 12, 16)
    noise_dB = (-100, -60, -18)
    invert = (0, 1)
    mismatch_ppm = (0, 100, -100, 50000, -50000)
    tests = itertools.product(symbol_duration, noise_dB, invert, mismatch_ppm)
    for (symbol_duration, noise_dB, invert, mismatch_ppm) in tests:
        lines.append('ParamType<{:2}, {:4}, {:1}, {:6}>'
            .format(symbol_duration, noise_dB, invert, mismatch_ppm))
    cog.outl(',\n'.join(lines))
    ]]]*/
    ParamType< 6, -100, 0,      0>,
    ParamType< 6, -100, 0,    100>,
    ParamType< 6, -100, 0,   -100>,
    ParamType< 6, -100, 0,  50000>,
    ParamType< 6, -100, 0, -50000>,
    ParamType< 6, -100, 1,      0>,
    ParamType< 6, -100, 1,    100>,
    ParamType< 6, -100, 1,   -100>,
    ParamType< 6, -100, 1,  50000>,
    ParamType< 6, -100, 1, -50000>,
    ParamType< 6,  -60, 0,      0>,
    ParamType< 6,  -60, 0,    100>,
    ParamType< 6,  -60, 0,   -100>,
    ParamType< 6,  -60, 0,  50000>,
    ParamType< 6,  -60, 0, -50000>,
    ParamType< 6,  -60, 1,      0>,
    ParamType< 6,  -60, 1,    100>,
    ParamType< 6,  -60, 1,   -100>,
    ParamType< 6,  -60, 1,  50000>,
    ParamType< 6,  -60, 1, -50000>,
    ParamType< 6,  -18, 0,      0>,
    ParamType< 6,  -18, 0,    100>,
    ParamType< 6,  -18, 0,   -100>,
    ParamType< 6,  -18, 0,  50000>,
    ParamType< 6,  -18, 0, -50000>,
    ParamType< 6,  -18, 1,      0>,
    ParamType< 6,  -18, 1,    100>,
    ParamType< 6,  -18, 1,   -100>,
    ParamType< 6,  -18, 1,  50000>,
    ParamType< 6,  -18, 1, -50000>,
    ParamType< 8, -100, 0,      0>,
    ParamType< 8, -100, 0,    100>,
    ParamType< 8, -100, 0,   -100>,
    ParamType< 8, -100, 0,  50000>,
    ParamType< 8, -100, 0, -50000>,
    ParamType< 8, -100, 1,      0>,
    ParamType< 8, -100, 1,    100>,
    ParamType< 8, -100, 1,   -100>,
    ParamType< 8, -100, 1,  50000>,
    ParamType< 8, -100, 1, -50000>,
    ParamType< 8,  -60, 0,      0>,
    ParamType< 8,  -60, 0,    100>,
    ParamType< 8,  -60, 0,   -100>,
    ParamType< 8,  -60, 0,  50000>,
    ParamType< 8,  -60, 0, -50000>,
    ParamType< 8,  -60, 1,      0>,
    ParamType< 8,  -60, 1,    100>,
    ParamType< 8,  -60, 1,   -100>,
    ParamType< 8,  -60, 1,  50000>,
    ParamType< 8,  -60, 1, -50000>,
    ParamType< 8,  -18, 0,      0>,
    ParamType< 8,  -18, 0,    100>,
    ParamType< 8,  -18, 0,   -100>,
    ParamType< 8,  -18, 0,  50000>,
    ParamType< 8,  -18, 0, -50000>,
    ParamType< 8,  -18, 1,      0>,
    ParamType< 8,  -18, 1,    100>,
    ParamType< 8,  -18, 1,   -100>,
    ParamType< 8,  -18, 1,  50000>,
    ParamType< 8,  -18, 1, -50000>,
    ParamType<12, -100, 0,      0>,
    ParamType<12, -100, 0,    100>,
    ParamType<12, -100, 0,   -100>,
    ParamType<12, -100, 0,  50000>,
    ParamType<12, -100, 0, -50000>,
    ParamType<12, -100, 1,      0>,
    ParamType<12, -100, 1,    100>,
    ParamType<12, -100, 1,   -100>,
    ParamType<12, -100, 1,  50000>,
    ParamType<12, -100, 1, -50000>,
    ParamType<12,  -60, 0,      0>,
    ParamType<12,  -60, 0,    100>,
    ParamType<12,  -60, 0,   -100>,
    ParamType<12,  -60, 0,  50000>,
    ParamType<12,  -60, 0, -50000>,
    ParamType<12,  -60, 1,      0>,
    ParamType<12,  -60, 1,    100>,
    ParamType<12,  -60, 1,   -100>,
    ParamType<12,  -60, 1,  50000>,
    ParamType<12,  -60, 1, -50000>,
    ParamType<12,  -18, 0,      0>,
    ParamType<12,  -18, 0,    100>,
    ParamType<12,  -18, 0,   -100>,
    ParamType<12,  -18, 0,  50000>,
    ParamType<12,  -18, 0, -50000>,
    ParamType<12,  -18, 1,      0>,
    ParamType<12,  -18, 1,    100>,
    ParamType<12,  -18, 1,   -100>,
    ParamType<12,  -18, 1,  50000>,
    ParamType<12,  -18, 1, -50000>,
    ParamType<16, -100, 0,      0>,
    ParamType<16, -100, 0,    100>,
    ParamType<16, -100, 0,   -100>,
    ParamType<16, -100, 0,  50000>,
    ParamType<16, -100, 0, -50000>,
    ParamType<16, -100, 1,      0>,
    ParamType<16, -100, 1,    100>,
    ParamType<16, -100, 1,   -100>,
    ParamType<16, -100, 1,  50000>,
    ParamType<16, -100, 1, -50000>,
    ParamType<16,  -60, 0,      0>,
    ParamType<16,  -60, 0,    100>,
    ParamType<16,  -60, 0,   -100>,
    ParamType<16,  -60, 0,  50000>,
    ParamType<16,  -60, 0, -50000>,
    ParamType<16,  -60, 1,      0>,
    ParamType<16,  -60, 1,    100>,
    ParamType<16,  -60, 1,   -100>,
    ParamType<16,  -60, 1,  50000>,
    ParamType<16,  -60, 1, -50000>,
    ParamType<16,  -18, 0,      0>,
    ParamType<16,  -18, 0,    100>,
    ParamType<16,  -18, 0,   -100>,
    ParamType<16,  -18, 0,  50000>,
    ParamType<16,  -18, 0, -50000>,
    ParamType<16,  -18, 1,      0>,
    ParamType<16,  -18, 1,    100>,
    ParamType<16,  -18, 1,   -100>,
    ParamType<16,  -18, 1,  50000>,
    ParamType<16,  -18, 1, -50000>
    //[[[end]]]
    >;

template <typename T>
class DecoderTest : public ::testing::Test
{
public:
    static inline Signal test_audio_;
    static inline std::vector<uint8_t> test_data_;

    static constexpr int kSymbolDuration = std::tuple_element_t<0, T>::value;
    static constexpr int kNoise_dB       = std::tuple_element_t<1, T>::value;
    static constexpr int kInvert         = std::tuple_element_t<2, T>::value;
    static constexpr int kMismatchPPM    = std::tuple_element_t<3, T>::value;

    static constexpr int kSymbolRate     = kSampleRate / kSymbolDuration;
    static constexpr int kPacketSize     = 256;
    static constexpr int kBlockSize      = 1024;
    Decoder<kSampleRate, kSymbolRate, kPacketSize, kBlockSize> decoder_;

    void DebugError(Error error)
    {
        printf("  PLL freq         : %li\n"
               "  Decision phase   : %li\n"
               "  Signal power     : %li\n",
               std::lround(1000 * decoder_.pll_step()),
               std::lround(1000 * decoder_.decision_phase()),
               std::lround(1000 * decoder_.signal_power()));

        if (error == ERROR_CRC)
        {
            printf("  Packet data      :\n");

            for (uint32_t row = 0; row < kPacketSize; row += 16)
            {
                printf("    ");

                for (uint32_t col = 0; col < 16; col++)
                {
                    if (row + col >= kPacketSize)
                    {
                        break;
                    }

                    printf("%02X ", decoder_.packet_data()[row + col]);
                }

                printf("\n");
            }
        }
    }

    void ReceiveError(Error error)
    {
        switch (error)
        {
        case ERROR_SYNC:
            printf("Error : sync\n");
            DebugError(error);
            break;

        case ERROR_CRC:
            printf("Error : CRC\n");
            DebugError(error);
            break;

        case ERROR_OVERFLOW:
            printf("Error : overflow\n");
            break;

        case ERROR_ABORT:
            printf("Error : abort\n");
            break;

        case ERROR_LENGTH:
            printf("Error : length\n");
            break;

        case ERROR_NONE:
            break;
        }
    }

    static void SetUpTestCase()
    {
        std::string bin_file = "unit_tests/data/data.bin";
        test_data_ = util::LoadBinary(bin_file);
        test_audio_ = util::LoadAudio<Signal>(bin_file,
            kSymbolRate, kPacketSize, kBlockSize);
    }

    void SetUp() override
    {
        decoder_.Init(kCRCSeed);
    }

    void Decode(float signal_level, float noise_dB, float resampling_ratio)
    {
        Signal signal = test_audio_;
        signal = util::Resample(signal, resampling_ratio);
        signal = util::AddNoise(signal, std::pow(10, noise_dB / 20));
        signal = util::Scale(signal, signal_level);
        Decode(signal);
    }

    void Decode(Signal signal)
    {
        int flash_write_delay = 0;

        ASSERT_EQ(decoder_.bytes_received(), 0);
        ASSERT_EQ(decoder_.total_size_bytes(), 0);
        ASSERT_FLOAT_EQ(decoder_.progress(), 0.0);

        // Begin decoding
        Result result;
        std::vector<uint8_t> data;
        for (auto sample : signal)
        {
            decoder_.Push(sample);

            if (flash_write_delay == 0)
            {
                result = decoder_.Process();

                if (result == RESULT_ERROR)
                {
                    ReceiveError(decoder_.error());
                    FAIL();
                }
                else if (result == RESULT_BLOCK_COMPLETE)
                {
                    const uint32_t* block = decoder_.block_data();
                    for (uint32_t i = 0; i < kBlockSize / 4; i++)
                    {
                        data.push_back(block[i] >>  0);
                        data.push_back(block[i] >>  8);
                        data.push_back(block[i] >> 16);
                        data.push_back(block[i] >> 24);
                    }
                    flash_write_delay = kSampleRate * kFlashWriteTime;
                    ASSERT_EQ(data.size(), decoder_.bytes_received());
                }
            }
            else
            {
                flash_write_delay--;
            }
        }

        ASSERT_EQ(result, RESULT_END);
        ASSERT_EQ(data.size(), decoder_.total_size_bytes());
        ASSERT_FLOAT_EQ(decoder_.progress(), 1.0);

        // Compare the received data to the bin file
        ASSERT_GE(data.size(), test_data_.size());

        for (uint32_t i = 0; i < data.size(); i++)
        {
            uint8_t expected = (i < test_data_.size()) ?
                test_data_[i] : kFillByte;
            ASSERT_EQ(data[i], expected) << "at i = " << i;
        }
    }
};

TYPED_TEST_CASE(DecoderTest, ParamTypeList);

TYPED_TEST(DecoderTest, Decode)
{
    float scale = this->kInvert ? -1 : 1;
    float noise_dB = this->kNoise_dB;
    float resampling_ratio = 1 + this->kMismatchPPM * 1e-6;
    this->Decode(scale, noise_dB, resampling_ratio);
}



class HangTest : public ::testing::Test
{
public:
    Decoder<kSampleRate, kSampleRate / 6, 256, 1024> decoder_;

    void SetUp() override
    {
        decoder_.Init(kCRCSeed);
    }

    Result Run(std::string command)
    {
        auto signal = util::LoadAudioFromCommand<Signal>(command);

        int flash_write_delay = 0;
        Result result;

        for (auto sample : signal)
        {
            decoder_.Push(sample);

            if (flash_write_delay == 0)
            {
                result = decoder_.Process();
                if (result == RESULT_BLOCK_COMPLETE)
                {
                    flash_write_delay = kSampleRate * kFlashWriteTime;
                }
            }
            else
            {
                flash_write_delay--;
            }
        }

        return result;
    }
};

TEST_F(HangTest, Sync)
{
    // Make sure that the decoder errors out instead of hanging when the
    // carrier sync is interrupted by silence.
    auto result = Run("PYTHONPATH=. python3 unit_tests/hang.py sync");
    ASSERT_EQ(result, RESULT_ERROR);
}

TEST_F(HangTest, Prealignment)
{
    // Make sure that the decoder errors out instead of hanging when the
    // alignment sequence is interrupted by silence.
    auto result = Run("PYTHONPATH=. python3 unit_tests/hang.py prealign");
    ASSERT_EQ(result, RESULT_ERROR);
}

TEST_F(HangTest, Alignment)
{
    // Make sure that the decoder errors out instead of hanging when the
    // alignment sequence is interrupted by silence.
    auto result = Run("PYTHONPATH=. python3 unit_tests/hang.py align");
    ASSERT_EQ(result, RESULT_ERROR);
}

TEST_F(HangTest, Write)
{
    // Make sure that the decoder errors out instead of hanging when a
    // block is followed by silence.
    auto result = Run("PYTHONPATH=. python3 unit_tests/hang.py write");
    ASSERT_EQ(result, RESULT_ERROR);
}

}
