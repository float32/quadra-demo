# MIT License
#
# Copyright 2023 Tyler Coy
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import argparse
import sys
from quadra.encoder import Encoder, Modulator, ALIGNMENT_PLACEHOLDER

class TestDataEncoder(Encoder):
    def encode(self, symbols):
        result = []
        result += self._encode_intro()
        result += self._encode_resync()
        result += [ALIGNMENT_PLACEHOLDER]
        result += symbols
        return result

def modulate(symbols, sample_rate, symbol_rate):
    assert(all((s < 16 for s in symbols)))
    encoder = TestDataEncoder(symbol_rate, 256, 0)
    symbols = encoder.encode(symbols)
    modulator = Modulator(sample_rate, symbol_rate)
    return modulator.modulate(symbols)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=
        'Reads a sequence of symbols from stdin and writes modulated audio to '
        'stdout. Input symbols are bytes. Output samples are signed 16-bit '
        'little-endian.')
    parser.add_argument('sample_rate', type=int)
    parser.add_argument('symbol_rate', type=int)
    args = parser.parse_args()

    input_file = sys.stdin.buffer
    symbols = input_file.read()

    samples = modulate(symbols, args.sample_rate, args.symbol_rate)

    out = sys.stdout.buffer
    if sys.byteorder == 'big':
        samples.byteswap();
    out.write(samples.tobytes())
