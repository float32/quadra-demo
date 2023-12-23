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

#include <gtest/gtest.h>
#include <cmath>
#include "quadra/inc/pll.h"

namespace quadra::test::pll
{

constexpr double kTestDuration = 5;
constexpr double kSampleRate = 48000;

static const double kCarrierFrequencies[] =
{
    1.0 / 16.0,
    1.0 / 12.0,
    1.0 / 10.0,
    1.0 / 8.0,
    1.0 / 6.0,
    1.0 / 5.0,
};

static const double kMismatchFactors[] =
{
    1.0,
    0.99999,
    1.00001,
    0.99,
    1.01,
    0.98,
    1.02,
    0.95,
    1.05,
};

class PLLTest : public ::testing::TestWithParam<std::tuple<double, double>>
{
protected:
    double freq_;
    PhaseLockedLoop pll_;

    void SetUp() override
    {
        double carrier;
        double mismatch;

        std::tie(carrier, mismatch) = GetParam();
        pll_.Init(carrier);

        freq_ = carrier * mismatch;
    }
};

double PhaseDifference(double a, double b)
{
    return fmod(a + 1.0 - b, 1.0);
}

TEST_P(PLLTest, Lock)
{
    double i_in = -0.75;
    double q_in = -0.75;

    for (int32_t j = 0; j < kTestDuration * kSampleRate; j++)
    {
        double t = j / kSampleRate;
        double input_phase = fmod(freq_ * j, 1.0);

        // Normally we would multiply the input signal by sin and cos of the
        // PLL phase and then lowpass to extract the DC component, but since
        // we already know the input signal's phase, we can calculate the DC
        // component directly by using trigonometric product-to-sum identities.
        double delta = 2 * M_PI * PhaseDifference(pll_.phase(), input_phase);
        double i_out = i_in * cos(delta) + q_in * sin(delta);
        double q_out = -i_in * sin(delta) + q_in * cos(delta);
        double phase_error = i_out * q_in - i_in * q_out;

        if (t > 0.25)
        {
            delta = fmod(delta / (2 * M_PI) + 0.5, 1.0) - 0.5;
            ASSERT_NEAR(delta / (2 * M_PI), 0, 0.001)
                << "j = " << j << ", t = " << t;
        }

        pll_.ProcessError(phase_error);
        pll_.Step();
    }
}

INSTANTIATE_TEST_CASE_P(Freq, PLLTest, ::testing::Combine(
    ::testing::ValuesIn(kCarrierFrequencies),
    ::testing::ValuesIn(kMismatchFactors)
    ));

}
