// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file logger-printf.c
 * \brief Logger API implementation for NBT framework logging via printf.
 */
#include <stdio.h>

#include "infineon/ifx-error.h"
#include "infineon/ifx-logger.h"
#include "infineon/logger-printf.h"
#include "logger-printf.h"

/**
 * \brief Initializes ifx_logger_t object to be used as a printf logger.
 *
 * \param[in] self Logger object to be initialized.
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other value in case of error.
 */
ifx_status_t logger_printf_initialize(ifx_logger_t *self)
{
    // Validate parameters
    if (self == NULL)
    {
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_INITIALIZE, IFX_ILLEGAL_ARGUMENT);
    }

    // Initialize object with default values
    ifx_status_t status = ifx_logger_initialize(self);
    if (ifx_error_check(status))
    {
        return status;
    }

    // Populate member functions
    self->_log = logger_printf_log;
    return IFX_SUCCESS;
}

/**
 * \brief \ref ifx_logger_log_callback_t for printf logger.
 *
 * \see ifx_logger_log_callback_t
 */
ifx_status_t logger_printf_log(const ifx_logger_t *self, const char *source, ifx_log_level level, const char *formatter)
{
    // Validate parameter
    if ((self == NULL) || (source == NULL) || (formatter == NULL))
    {
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_LOG, IFX_ILLEGAL_ARGUMENT);
    }

    // Get log level tag as string
    const char *level_tag;
    switch (level)
    {
    case IFX_LOG_DEBUG:
        level_tag = "DEBUG";
        break;
    case IFX_LOG_INFO:
        level_tag = "INFO";
        break;
    case IFX_LOG_WARN:
        level_tag = "WARNING";
        break;
    case IFX_LOG_ERROR:
        level_tag = "ERROR";
        break;
    case IFX_LOG_FATAL:
        level_tag = "FATAL";
        break;
    default:
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_LOG, IFX_ILLEGAL_ARGUMENT);
    }

    // Actually log data
    printf("[%-9s] [%-7s] %s\n", source, level_tag, formatter);
    return IFX_SUCCESS;
}
