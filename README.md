# quadra-demo

https://github.com/float32/quadra-demo

Implementation examples, unit tests, and simulations for
[quadra](https://github.com/float32/quadra).

---

## Implementation examples

There's currently a single implementation example, written for the
[STM32F4 Discovery](https://www.st.com/en/evaluation-tools/stm32f4discovery.html)
board. See `example/README.md`.


## Unit tests

A suite of unit tests for the decoder and encoder can be found in the
`unit_tests` directory. They depend on:

- [googletest](https://github.com/google/googletest)
- [zlib](https://www.zlib.net/)
- [libsamplerate](http://www.mega-nerd.com/SRC/index.html)

Run the tests using these commands:

    make check
    make py-check


## Simulation

There are simulations of the decoder, demodulator, and PLL under the `sim`
directory. Run them with the commands:

    make run-sim-decoder
    make run-sim-demodulator
    make run-sim-pll

You can view simulation traces using [GTKWave](http://gtkwave.sourceforge.net/),
for which project files are provided.


## Licensing

This project contains a few libraries with varying licenses.

- [**quadra**](https://github.com/float32/quadra)
  is licensed MIT, copyright Émilie Gillet and Tyler Coy.
- [**boilermake**](https://github.com/float32/boilermake)
  is licensed GPL-3.0, copyright Dan Moulding, Alan T. DeKok
- [**vcd-writer**](https://github.com/favorart/vcd-writer)
  is licensed MIT, copyright Kirill Golikov.
- [**CMSIS**](https://github.com/ARM-software/CMSIS_5)
  is licensed Apache-2.0, copyright Arm Limited.
- [**STM32F4 HAL**](https://github.com/STMicroelectronics/STM32CubeF4)
  is licensed BSD-3-Clause, copyright STMicroelectronics.
- Everything else is licensed MIT, copyright Tyler Coy.

---

Copyright 2023 Tyler Coy

https://www.alrightdevices.com

https://github.com/float32