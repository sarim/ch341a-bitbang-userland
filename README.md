## ch341a-bitbang-userland

This code talks to ch341a usb interfacing chip via libusb from userland. It includes some basic functions for implementing protocols (ex: SPI) by bitbanging or use as gpio.

This project is heavily inspired by and uses parts from these projects, https://github.com/Trel725/chavrprog, https://github.com/setarcos/ch341prog, https://github.com/gschorcht/spi-ch341-usb, https://github.com/gschorcht/i2c-ch341-usb


The goal of this project is to make a bare minimum simple code for ch341a interfacing. So it can be used as a bootstraping code for any project with ch341a.

Two example is provided in `main.c`

1. A simple gpio led blink example
2. SPI communication with AVR chip implemented by bitbang