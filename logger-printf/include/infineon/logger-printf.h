// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file infineon/logger-printf.h
 * \brief Logger API implementation for NBT framework logging via printf.
 */
#ifndef INFINEON_LOGGER_PRINTF_H
#define INFINEON_LOGGER_PRINTF_H

#include "infineon/ifx-error.h"
#include "infineon/ifx-logger.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Initializes ifx_logger_t object to be used as a printf logger.
 *
 * \param[in] self Logger object to be initialized.
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other value in case of error.
 */
ifx_status_t logger_printf_initialize(ifx_logger_t *self);

#ifdef __cplusplus
}
#endif

#endif // INFINEON_LOGGER_PRINTF_H
