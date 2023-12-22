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

#pragma once

#include <cmath>
#include <random>
#include <string>
#include <vector>
#include <cassert>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <samplerate.h>

namespace quadra::test::util
{

template <typename T>
T Resample(T signal, double ratio)
{
    if (ratio != 1.0)
    {
        uint32_t length = floor(signal.size() * ratio);
        T resampled;
        resampled.resize(length);

        SRC_DATA src_data =
        {
            .data_in           = &signal[0],
            .data_out          = &resampled[0],
            .input_frames      = static_cast<long>(signal.size()),
            .output_frames     = static_cast<long>(resampled.capacity()),
            .input_frames_used = 0,
            .output_frames_gen = 0,
            .end_of_input      = 0,
            .src_ratio         = ratio,
        };

        int converter_type = SRC_SINC_MEDIUM_QUALITY;
        int channels = 1;
        int error = src_simple(&src_data, converter_type, channels);

        if (error != 0)
        {
            throw std::runtime_error("SRC error");
        }

        resampled.resize(src_data.output_frames_gen);
        return resampled;
    }
    else
    {
        return signal;
    }
}

template <typename T>
T Scale(T signal, float level)
{
    if (level != 1.f)
    {
        for (auto& sample : signal)
        {
            sample *= level;
        }
    }

    return signal;
}

template <typename T>
T AddOffset(T signal, float level)
{
    if (level != 1.f)
    {
        for (auto& sample : signal)
        {
            sample += level;
        }
    }

    return signal;
}

template <typename T>
T AddNoise(T signal, float noise_level)
{
    if (noise_level != 0.f)
    {
        std::minstd_rand rng;
        std::uniform_real_distribution<float> dist(-1, 1);

        for (auto& sample : signal)
        {
            sample += noise_level * dist(rng);
            sample = (sample > 1) ? 1 : (sample < -1) ? -1 : sample;
        }
    }

    return signal;
}

template <typename T>
T Clamp(T signal, float low, float high)
{
    for (auto& sample : signal)
    {
        sample = std::clamp(sample, low, high);
    }

    return signal;
}

template <typename T>
T LoadAudio(std::string file_path)
{
    std::ifstream wav_file;
    wav_file.open(file_path, std::ios::in | std::ios::binary);
    assert(wav_file.good());
    wav_file.seekg(44);
    T signal;
    for (;;)
    {
        int16_t sample = (wav_file.get() & 0xFF);
        sample |= (wav_file.get() << 8);
        if (wav_file.eof())
        {
            break;
        }
        signal.push_back(sample / 32767.f);
    }
    wav_file.close();
    return signal;
}

template <typename T>
T LoadAudioFromCommand(std::string command)
{
    auto wav_file = popen(command.c_str(), "r");
    T signal;

    for (uint32_t i = 0; i < 44; i++)
    {
        fgetc(wav_file);
    }

    for (;;)
    {
        int16_t sample = (fgetc(wav_file) & 0xFF);
        sample |= (fgetc(wav_file) << 8);
        if (feof(wav_file))
        {
            break;
        }
        signal.push_back(sample / 32767.f);
    }

    pclose(wav_file);
    return signal;
}

template <typename T>
T LoadAudio(std::string bin_file_path,
    int symbol_rate, int packet_size, int block_size,
    float write_time = 0.05f)
{
    std::stringstream ss;
    ss << "python3 quadra/encoder.py -s 48000 -t bin -o -"
        << " -i " << bin_file_path
        << " -y " << symbol_rate
        << " -p " << packet_size
        << " -b " << block_size
        << " -w " << (write_time / 2 * 1000)
        << " -f " << block_size << ":" << (write_time / 2 * 1000)
        << " -a 0";
    std::string cmd = ss.str();

    return LoadAudioFromCommand<T>(cmd);
}

inline std::vector<uint8_t> LoadBinary(std::string file_path)
{
    std::ifstream bin_file;
    bin_file.open(file_path, std::ios::in | std::ios::binary);
    assert(bin_file.good());
    std::vector<uint8_t> bin_data;
    for (;;)
    {
        uint32_t byte = bin_file.get();
        if (bin_file.eof())
        {
            break;
        }
        bin_data.push_back(byte);
    }
    bin_file.close();
    return bin_data;
}

}
