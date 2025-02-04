// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file logger-printf.h
 * \brief Internal definitions for printf logger API implementation for NBT framework.
 */
#ifndef LOGGER_PRINTF_H
#define LOGGER_PRINTF_H

#include "infineon/ifx-error.h"
#include "infineon/ifx-logger.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief \ref ifx_logger_log_callback_t for printf logger.
 *
 * \see ifx_logger_log_callback_t
 */
ifx_status_t logger_printf_log(const ifx_logger_t *self, const char *source, ifx_log_level level, const char *formatter);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_PRINTF_H
