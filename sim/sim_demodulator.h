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

namespace quadra::sim::demodulator
{

using namespace vcd;

constexpr uint32_t kSampleRate = 48000;
constexpr uint32_t kSymbolRate = 8000;

using Symbols = std::vector<uint8_t>;
using Signal = std::vector<float>;

inline Symbols RunSim(std::string vcd_file, Signal signal,
    Symbols expected_symbols, double timestep)
{
    VCDWriter vcd{vcd_file,
        makeVCDHeader(TimeScale::ONE, TimeScaleUnit::us, utils::now())};
    VCDIntegerVar<1> v_time_extend(vcd, "top", "time_extend");

    // Demodulator vars
    VCDFixedPointVar<4, 16> v_dm_in(vcd, "top", "in");
    VCDIntegerVar<4> v_dm_state(vcd, "top", "state");
    VCDIntegerVar<4> v_dm_symbol(vcd, "top", "symbol");
    VCDIntegerVar<1> v_dm_decide(vcd, "top", "decide");
    VCDFixedPointVar<4, 16> v_dm_power(vcd, "top", "power");
    VCDFixedPointVar<2, 16> v_dm_dec_ph(vcd, "top", "dec_phase");
    VCDFixedPointVar<4, 16> v_dm_agc(vcd, "top", "agc");

    // PLL vars
    VCDFixedPointVar<2, 16> v_pll_phase(vcd, "top", "pll_phase");
    VCDFixedPointVar<2, 16> v_pll_error(vcd, "top", "pll_error");
    VCDFixedPointVar<1, 20> v_pll_step(vcd, "top", "pll_step");
    VCDFixedPointVar<4, 16> v_pll_crfi_out(vcd, "top", "I");
    VCDFixedPointVar<4, 16> v_pll_crfq_out(vcd, "top", "Q");

    // Correlator vars
    VCDFixedPointVar<8, 16> v_corr_out(vcd, "top", "correlation");

    // Analysis vars
    VCDIntegerVar<4> v_dm_expected(vcd, "top", "expected");
    VCDIntegerVar<1> v_dm_match(vcd, "top", "match");

    Demodulator<kSampleRate, kSymbolRate> dm;
    dm.Init();

    // Begin decoding
    double time = 0;
    Symbols received;
    auto expected = expected_symbols.begin();
    for (auto sample : signal)
    {
        v_dm_in.change(time, sample);

        uint8_t symbol;
        float pll_phase = dm.pll_phase();

        if (dm.Process(symbol, sample))
        {
            received.push_back(symbol);
            v_dm_symbol.change(time, symbol);
            v_dm_expected.change(time, *expected);
            v_dm_match.change(time, symbol == *expected);
            expected++;
        }

        v_dm_state.change(time, dm.state());
        v_dm_decide.change(time, dm.decide());
        v_dm_power.change(time, dm.signal_power());
        v_dm_dec_ph.change(time, dm.decision_phase());
        v_dm_agc.change(time, dm.agc());

        v_pll_phase.change(time, pll_phase);
        v_pll_error.change(time, dm.pll_error());
        v_pll_step.change(time, dm.pll_step());
        v_pll_crfi_out.change(time, dm.recovered_i());
        v_pll_crfq_out.change(time, dm.recovered_q());
        v_corr_out.change(time, dm.correlation());

        time += timestep;
    }

    v_time_extend.change(time, 0);
    vcd.flush();

    return received;
}

inline Symbols GenerateTestData(void)
{
    auto rng = std::minstd_rand();
    auto dist = std::uniform_int_distribution(0, 15);
    Symbols symbols;

    // Random symbols to mimic scrambled data
    for (int i = 0; i < kSymbolRate * 5; i++)
    {
        symbols.push_back(dist(rng));
    }

    return symbols;
}

// Like popen(), but returns two FILE*: child's stdin and stdout, respectively.
// https://stackoverflow.com/a/64359731
inline std::pair<FILE*, FILE*> popen2(std::string command)
{
    // pipes[0]: parent writes, child reads (child's stdin)
    // pipes[1]: child writes, parent reads (child's stdout)
    int pipes[2][2];
    int result;
    result = pipe(pipes[0]);
    result = pipe(pipes[1]);
    (void)result;

    if (fork() > 0)
    {
        // parent
        close(pipes[0][0]);
        close(pipes[1][1]);

        return {fdopen(pipes[0][1], "w"), fdopen(pipes[1][0], "r")};
    }
    else
    {
        // child
        close(pipes[0][1]);
        close(pipes[1][0]);

        dup2(pipes[0][0], STDIN_FILENO);
        dup2(pipes[1][1], STDOUT_FILENO);

        execl("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);

        exit(1);
    }
}

inline Signal Modulate(Symbols symbols)
{
    std::stringstream ss;
    ss << "python3 sim/modulate.py " << kSampleRate << " " << kSymbolRate;
    auto [out, in] = popen2(ss.str());

    for (auto symbol : symbols)
    {
        fputc(symbol, out);
    }

    fclose(out);
    Signal signal;

    for (;;)
    {
        int16_t sample = (fgetc(in) & 0xFF);
        sample |= (fgetc(in) << 8);
        if (feof(in))
        {
            break;
        }
        signal.push_back(sample / 32767.f);
    }

    // Append half a symbol's worth of null samples so that the demodulator
    // will have enough time to produce the final symbol.
    for (int i = 0; i < kSampleRate / kSymbolRate / 2; i++)
    {
        signal.push_back(0);
    }

    fclose(in);
    return signal;
}

inline void Simulate(std::string vcd_file)
{
    Symbols expected = GenerateTestData();

    Signal signal = Modulate(expected);

    // Resample and add noise
    static constexpr float kResamplingRatio = 1.02;
    static constexpr float kScale = 1.0;
    static constexpr float kNoise_dB = -18;
    signal = test::util::Resample(signal, kResamplingRatio);
    signal = test::util::Scale(signal, kScale);
    signal = test::util::AddNoise(signal, std::pow(10, kNoise_dB / 20));

    double timestep = 1.0e6 / (kSampleRate * kResamplingRatio);
    Symbols received = RunSim(vcd_file, signal, expected, timestep);

    if (received == expected)
    {
        std::cout << "Success!" << std::endl;
    }
    else
    {
        for (int i = 0; i < expected.size(); i++)
        {
            int32_t x = expected[i];

            if (i < received.size())
            {
                if (expected[i] != received[i])
                {
                    int32_t y = received[i];
                    std::cout << "Index " << i << ": expected " << x
                        << ", received " << y << std::endl;
                    break;
                }
            }
            else
            {
                std::cout << "Index " << i << ": expected " << x
                    << ", reception terminated" << std::endl;
                break;
            }
        }

        throw std::runtime_error("Data mismatch");
    }
}

}
