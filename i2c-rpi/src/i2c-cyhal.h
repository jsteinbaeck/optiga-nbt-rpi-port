// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file i2c-cyhal.h
 * \brief Internal definitions for I2C driver wrapper for NBT framework based on ModusToolbox HAL.
 */
#ifndef I2C_CYHAL_H
#define I2C_CYHAL_H

#if defined(I2C_LOG_ENABLE) && (I2C_LOG_ENABLE)
/**
 * \brief Utility checking \c I2C_LOG_ENABLE macro and then wrapping to
 * actual (log) statement.
 *
 * \code
 *     CHECKED_LOG(ifx_logger_log(logger_object, source_information, \ 
 *                  logger_level, formatter, ##__VA_ARGS__));
 *
 * \param[in] statement (Log) statement to be executed.
 * \relates Logger
 */
// clang-format off
#define CHECKED_LOG(statement) statement
// clang-format on
#else
/**
 * \brief Utility checking \c I2C_LOG_ENABLE macro and then wrapping to
 * actual (log) statement.
 *
 * \code
 *     CHECKED_LOG(ifx_logger_log(logger_object, source_information, \ 
 *                  logger_level, formatter, ##__VA_ARGS__));
 * \endcode
 *
 * \param[in] statement (Log) statement to be executed.
 * \relates Logger
 */
#define CHECKED_LOG(statement) do {} while(0)
#endif

#include <stdint.h>
#include <stddef.h>

#include "cyhal.h"

#include "infineon/ifx-error.h"
#include "infineon/ifx-protocol.h"
#include "infineon/ifx-timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief ifx_protocol_activate_callback_t for ModusToolbox I2C HAL driver layer.
 *
 * \see ifx_protocol_activate_callback_t
 */
ifx_status_t i2c_cyhal_activate(ifx_protocol_t *self, uint8_t **response_buffer, size_t *response_len);

/**
 * \brief ifx_protocol_transmit_callback_t for ModusToolbox I2C HAL driver layer.
 *
 * \see ifx_protocol_transmit_callback_t
 */
ifx_status_t i2c_cyhal_transmit(ifx_protocol_t *self, const uint8_t *data, size_t data_len);

/**
 * \brief ifx_protocol_receive_callback_t for ModusToolbox I2C HAL driver layer.
 *
 * \see ifx_protocol_receive_callback_t
 */
ifx_status_t i2c_cyhal_receive(ifx_protocol_t *self, size_t expected_len, uint8_t **response, size_t *response_len);

/**
 * \brief ifx_protocol_destroy_callback_t for ModusToolbox I2C HAL driver layer.
 *
 * \see ifx_protocol_destroy_callback_t
 */
void i2c_cyhal_destroy(ifx_protocol_t *self);

/**
 * \brief Protocol Layer ID for ModusToolbox I2C HAL driver layer.
 *
 * \details Used to verify that correct protocol layer has called member functionality.
 */
#define I2C_CYHAL_PROTOCOLLAYER_ID 0x35U

/**
 * \brief Default value for I2C clock frequency in [Hz].
 */
#define I2C_CYHAL_DEFAULT_CLOCK_FREQUENCY_HZ ((uint32_t) 400000U)

/**
 * \brief Default I2C guard time in [us].
 */
#define I2C_CYHAL_DEFAULT_GUARD_TIME_US 0U

/** \struct I2CCyHALProtocolProperties
 * \brief State of I2C driver driver layer keeping track of current property values.
 */
typedef struct
{
    /**
     * \brief File descriptor of the opened I2C character device.
     *
     * \see i2c_cyhal_initialize()
     */
    int native_instance;

    /**
     * \brief I2C slave address currently in use.
     *
     * \see ifx_i2c_get_slave_address()
     */
    uint8_t slave_address;

    /**
     * \brief I2C clock frequency in [Hz].
     *
     * \see ifx_i2c_get_clock_frequency()
     */
    uint32_t clock_frequency_hz;

    /**
     * \brief I2C guard time in [us].
     *
     * \see ifx_i2c_get_guard_time()
     */
    uint32_t guard_time_us;

    /**
     * \brief Timer used to ensure guard time between I2C accesses is handled correctly.
     *
     * \see I2CCyHALProtocolProperties.guard_time
     */
    ifx_timer_t _guard_time_timer;
} I2CCyHALProtocolProperties;

/**
 * \brief IFX status encoding function identifier for private function i2c_cyhal_get_protocol_properties().
 */
#define IFX_I2C_CYHAL_GET_PROPERTIES (0x80U)

/**
 * \brief Returns current protocol properties for of ModusToolbox I2C HAL driver layer.
 *
 * \param[in] self Protocol stack to get protocol state for.
 * \param[out] properties_buffer Buffer to store protocol properties in.
 * \return ifx_status_t `IFX_SUCCESS` if successful, any other value in case of error.
 */
ifx_status_t i2c_cyhal_get_protocol_properties(ifx_protocol_t *self, I2CCyHALProtocolProperties **properties_buffer);

/**
 * \brief Starts I2C guard time to be waited between consecutive I2C accesses.
 *
 * \param[in] properties Protocol properties containing required information.
 * \return ifx_status_t `IFX_SUCCESS` if successful, any other value in case of error.
 */
ifx_status_t i2c_cyhal_start_guard_time(I2CCyHALProtocolProperties *properties);

/**
 * \brief Waits for I2C guard time to be over to be able to send / receive next I2C frame.
 *
 * \param[in] properties Protocol properties containing required information.
 * \return ifx_status_t `IFX_SUCCESS` if successful, any other value in case of error.
 */
ifx_status_t i2c_cyhal_await_guard_time(I2CCyHALProtocolProperties *properties);

#ifdef __cplusplus
}
#endif

#endif // I2C_CYHAL_H
