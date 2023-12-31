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

#include <cstdint>
#include <cmath>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include <zlib.h>
#include "quadra/inc/packet.h"
#include "quadra/inc/scrambler.h"
#include "test_error_correction.h"

namespace quadra::test::packet
{

template <int packet_size, int packets_per_block>
using ParamType = std::tuple<
    std::integral_constant<int, packet_size>,
    std::integral_constant<int, packets_per_block>>;
using ParamTypeList = ::testing::Types<
    /*[[[cog
    import itertools
    lines = []
    packet_size = (4, 8, 16, 32, 52, 64, 100, 128, 252, 256, 260, 1000, 4096)
    packets_per_block = (1, 4, 7)
    tests = itertools.product(packet_size, packets_per_block)
    for (packet_size, packets_per_block) in tests:
        lines.append('ParamType<{:4}, {:2}>'
            .format(packet_size, packets_per_block))
    cog.outl(',\n'.join(lines))
    ]]]*/
    ParamType<   4,  1>,
    ParamType<   4,  4>,
    ParamType<   4,  7>,
    ParamType<   8,  1>,
    ParamType<   8,  4>,
    ParamType<   8,  7>,
    ParamType<  16,  1>,
    ParamType<  16,  4>,
    ParamType<  16,  7>,
    ParamType<  32,  1>,
    ParamType<  32,  4>,
    ParamType<  32,  7>,
    ParamType<  52,  1>,
    ParamType<  52,  4>,
    ParamType<  52,  7>,
    ParamType<  64,  1>,
    ParamType<  64,  4>,
    ParamType<  64,  7>,
    ParamType< 100,  1>,
    ParamType< 100,  4>,
    ParamType< 100,  7>,
    ParamType< 128,  1>,
    ParamType< 128,  4>,
    ParamType< 128,  7>,
    ParamType< 252,  1>,
    ParamType< 252,  4>,
    ParamType< 252,  7>,
    ParamType< 256,  1>,
    ParamType< 256,  4>,
    ParamType< 256,  7>,
    ParamType< 260,  1>,
    ParamType< 260,  4>,
    ParamType< 260,  7>,
    ParamType<1000,  1>,
    ParamType<1000,  4>,
    ParamType<1000,  7>,
    ParamType<4096,  1>,
    ParamType<4096,  4>,
    ParamType<4096,  7>
    //[[[end]]]
    >;

constexpr uint32_t kCRCSeed = 420;

template <typename T>
class PacketTest : public ::testing::Test
{
protected:
    static constexpr uint32_t kPacketSize = std::tuple_element_t<0, T>::value;
    static constexpr uint32_t kPacketsPerBlock = std::tuple_element_t<1, T>::value;
    static constexpr uint32_t kBlockSize = kPacketSize * kPacketsPerBlock;
    uint8_t data_[kPacketSize];
    uint32_t expected_crc_;
    error_correction::HammingEncoder hamming_;
    Packet<kPacketSize> packet_;
    Block<kBlockSize> block_;
    std::minstd_rand rng_;
    Scrambler scrambler_;

    void SetUp() override
    {
        rng_.seed(0);
        RandomizeData();
        packet_.Init(kCRCSeed);
        block_.Init();
        scrambler_.Init();
    }

    void RandomizeData(void)
    {
        std::uniform_int_distribution<uint8_t> dist(0);

        for (uint32_t i = 0; i < kPacketSize; i++)
        {
            data_[i] = dist(rng_);
        }

        expected_crc_ = crc32(kCRCSeed, data_, kPacketSize);

        hamming_.Encode(data_, kPacketSize);
        hamming_.Encode((expected_crc_ >>  0) & 0xFF);
        hamming_.Encode((expected_crc_ >>  8) & 0xFF);
        hamming_.Encode((expected_crc_ >> 16) & 0xFF);
        hamming_.Encode((expected_crc_ >> 24) & 0xFF);
    }

    void PushByte(uint8_t byte)
    {
        byte = scrambler_.Process(byte);
        packet_.WriteSymbol((byte >> 4) & 0xF);
        packet_.WriteSymbol((byte >> 0) & 0xF);
    }
};

TYPED_TEST_CASE(PacketTest, ParamTypeList);

TYPED_TEST(PacketTest, Valid)
{
    for (auto byte : this->data_)
    {
        ASSERT_FALSE(this->packet_.full());
        ASSERT_FALSE(this->packet_.valid());
        this->PushByte(byte);
    }

    for (uint32_t i = 0; i < 32; i += 8)
    {
        ASSERT_FALSE(this->packet_.full());
        ASSERT_FALSE(this->packet_.valid());
        this->PushByte(this->expected_crc_ >> i);
    }

    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    this->PushByte(this->hamming_.parity());
    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    this->PushByte(this->hamming_.parity() >> 8);

    ASSERT_TRUE(this->packet_.full());
    ASSERT_TRUE(this->packet_.valid());
    ASSERT_EQ(this->expected_crc_, this->packet_.calculated_crc());

    this->packet_.Reset();
    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
}

TYPED_TEST(PacketTest, Invalid)
{
    // Tamper with one byte
    this->data_[this->kPacketSize / 2] ^= 0xFF;

    for (uint32_t i = 0; i < this->kPacketSize; i++)
    {
        ASSERT_FALSE(this->packet_.full());
        ASSERT_FALSE(this->packet_.valid());
        this->PushByte(this->data_[i]);
    }

    for (uint32_t i = 0; i < 32; i += 8)
    {
        ASSERT_FALSE(this->packet_.full());
        ASSERT_FALSE(this->packet_.valid());
        this->PushByte(this->expected_crc_ >> i);
    }

    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    this->PushByte(this->hamming_.parity());
    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    this->PushByte(this->hamming_.parity() >> 8);

    ASSERT_TRUE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
    ASSERT_NE(this->expected_crc_, this->packet_.calculated_crc());

    this->packet_.Reset();
    ASSERT_FALSE(this->packet_.full());
    ASSERT_FALSE(this->packet_.valid());
}

TYPED_TEST(PacketTest, BlockFill)
{
    std::vector<uint8_t> bytes;

    for (uint32_t i = 0; i < this->kPacketsPerBlock; i++)
    {
        ASSERT_FALSE(this->block_.full());
        this->RandomizeData();
        this->packet_.Reset();
        this->scrambler_.Init();

        for (auto byte : this->data_)
        {
            this->PushByte(byte);
            bytes.push_back(byte);
        }

        this->block_.AppendPacket(this->packet_);
    }

    ASSERT_TRUE(this->block_.full());

    for (uint32_t i = 0; i < this->kBlockSize / 4; i++)
    {
        uint32_t word = this->block_.data()[i];

        #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            word = __builtin_bswap32(word);
        #endif

        ASSERT_EQ(bytes[i * 4 + 0], (word >>  0) & 0xFF);
        ASSERT_EQ(bytes[i * 4 + 1], (word >>  8) & 0xFF);
        ASSERT_EQ(bytes[i * 4 + 2], (word >> 16) & 0xFF);
        ASSERT_EQ(bytes[i * 4 + 3], (word >> 24) & 0xFF);
    }

    this->block_.Clear();
    ASSERT_FALSE(this->block_.full());
}

}
