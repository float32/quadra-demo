#!/usr/bin/env python3
#
# MIT License
#
# Copyright 2021 Tyler Coy
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

import unittest
from intelhex import IntelHex
import tempfile
import subprocess
import random
import zlib
import io
import codecs
import itertools

import quadra.encoder as encoder



class TestParsing(unittest.TestCase):

    def test_parse_size(self):
        for size in [4, 5, 16, 50, 256, 1000]:
            with self.subTest(size=size):
                self.assertEqual(size, encoder.parse_size(str(size)))
                self.assertEqual(size * 1024,
                    encoder.parse_size(str(size) + 'K'))
                self.assertEqual(size * 1024,
                    encoder.parse_size(str(size) + 'k'))



class TestBlocks(unittest.TestCase):

    def test_fill(self):
        data = bytes([0] * 1000)
        for (size, fill) in itertools.product([16, 256, 1024], range(256)):
            with self.subTest(block_size=size, fill=fill):
                blocks = encoder.Blocks(data, size, fill)
                blocks = b''.join(blocks)
                # Returned data should be padded to a multiple of block size
                self.assertEqual(len(blocks) % size, 0)
                # The padding should contain only the fill value
                padding = blocks[len(data):]
                self.assertGreater(len(padding), 0)
                self.assertEqual(bytes([fill] * len(padding)), padding)

    def test_block_data(self):
        data = bytes([random.randint(0, 0xFF) for i in range(1000)])
        for size in [1, 4, 10, 16, 100, 256, 1000, 4096]:
            with self.subTest(block_size=size):
                blocks = encoder.Blocks(data, size, 0xFF)
                blocks = b''.join(blocks)
                # The block data should contain exactly the input data followed
                # by zero or more padding bytes
                self.assertEqual(data, blocks[:len(data)])



class TestPages(unittest.TestCase):

    def test_page_data(self):
        data = bytes([random.randint(0, 0xFF) for i in range(10000)])
        block_size = 4
        for size in [4, 12, 16, 100, 256, 1000, 4096]:
            with self.subTest(page_size=size):
                flash_spec = ['{}:10'.format(size)]
                pages = encoder.Pages(data, flash_spec, 0, block_size)
                pages = b''.join((page for (time, page) in pages))
                # The page data should contain exactly the input data
                self.assertEqual(data, pages)

    def test_page_layout(self):
        data = bytes([random.randint(0, 0xFF) for i in range(10000)])
        sizes = [12, 16, 100, 256, 1000]
        block_size = 4
        for (size_a, size_b, size_c) in itertools.permutations(sizes, 3):
            spec = '{}:10:1 {}:20:2 {}:30'.format(size_a, size_b, size_c)
            with self.subTest(flash_spec=spec):
                pages = list(encoder.Pages(data, spec.split(), 0, block_size))
                addr = 0
                self.assertEqual(pages[0], (10, data[addr : addr + size_a]))
                addr += size_a
                self.assertEqual(pages[1], (20, data[addr : addr + size_b]))
                addr += size_b
                self.assertEqual(pages[2], (20, data[addr : addr + size_b]))
                addr += size_b
                for page in pages[3:]:
                    self.assertEqual(page, (30, data[addr : addr + size_c]))
                    addr += size_c



if __name__ == '__main__':
    unittest.main()
