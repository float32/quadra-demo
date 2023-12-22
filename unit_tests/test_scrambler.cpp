// MIT License
//
// Copyright 2023 Tyler Coy
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
#include <random>
#include <gtest/gtest.h>
#include "quadra/inc/scrambler.h"

namespace quadra::test::scrambler
{

TEST(ScramblerTest, Sequence)
{
    std::linear_congruential_engine<uint64_t, 1664525, 1013904223, 1ULL << 32>
        ref;
    ref.seed(0);

    Scrambler scrambler;
    scrambler.Init();

    for (int i = 0; i < 10e6; i++)
    {
        uint8_t expected = ref() >> 24;
        uint8_t actual = scrambler.Process(0);
        ASSERT_EQ(expected, actual);
    }
}

}
