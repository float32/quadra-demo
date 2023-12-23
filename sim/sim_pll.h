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

#pragma once

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <climits>
#include <cstdint>
#include <vector>
#include <cmath>
#include <random>
#include <unistd.h>

#include "sim/vcd-writer/vcd_writer.h"
#include "sim/vcd_var.h"
#include "unit_tests/util.h"
#include "quadra/inc/demodulator.h"

namespace quadra::sim::pll
{

using namespace vcd;

constexpr uint32_t kSampleRate = 48000;
constexpr uint32_t kSymbolRate = 9600;
constexpr uint32_t kSymbolDuration = kSampleRate / kSymbolRate;

using Symbols = std::vector<uint8_t>;

inline void RunSim(std::string vcd_file, Symbols symbols, double timestep)
{
    VCDWriter vcd{vcd_file,
        makeVCDHeader(TimeScale::ONE, TimeScaleUnit::us, utils::now())};
    VCDIntegerVar<1> v_time_extend(vcd, "top", "time_extend");

    VCDIntegerVar<4> v_symbol(vcd, "top", "symbol");
    VCDFixedPointVar<1, 4> v_i_in(vcd, "top", "i_in");
    VCDFixedPointVar<1, 4> v_q_in(vcd, "top", "q_in");
    VCDFixedPointVar<2, 12> v_signal(vcd, "top", "signal");
    VCDFixedPointVar<2, 12> v_ref_phase(vcd, "top", "ref_phase");
    VCDFixedPointVar<2, 12> v_pll_phase(vcd, "top", "pll_phase");
    VCDFixedPointVar<2, 16> v_exact_error(vcd, "top", "exact_error");
    VCDFixedPointVar<4, 16> v_error(vcd, "top", "error");
    VCDFixedPointVar<2, 20> v_step(vcd, "top", "step");
    VCDFixedPointVar<2, 12> v_i_mod(vcd, "top", "i_mod");
    VCDFixedPointVar<2, 12> v_q_mod(vcd, "top", "q_mod");
    VCDFixedPointVar<2, 12> v_i_out(vcd, "top", "i_out");
    VCDFixedPointVar<2, 12> v_q_out(vcd, "top", "q_out");

    PhaseLockedLoop pll;
    CarrierRejectionFilter<kSymbolDuration> crf;
    pll.Init(1.0 / kSymbolDuration);
    crf.Init();
    double time = 0;
    double ref_phase = 0;
    double ref_phase_step = timestep * kSymbolRate * 1.0e-6;
    bool sync = true;

    for (auto symbol : symbols)
    {
        v_symbol.change(time, symbol);
        float i_in = 0.5 * (symbol & 3) - 0.75;
        float q_in = 0.5 * (symbol >> 2) - 0.75;
        v_i_in.change(time, i_in);
        v_q_in.change(time, q_in);

        while (ref_phase < 1)
        {
            float phi = 2 * M_PI * ref_phase;
            float sample = i_in * std::cos(phi) - q_in * std::sin(phi);

            float theta = 2 * M_PI * pll.phase();
            float i_osc = std::cos(theta);
            float q_osc = -std::sin(theta);
            float i_mod = 2 * sample * i_osc;
            float q_mod = 2 * sample * q_osc;
            Vector v_out = crf.Process({i_mod, q_mod});
            float i_out = v_out.real();
            float q_out = v_out.imag();
            float i_bar = std::clamp<int32_t>(2 * i_out + 2, 0, 3) * 0.5 - 0.75;
            float q_bar = std::clamp<int32_t>(2 * q_out + 2, 0, 3) * 0.5 - 0.75;

            if (sync)
            {
                if (symbol == 0)
                {
                    q_bar = -0.75;
                    i_bar = -0.75;
                }
                else
                {
                    sync = false;
                }
            }

            float error = i_out * q_bar - i_bar * q_out;
            float exact_error = pll.phase() - ref_phase;
            exact_error -= std::floor(exact_error + 0.5f);

            pll.ProcessError(error);

            v_signal.change(time, sample);
            v_ref_phase.change(time, ref_phase);
            v_pll_phase.change(time, pll.phase());
            v_exact_error.change(time, exact_error);
            v_error.change(time, pll.error());
            v_step.change(time, pll.step());
            v_i_mod.change(time, i_mod);
            v_q_mod.change(time, q_mod);
            v_i_out.change(time, i_out);
            v_q_out.change(time, q_out);

            pll.Step();
            ref_phase += ref_phase_step;
            time += timestep;
        }

        ref_phase -= std::floor(ref_phase);
    }

    v_time_extend.change(time, 0);
    vcd.flush();
}

inline Symbols GenerateTestData(void)
{
    auto rng = std::minstd_rand();
    auto dist = std::uniform_int_distribution(0, 15);
    Symbols symbols;

    // Sync sequence
    for (int i = 0; i < kSymbolRate / 2; i++)
    {
        symbols.push_back(0);
    }

    // Random symbols to mimic scrambled data
    for (int i = 0; i < kSymbolRate * 5; i++)
    {
        symbols.push_back(dist(rng));
    }

    return symbols;
}

inline void Simulate(std::string vcd_file)
{
    Symbols symbols = GenerateTestData();
    static constexpr float kResamplingRatio = 1.02f;
    double timestep = 1.0e6 / (kSampleRate * kResamplingRatio);
    RunSim(vcd_file, symbols, timestep);

    std::cout << "Done" << std::endl;
}

}
