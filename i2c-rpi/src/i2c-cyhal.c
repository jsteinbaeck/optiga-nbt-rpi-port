// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file i2c-cyhal.c
 * \brief I2C driver wrapper for NBT framework based on ModusToolbox HAL.
 */
#include <stdlib.h>

/* Raspberry PI I2C specific headers */
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>


#include "infineon/ifx-error.h"
#include "infineon/ifx-i2c.h"
#include "infineon/ifx-logger.h"
#include "infineon/ifx-protocol.h"
#include "infineon/ifx-timer.h"
#include "infineon/i2c-cyhal.h"
#include "i2c-cyhal.h"

/**
 * \brief String used as source information for logging.
 */
#define LOG_TAG I2C_CYHAL_LOG_TAG

/**
 * \brief Initializes protocol object for Raspberry PI.
 *
 * \param[in] self Protocol object to be initialized.
 * \param[in] native_instance File descriptor of the opened I2C device file.
 * \param[in] slave_address Initial I2C slave address to be used.
 * \return ifx_status_t `IFX_SUCCESS` if successful, any other value in case of error.
 */
ifx_status_t i2c_cyhal_initialize(ifx_protocol_t *self, int native_instance, uint8_t slave_address)
{
    // Validate parameters
    if ((self == NULL) || (native_instance == -1))
    {
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_LAYER_INITIALIZE, IFX_ILLEGAL_ARGUMENT);
    }

    // Populate object
    ifx_status_t status = ifx_protocol_layer_initialize(self);
    if (ifx_error_check(status))
    {
        return status;
    }
    self->_layer_id = I2C_CYHAL_PROTOCOLLAYER_ID;
    self->_activate = i2c_cyhal_activate;
    self->_transmit = i2c_cyhal_transmit;
    self->_receive = i2c_cyhal_receive;
    self->_destructor = i2c_cyhal_destroy;

    // Populate protocol properties
    I2CCyHALProtocolProperties *properties = malloc(sizeof(I2CCyHALProtocolProperties));
    if (properties == NULL)
    {
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_LAYER_INITIALIZE, IFX_OUT_OF_MEMORY);
    }
    properties->native_instance = native_instance;
    properties->clock_frequency_hz = I2C_CYHAL_DEFAULT_CLOCK_FREQUENCY_HZ;
    properties->slave_address = slave_address;
    properties->_guard_time_timer._start = NULL;
    self->_properties = properties;

    return IFX_SUCCESS;
}

/**
 * \brief ifx_protocol_activate_callback_t for Raspberry PI.
 *
 * \see ifx_protocol_activate_callback_t
 */
ifx_status_t i2c_cyhal_activate(ifx_protocol_t *self, uint8_t **response_buffer, size_t *response_len)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_ACTIVATE, IFX_ILLEGAL_ARGUMENT);
    }

    // Do not send any response
    if (response_buffer != NULL) *response_buffer = NULL;
    if (response_len != NULL) *response_len = 0;
    return IFX_SUCCESS;
}

/**
 * \brief ifx_protocol_transmit_callback_t for Raspberry PI.
 *
 * \see ifx_protocol_transmit_callback_t
 */
ifx_status_t i2c_cyhal_transmit(ifx_protocol_t *self, const uint8_t *data, size_t data_len)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_TRANSMIT, IFX_ILLEGAL_ARGUMENT);
    }
    if (data == NULL)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "i2c_cyhal_transmit() called with illegal NULL argument"));
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_TRANSMIT, IFX_ILLEGAL_ARGUMENT);
    }
    if ((data_len == 0U) || (data_len > 0xffffffffU))
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "can only send between 1 and 0xffffffff bytes (%zu requested)", data_len));
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_TRANSMIT, IFX_ILLEGAL_ARGUMENT);
    }

    // Get protocol properties with native I2C instance
    I2CCyHALProtocolProperties *properties = NULL;
    ifx_status_t status = i2c_cyhal_get_protocol_properties(self, &properties);
    if (ifx_error_check(status))
    {
        return status;
    }

    // Await guard time to avoid issues with consecutive I2C requests
    status = i2c_cyhal_await_guard_time(properties);
    if (ifx_error_check(status))
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "Error occurred while awaiting I2C guard time"));
        return status;
    }

    // Actually send data to I2C slave
    CHECKED_LOG(ifx_logger_log_bytes(self->_logger, LOG_TAG, IFX_LOG_INFO, ">> ", data, data_len, " "));

    /* 1. Set the slave address */
    if (ioctl(properties->native_instance, I2C_SLAVE, properties->slave_address) < 0)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "Unspecified error occurred while setting I2C Slave address"));
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_TRANSMIT, IFX_UNSPECIFIED_ERROR);
    }
    
    /* 2. Write data to I2C character file */
    if (write(properties->native_instance, data, data_len) != data_len)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "Unspecified error occurred while transmitting data via I2C"));
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_TRANSMIT, IFX_UNSPECIFIED_ERROR);
    }

    // Start new guard time between secure element accesses
    status = i2c_cyhal_start_guard_time(properties);
    if (ifx_error_check(status))
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "could not start I2C guard time timer"));
        return status;
    }

    return IFX_SUCCESS;
}

/**
 * \brief ifx_protocol_receive_callback_t for Raspberry PI I2C layer.
 *
 * \see ifx_protocol_receive_callback_t
 */
ifx_status_t i2c_cyhal_receive(ifx_protocol_t *self, size_t expected_len, uint8_t **response, size_t *response_len)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_RECEIVE, IFX_ILLEGAL_ARGUMENT);
    }
    if ((expected_len == 0U) || (expected_len > 0xffffffffU) || (expected_len == IFX_PROTOCOL_RECEIVE_LEN_UNKOWN))
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "can only read between 1 and 0xffffffff bytes (%zu requested)", expected_len));
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_RECEIVE, IFX_ILLEGAL_ARGUMENT);
    }
    if ((response == NULL) || (response_len == NULL))
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "i2c_cyhal_receive() called with illegal NULL argument"));
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_RECEIVE, IFX_ILLEGAL_ARGUMENT);
    }

    // Get protocol properties with native I2C instance
    I2CCyHALProtocolProperties *properties = NULL;
    ifx_status_t status = i2c_cyhal_get_protocol_properties(self, &properties);
    if (ifx_error_check(status))
    {
        return status;
    }

    // Await guard time to avoid issues with consecutive I2C requests
    status = i2c_cyhal_await_guard_time(properties);
    if (ifx_error_check(status))
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "Error occurred while awaiting I2C guard time"));
        return status;
    }

    // Allocate buffer for I2C receive
    *response = malloc(expected_len);
    if ((*response) == NULL)
    {
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_RECEIVE, IFX_OUT_OF_MEMORY);
    }

    /* 1. Set the slave address */
    if (ioctl(properties->native_instance, I2C_SLAVE, properties->slave_address) < 0)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "Unspecified error occurred while setting I2C Slave address"));
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_TRANSMIT, IFX_UNSPECIFIED_ERROR);
    }

    /* 2. Read data from I2C character file */
    /* TODO: May be making it error if expected_len != response_len is a good idea.. */
    if (*response_len = read(properties->native_instance, *response, expected_len) == -1)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "Unspecified error occurred while reading data via I2C"));
        free(*response);
        *response = NULL;
        *response_len = 0U;
        return IFX_ERROR(LIBI2CCYHAL, IFX_PROTOCOL_TRANSMIT, IFX_UNSPECIFIED_ERROR);
    }

    *response_len = expected_len;
    CHECKED_LOG(ifx_logger_log_bytes(self->_logger, LOG_TAG, IFX_LOG_INFO, "<< ", *response, *response_len, " "));

    // Start new guard time between secure element accesses
    status = i2c_cyhal_start_guard_time(properties);
    if (ifx_error_check(status))
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "could not start I2C guard time timer"));
        free(*response);
        *response = NULL;
        *response_len = 0U;
        return status;
    }

    return IFX_SUCCESS;
}

/**
 * \brief ifx_protocol_destroy_callback_t for ModusToolbox I2C HAL driver layer.
 *
 * \see ifx_protocol_destroy_callback_t
 */
void i2c_cyhal_destroy(ifx_protocol_t *self)
{
    if (self != NULL)
    {
        if (self->_properties != NULL)
        {
            // Get properties casted to correct type
            I2CCyHALProtocolProperties *properties = NULL;
            if (!ifx_error_check(i2c_cyhal_get_protocol_properties(self, &properties)))
            {
                // Stop running guard timer
                ifx_timer_destroy(&properties->_guard_time_timer);
            }
            free(self->_properties);
        }
        self->_properties = NULL;
    }
}

/**
 * \brief Getter for I2C clock frequency in [Hz].
 *
 * \param[in] self Protocol object to get clock frequency for.
 * \param[out] frequency_hz_buffer Buffer to store clock frequency in.
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other
 * value in case of error.
 * \cond FULL_DOCUMENTATION_BUILD
 * \relates ifx_protocol
 * \endcond
 */
ifx_status_t ifx_i2c_get_clock_frequency(ifx_protocol_t *self, uint32_t *frequency_hz_buffer)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_GET_CLOCK_FREQUENCY, IFX_ILLEGAL_ARGUMENT);
    }
    if (frequency_hz_buffer == NULL)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "i2c_get_clock_frequency() called with illegal NULL argument"));
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_GET_CLOCK_FREQUENCY, IFX_ILLEGAL_ARGUMENT);
    }

    I2CCyHALProtocolProperties *properties = NULL;
    ifx_status_t status = i2c_cyhal_get_protocol_properties(self, &properties);
    if (ifx_error_check(status))
    {
        return status;
    }
    *frequency_hz_buffer = properties->clock_frequency_hz;

    return IFX_SUCCESS;
}

/**
 * \brief Sets I2C clock frequency in [Hz].
 *
 * \param[in] self Protocol object to set clock frequency for.
 * \param[in] frequency_hz Desired clock frequency in [Hz].
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other
 * value in case of error.
 * \cond FULL_DOCUMENTATION_BUILD
 * \relates ifx_protocol
 * \endcond
 */
ifx_status_t ifx_i2c_set_clock_frequency(ifx_protocol_t *self, uint32_t frequency_hz)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_SET_CLOCK_FREQUENCY, IFX_ILLEGAL_ARGUMENT);
    }
    if (frequency_hz == 0U)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "Cannot set I2C clock frequency to 0 Hz"));
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_SET_CLOCK_FREQUENCY, IFX_ILLEGAL_ARGUMENT);
    }

    I2CCyHALProtocolProperties *properties = NULL;
    ifx_status_t status = i2c_cyhal_get_protocol_properties(self, &properties);
    if (ifx_error_check(status))
    {
        return status;
    }
    /* FIXME: Not possible. Set the I2C clock frequency */

    properties->clock_frequency_hz = frequency_hz;
    CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_INFO, "Successfully set I2C clock frequency to %lu Hz", frequency_hz));

    return IFX_SUCCESS;
}

/**
 * \brief Getter for I2C slave address.
 *
 * \param[in] self Protocol object to get I2C slave address for.
 * \param[out] address_buffer Buffer to store I2C address in.
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other
 * value in case of error.
 * \cond FULL_DOCUMENTATION_BUILD
 * \relates ifx_protocol
 * \endcond
 */
ifx_status_t ifx_i2c_get_slave_address(ifx_protocol_t *self, uint16_t *address_buffer)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_GET_SLAVE_ADDR, IFX_ILLEGAL_ARGUMENT);
    }
    if (address_buffer == NULL)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "i2c_get_slave_address() called with illegal NULL argument"));
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_GET_SLAVE_ADDR, IFX_ILLEGAL_ARGUMENT);
    }

    I2CCyHALProtocolProperties *properties = NULL;
    ifx_status_t status = i2c_cyhal_get_protocol_properties(self, &properties);
    if (ifx_error_check(status))
    {
        return status;
    }
    *address_buffer = properties->slave_address;
    return IFX_SUCCESS;
}

/**
 * \brief Sets I2C slave address.
 *
 * \param[in] self Protocol object to set I2C slave address for.
 * \param[out] address Desired I2C slave address.
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other
 * value in case of error.
 * \cond FULL_DOCUMENTATION_BUILD
 * \relates ifx_protocol
 * \endcond
 */
ifx_status_t ifx_i2c_set_slave_address(ifx_protocol_t *self, uint16_t address)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_SET_SLAVE_ADDR, IFX_ILLEGAL_ARGUMENT);
    }
    if ((address == 0x00U) || (address > 0xffU))
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "I2C slave address must be in range from 0x01 to 0xff (0x%x given)", address));
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_SET_SLAVE_ADDR, IFX_ILLEGAL_ARGUMENT);
    }

    I2CCyHALProtocolProperties *properties = NULL;
    ifx_status_t status = i2c_cyhal_get_protocol_properties(self, &properties);
    if (ifx_error_check(status))
    {
        return status;
    }

    // Cache I2C slave address
    properties->slave_address = address & 0xffU;
    CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_DEBUG, "Successfully set I2C slave address to 0x%x", address));
    return IFX_SUCCESS;
}

/**
 * \brief Getter for I2C guard time in [us].
 *
 * \details Some peripherals have a guard time that needs to be waited
 * between consecutive I2C requests. Setting this guard time will ensure that
 * said time is awaited between requests.
 *
 * \param[in] self Protocol object to get I2C guard time for.
 * \param[out] guard_time_us_buffer Buffer to store I2C guard time in.
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other
 * value in case of error.
 * \cond FULL_DOCUMENTATION_BUILD
 * \relates ifx_protocol
 * \endcond
 */
ifx_status_t ifx_i2c_get_guard_time(ifx_protocol_t *self, uint32_t *guard_time_us_buffer)
{
    // Validate parameters
    if ((self == NULL) || (guard_time_us_buffer == NULL))
    {
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_GET_GUARD_TIME, IFX_ILLEGAL_ARGUMENT);
    }

    I2CCyHALProtocolProperties *properties = NULL;
    ifx_status_t status = i2c_cyhal_get_protocol_properties(self, &properties);
    if (ifx_error_check(status))
    {
        return status;
    }
    *guard_time_us_buffer = properties->guard_time_us;
    return IFX_SUCCESS;
}

/**
 * \brief Sets guard time to be waited between I2C transmissions.
 *
 * \param[in] self Protocol object to set I2C guard time for.
 * \param[in] guard_time_us Desired I2C guard time in [us].
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other
 * value in case of error.
 * \see ifx_i2c_get_guard_time()
 * \cond FULL_DOCUMENTATION_BUILD
 * \relates ifx_protocol
 * \endcond
 */
ifx_status_t ifx_i2c_set_guard_time(ifx_protocol_t *self, uint32_t guard_time_us)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIB_PROTOCOL, IFX_I2C_SET_GUARD_TIME, IFX_ILLEGAL_ARGUMENT);
    }

    CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_DEBUG, "Setting I2C guard time to %lu us", guard_time_us));

    I2CCyHALProtocolProperties *properties = NULL;
    ifx_status_t status = i2c_cyhal_get_protocol_properties(self, &properties);
    if (ifx_error_check(status))
    {
        return status;
    }
    properties->guard_time_us = guard_time_us;

    CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_DEBUG, "Successfully set I2C guard time to %lu us", guard_time_us));
    return IFX_SUCCESS;
}

/**
 * \brief Returns current protocol properties for of ModusToolbox I2C HAL driver layer.
 *
 * \param[in] self Protocol stack to get protocol state for.
 * \param[out] properties_buffer Buffer to store protocol properties in.
 * \return ifx_status_t `IFX_SUCCESS` if successful, any other value in case of error.
 */
ifx_status_t i2c_cyhal_get_protocol_properties(ifx_protocol_t *self, I2CCyHALProtocolProperties **properties_buffer)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIBI2CCYHAL, IFX_I2C_CYHAL_GET_PROPERTIES, IFX_ILLEGAL_ARGUMENT);
    }
    if (properties_buffer == NULL)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_ERROR, "i2c_cyhal_get_protocol_properties() called with illegal NULL argument"));
        return IFX_ERROR(LIBI2CCYHAL, IFX_I2C_CYHAL_GET_PROPERTIES, IFX_ILLEGAL_ARGUMENT);
    }

    // Verify that correct protocol layer called this function
    if (self->_layer_id != I2C_CYHAL_PROTOCOLLAYER_ID)
    {
        if (self->_base == NULL)
        {
            return IFX_ERROR(LIBI2CCYHAL, IFX_I2C_CYHAL_GET_PROPERTIES, IFX_PROTOCOL_STACK_INVALID);
        }
        return i2c_cyhal_get_protocol_properties(self->_base, properties_buffer);
    }

    // Verify protocol state
    if (self->_properties == NULL)
    {
        CHECKED_LOG(ifx_logger_log(self->_logger, LOG_TAG, IFX_LOG_FATAL, "i2c_cyhal_get_protocol_properties() called with uninitialized/destroyed protocol stack"));
        return IFX_ERROR(LIBI2CCYHAL, IFX_I2C_CYHAL_GET_PROPERTIES, IFX_PROTOCOL_STACK_INVALID);
    }
    *properties_buffer = (I2CCyHALProtocolProperties *) self->_properties;
    return IFX_SUCCESS;
}

/**
 * \brief Starts I2C guard time to be waited between consecutive I2C accesses.
 *
 * \param[in] properties Protocol properties containing required information.
 * \return ifx_status_t `IFX_SUCCESS` if successful, any other value in case of error.
 */
ifx_status_t i2c_cyhal_start_guard_time(I2CCyHALProtocolProperties *properties)
{
    // Validate parameters
    if (properties == NULL)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_ILLEGAL_ARGUMENT);
    }

    // Destroy old timer just to be sure
    ifx_timer_destroy(&properties->_guard_time_timer);

    // Set new timer if guard time is set
    if (properties->guard_time_us > 0U)
    {
        return ifx_timer_set(&properties->_guard_time_timer, properties->guard_time_us);
    }
    else
    {
        return IFX_SUCCESS;
    }
}

/**
 * \brief Waits for I2C guard time to be over to be able to send / receive next I2C frame.
 *
 * \param[in] properties Protocol properties containing required information.
 * \return ifx_status_t `IFX_SUCCESS` if successful, any other value in case of error.
 */
ifx_status_t i2c_cyhal_await_guard_time(I2CCyHALProtocolProperties *properties)
{
    // Validate parameters
    if (properties == NULL)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_JOIN, IFX_ILLEGAL_ARGUMENT);
    }

    if (properties->_guard_time_timer._start == NULL)
    {
        return IFX_SUCCESS;
    }

    // Await old timer
    ifx_status_t status = ifx_timer_join(&properties->_guard_time_timer);
    ifx_timer_destroy(&properties->_guard_time_timer);

    // Check if join was successful
    if (ifx_error_check(status))
    {
        // Errors because of unset timers are acceptable
        uint8_t module = ifx_error_get_module(status);
        uint8_t function = ifx_error_get_function(status);
        uint8_t reason = ifx_error_get_reason(status);
        if ((module == LIB_TIMER) && (function == IFX_TIMER_JOIN) && (reason == IFX_TIMER_NOT_SET))
        {
            status = IFX_SUCCESS;
        }
    }

    return status;
}
