<!--
SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG
SPDX-License-Identifier: MIT
-->

# Interfacing OPTIGA&trade; Authenticate NBT with Raspberry Pi

[![Contributor Covenant](https://img.shields.io/badge/Contributor%20Covenant-2.1-4baaaa.svg)](CODE_OF_CONDUCT.md)
[![REUSE Compliance Check](https://github.com/Infineon/optiga-nbt-lib-c/actions/workflows/linting-test.yml/badge.svg?branch=main)](https://github.com/Infineon/optiga-nbt-lib-c/actions/workflows/linting-test.yml)
[![CMake Build](https://github.com/Infineon/optiga-nbt-lib-c/actions/workflows/build-test.yml/badge.svg?branch=main)](https://github.com/Infineon/optiga-nbt-lib-c/actions/workflows/cmake-single-platform.yml)

This guide provides a step-by-step process for porting the OPTIGA™ Authenticate NBT's Host Library for C to Raspberry Pi OS, utilizing the I2C interface.

## Overview

The guide outlines the process of setting up the development environment, confirguring the Raspberry Pi and adapting the host code for compatibility.

Refer to the [OPTIGA&trade; Authenticate NBT Host Library for C: User guide](https://github.com/Infineon/optiga-nbt-lib-c/blob/main/docs/userguide.md) repository to understand the features and functionality of the host library, including an architecture overview and descriptions of the host library's components.

## Setup and requirements
This section contains information on how to setup and interface the OPTIGA™ Authenticate NBT with Raspberry Pi.

### Hardware requirements
1. Raspberry Pi 4/5
2. OPTIGA&trade; Authenticate NBT Development Shield

**Table 1. Mapping of the OPTIGA&trade; Authenticate NBT Development Shield's pins to Raspberry Pi**

| OPTIGA&trade; Authenticate NBT Development Shield | Raspberry Pi | Function |
| ------------------------------------------------- | --------------------- | -------- |
| SDA                           | GPIO 2        | I2C data                   |
| SCL                           | GPIO 3        | I2C clock                  |
| IRQ                           | NC        | Interrupt                  |
| 3V3                           | 3V3                     | Power and pad supply (3V3) |
| GND                           | GND                     | Common ground reference    |

The Raspberry Pi's pins need to be connected to the OPTIGA&trade; Authenticate NBT Development Shield as shown in Table 1.

### Modify confirguration file
To change the I2C speed and baudrate on a Raspberry Pi, you need to modify the `config.txt` file. The I2C interface on the Raspberry Pi can be configured to operate at different speeds by setting appropriate parameters in this file.
1. Open the `config.txt` file located in the `/boot` directory.
```sh
sudo nano /boot/config.txt
```
2. To set the I2C speed, you need to add or modify the dtparam entry for the I2C bus. The parameter `i2c_arm_baudrate` is used to set the baud rate for the ARM I2C interface.

Note: The I2C clock frequency cannot be changed dynamically in Raspberry Pi. So setting the clock frequency using ```ifx_i2c_set_clock_frequency``` will remain invalid and returns an error.
```sh
# Enable I2C interface
dtparam=i2c_arm=on

#Set I2C speed
dtparam=i2c_arm_baudrate=100000
```
3. After saving the confirguration file, reboot the system for changes to take effect.

### Toolset
`CMake`, `GCC` and `Make` tools are required for compiling and building software projects from source on Linux platform..

```sh
#Update the package list first
sudo apt-get update

#Install the toolset
sudo apt-get install cmake gcc make
```

## CMake build system

To build this project as a library, configure CMake and use `cmake --build` to perform the compilation.
Here are the detailed steps for compiling and installing as library:

1. Open a terminal and clone the repository from GitHub:
```sh
git clone <repository_url>
```
2. Change to the directory where the library is located:
```sh
cd path/to/repository
```
3. Now configuring the build system with CMake:
```sh
# Create a build folder in the root path 
mkdir build
cd build

# Run CMake to configure the build system
cmake -S ..

# Build the code
cmake --build .

# Install the compiled library to the system 
sudo make install
```


## Sample code

The example code in main.c demonstrates how to use the Infineon I2C protocol with a Raspberry Pi to communicate with an OPTIGA™ Authenticate NBT security chip using the GP T=1' protocol. 

The code includes initialization of I2C communication, logging, and protocol handling and provides an example of how to send and receive data from the security chip.

The command {0x00, 0xA4, 0x04, 0x00} is a standard APDU (Application Protocol Data Unit) used in smart card communication to select an application. This command tells the OPTIGA™ Authenticate NBT security chip to prepare for subsequent operations by selecting the appropriate application or file.