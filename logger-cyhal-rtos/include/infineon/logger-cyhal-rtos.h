// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file infineon/logger-cyhal-rtos.h
 * \brief Logger API implementation for NBT framework dispatching log calls via a single RTOS task.
 * \details Requires ModusTooblox RTOS abstraction to be available (via `RTOS_AWARE` component).
 */
#ifndef INFINEON_LOGGER_CYHAL_RTOS_H
#define INFINEON_LOGGER_CYHAL_RTOS_H

#if (!(defined(CY_RTOS_AWARE) || defined(COMPONENT_RTOS_AWARE)))
#error "RTOS logging utility requires RTOS_AWARE component to be set"
#endif

#include "cy_result.h"
#include "cyabs_rtos.h"

#include "infineon/ifx-error.h"
#include "infineon/ifx-logger.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief IFX error encoding module identifier.
 */
#define LIBLOGGERCYHALRTOS 0x10u

/**
 * \brief Initializes logger object to be used as a RTOS logger.
 *
 * \param[in] self Logger object to be initialized.
 * \param[in] wrapped Actual Logger object being wrapped.
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other value in case of error.
 * \cond FULL_DOCUMENTATION_BUILD
 * \relates Logger
 * \endcond
 */
ifx_status_t logger_cyhal_rtos_initialize(ifx_logger_t *self, ifx_logger_t *wrapped);

/**
 * \brief IFX error encoding function identifier for logger_rtos_start().
 */
#define LOGGERCYHALRTOS_START 0x01u

/**
 * \brief Starts RTOS logging task waiting for data to be logged and dispatching log calls to created task.
 *
 * \param[in] self Logger instance to start task for.
 * \param[out] thread_buffer Buffer to store created thread in.
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other value in case of error.
 * \cond FULL_DOCUMENTATION_BUILD
 * \relates Logger
 * \endcond
 */
ifx_status_t logger_cyhal_rtos_start(ifx_logger_t *self, cy_thread_t *thread_buffer);

#ifdef __cplusplus
}
#endif

#endif // INFINEON_LOGGER_CYHAL_RTOS_H
