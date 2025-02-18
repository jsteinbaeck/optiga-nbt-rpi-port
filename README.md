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

Note: The I2C clock frequency cannot be changed dynamically in Raspberry Pi with i2c-dev driver. So setting the clock frequency using ```ifx_i2c_set_clock_frequency``` will not have any effect and returns success.
```sh
# Enable I2C interface
dtparam=i2c_arm=on

#Set I2C speed
dtparam=i2c_arm_baudrate=400000
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
## Build dependent library

The application relies on ```optiga-nbt-lib-c``` which provides essential services and APIs that our application will leverage to perform its tasks. Therefore, the first step in our project is to ensure that the host library is built and functioning correctly.

Steps to build the host library as a static library is available [here](https://github.com/Infineon/optiga-nbt-lib-c/blob/main/docs/userguide.md#build-as-library).

After configuring and buildng the code, install the compiled library to the system:
```sh
sudo make install
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


## Example

This example code demonstrates how to use the Infineon I2C protocol with a Raspberry Pi to communicate with an OPTIGA™ Authenticate NBT security chip using the GP T=1' protocol. 

The code includes initialization of I2C communication, logging, and protocol handling and provides an example of how to send and receive data from the security chip.

The command {0x00, 0xA4, 0x04, 0x00} is a standard APDU (Application Protocol Data Unit) used in smart card communication to select an application. This command tells the OPTIGA™ Authenticate NBT security chip to prepare for subsequent operations by selecting the appropriate application or file.

```c
#include "infineon/ifx-error.h"
#include "infineon/ifx-protocol.h"
#include "infineon/ifx-utils.h"
#include "infineon/ifx-t1prime.h"
#include "infineon/nbt-cmd.h"
#include "infineon/i2c-rpi.h"
#include "infineon/logger-printf.h"

/* Required for I2C */
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>


/* NBT slave address */
#define NBT_DEFAULT_I2C_ADDRESS 0x18U
#define RPI_I2C_FILE  "/dev/i2c-1"
#define LOG_TAG "NBT example"

#define RPI_I2C_OPEN_FAIL   (-1)
#define RPI_I2C_INIT_FAIL   (-2)
#define OPTIGA_NBT_ERROR    (-3)

int main()
{
    /** GP T=1' I2C protocol - PSoC&trade;6 Host MCU */
    // Protocol to handle the GP T=1' I2C protocol communication with tag
    ifx_protocol_t gp_i2c_protocol;

    // Protocol to handle communication with Raspberry PI I2C driver
    ifx_protocol_t driver_adapter;
    /* Initialize protocol driver layer here with I2C implementation.
    Note: Does not work without initialized driver layer for I2C. */

    /* Logger object */
    ifx_logger_t logger_implementation;

    // code placeholder
    ifx_status_t status;

    /* I2C file descriptor */
    int i2c_fd;

    /* Initialize logging */
    status = logger_printf_initialize(ifx_logger_default);
    if (ifx_error_check(status))
    {
        goto ret;
    }

    status = ifx_logger_set_level(ifx_logger_default, IFX_LOG_DEBUG);
    if (ifx_error_check(status))
    {
        goto ret;
    }

    /* Open the I2C device */
    if ((i2c_fd = open(RPI_I2C_FILE, O_RDWR)) == -1)
    {
        ifx_logger_log(ifx_logger_default, LOG_TAG, IFX_LOG_ERROR, "Failed to open I2C character device");
        status = RPI_I2C_OPEN_FAIL;
        goto ret;
    }

    /* Initialize RPI I2c driver adaptor */
    // I2C driver adapter
    status = i2c_rpi_initialize(&driver_adapter, i2c_fd, NBT_DEFAULT_I2C_ADDRESS);
    if (ifx_error_check(status))
    {
        ifx_logger_log(ifx_logger_default, LOG_TAG, IFX_LOG_ERROR, "Could not initialize I2C driver adapter");
        status = RPI_I2C_INIT_FAIL;
        goto exit;
    }

    // Use GP T=1' protocol channel as a interface to communicate with the OPTIGA&trade; Authenticate NBT
    status = ifx_t1prime_initialize(&gp_i2c_protocol, &driver_adapter);
    if (status != IFX_SUCCESS)
    {
        goto exit;
    }

    ifx_protocol_set_logger(&gp_i2c_protocol, ifx_logger_default);
    status = ifx_protocol_activate(&gp_i2c_protocol, NULL, NULL);
    if (status != IFX_SUCCESS)
    {
        goto cleanup;
    }

    // Exchange data with the secure element
    uint8_t data[] = {0x00u, 0xa4u, 0x04u, 0x00u};
    uint8_t *response = NULL;
    size_t response_len = 0u;
    status = ifx_protocol_transceive(&gp_i2c_protocol, data, sizeof(data), &response, &response_len);
    if (response != NULL) free(response);
    if (status != IFX_SUCCESS)
    {
        goto cleanup;
    }


cleanup:

    // Perform cleanup of full protocol stack
    ifx_protocol_destroy(&gp_i2c_protocol);

exit:
    // Close the File 
    close(i2c_fd);

ret:
    return status;
}

```

Save the file (main.c) and execute the following command to compile the example.

```
gcc main.c -l:liboptiga-nbt-rpi-port.a  -l:libhsw-apdu-protocol.a -l:libhsw-logger.a -l:libhsw-apdu.a -l:libhsw-protocol.a -l:libhsw-t1prime.a -l:libhsw-utils.a -l:libhsw-ndef.a -l:libhsw-crc.a -l:libhsw-ndef-bp.a -l:libhsw-apdu-nbt.a -l:libhsw-error.a -o main
```

This command will compile the main.c and links the nbt-c libraries and the nbt-rpi-port library to create final execute `main`. Once executed, you will get the following output.

**Figure 1. Example code output**
![](images/nbt-rpi-demo-output.png)

