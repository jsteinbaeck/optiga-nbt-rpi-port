// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file logger-chyal-rtos.h
 * \brief Internal definitions for Logger API implementation for NBT framework dispatching log calls via a single RTOS task.
 */
#ifndef LOGGER_CYHAL_RTOS_H
#define LOGGER_CYHAL_RTOS_H

#include "cyabs_rtos.h"

#include "infineon/ifx-error.h"
#include "infineon/ifx-logger.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOGGER_CYHAL_RTOS_PRIORITY
/**
 * \brief RTOS task priority for log consumer.
 * \details Can be overwritten via build system define.
 */
#define LOGGER_CYHAL_RTOS_PRIORITY (CY_RTOS_PRIORITY_MIN)
#endif

#ifndef LOGGER_CYHAL_RTOS_QUEUE_SIZE
/**
 * \brief Size of logging queue for RTOS based logger.
 * \details Log messages are queued here and later consumed by dedicated thread.
 * \details Can be overwritten via build system define.
 */
#define LOGGER_CYHAL_RTOS_QUEUE_SIZE 64U
#endif

#ifndef LOGGER_CYHAL_RTOS_STACK_SIZE
/**
 * \brief Stack size of RTOS log consumer thread.
 * \details Can be overwritten via build system define.
 */
#define LOGGER_CYHAL_RTOS_STACK_SIZE 2048U
#endif

/** \struct logger_cyhal_rtos_log_data
 * \brief Data as given to \ref ifx_logger_log_callback_t cached to be consumed by worker task.
 */
struct logger_cyhal_rtos_log_data
{
    /**
     * \brief String with information where the log originated from.
     */
    char *source;

    /**
     * \brief Log level of message (used for filtering).
     */
    ifx_log_level level;

    /**
     * \brief Formatted string to be logged.
     */
    char *formatter;
};

/** \struct logger_cyhal_rtos_data
 * \brief Instance properties for RTOS logger utility.
 */
struct logger_cyhal_rtos_data
{
    /**
     * \brief Actual logger object being wrapped.
     */
    ifx_logger_t *wrapped;

    /**
     * \brief RTOS queue for data exchange between logger and consumer task.
     */
    cy_queue_t queue;
};

/**
 * \brief \ref ifx_logger_log_callback_t for RTOS logging task.
 *
 * \see ifx_logger_log_callback_t
 */
ifx_status_t logger_cyhal_rtos_log(const ifx_logger_t *self, const char *source, ifx_log_level level, const char *formatter);

/**
 * \brief \ref ifx_logger_destroy_callback_t for RTOS logging utility.
 *
 * \see ifx_logger_destroy_callback_t
 */
void logger_cyhal_rtos_destroy(ifx_logger_t *self);

/**
 * \brief RTOS task consuming data being logged by all different tasks.
 *
 * \param[in] arg RTOS logger object (as ifx_logger_t).
 */
void logger_cyhal_rtos_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif // LOGGER_CYHAL_RTOS_H
