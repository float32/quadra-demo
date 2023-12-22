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

TARGET := sim_decoder
SOURCES := \
	sim/sim_decoder.cpp \
	sim/vcd-writer/*.cpp \

TGT_DEFS :=
CPPFLAGS := -g -O3 -iquote .
TGT_CXXFLAGS := $(CPPFLAGS) -std=c++17
TGT_LDLIBS := -lsamplerate

VCD_FILE := $(TARGET_DIR)/sim-decoder.vcd

.PHONY: sim-decoder
sim-decoder: $(TARGET_DIR)/$(TARGET)

.PHONY: $(VCD_FILE)
$(VCD_FILE): $(TARGET_DIR)/$(TARGET)
	$< $@ unit_tests/data/data.bin

.PHONY: run-sim-decoder
run-sim-decoder: $(VCD_FILE)

define TGT_POSTCLEAN
	$(RM) $(VCD_FILE)
endef
