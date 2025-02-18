// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file infineon/i2c-cyhal.h
 * \brief I2C driver wrapper for NBT framework based on Raspberry PI Linux OS.
 */
#ifndef INFINEON_I2C_CYHAL_H
#define INFINEON_I2C_CYHAL_H

#include "infineon/ifx-protocol.h"
#include "infineon/ifx-i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief IFX status code module identifier.
 */
#define LIBI2CCYHAL 0x35U

/**
 * \brief String used as source information for logging.
 */
#define I2C_CYHAL_LOG_TAG IFX_I2C_LOG_TAG

/**
 * \brief Initializes protocol object for Raspberry PI Linux OS.
 *
 * \param[in] self Protocol object to be initialized.
 * \param[in] native_instance File descriptor of the opened I2C device file.
 * \param[in] slave_address Initial I2C slave address to be used.
 * \return ifx_status_t `IFX_SUCCESS` if successful, any other value in case of error.
 */
ifx_status_t i2c_cyhal_initialize(ifx_protocol_t *self, int native_instance, uint8_t slave_address);

#ifdef __cplusplus
}
#endif

#endif // INFINEON_I2C_CYHAL_H
