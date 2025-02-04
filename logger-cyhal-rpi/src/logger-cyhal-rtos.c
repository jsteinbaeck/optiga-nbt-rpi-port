// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file logger-cyhal-rtos.c
 * \brief Logger API implementation for NBT framework dispatching log calls via a single RTOS task.
 */
#if defined(CY_RTOS_AWARE) || defined(COMPONENT_RTOS_AWARE)
#include <stdlib.h>
#include <string.h>

#include "cy_result.h"
#include "cy_utils.h"
#include "cyabs_rtos.h"

#include "infineon/ifx-error.h"
#include "infineon/ifx-logger.h"
#include "infineon/logger-cyhal-rtos.h"
#include "logger-cyhal-rtos.h"

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
ifx_status_t logger_cyhal_rtos_initialize(ifx_logger_t *self, ifx_logger_t *wrapped)
{
    // Validate parameters
    if ((self == NULL) || (wrapped == NULL) || (wrapped->_log == NULL))
    {
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_INITIALIZE, IFX_ILLEGAL_ARGUMENT);
    }

    // Prepare queue for sending data to task
    struct logger_cyhal_rtos_data *data = malloc(sizeof(struct logger_cyhal_rtos_data));
    if (data == NULL)
    {
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_INITIALIZE, IFX_OUT_OF_MEMORY);
    }
    data->wrapped = wrapped;
    if (cy_rtos_queue_init(&data->queue, LOGGER_CYHAL_RTOS_QUEUE_SIZE, sizeof(struct logger_cyhal_rtos_log_data *)) != CY_RSLT_SUCCESS)
    {
        free(data);
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_INITIALIZE, IFX_UNSPECIFIED_ERROR);
    }

    // Initialize object with default values
    ifx_status_t status = ifx_logger_initialize(self);
    if (ifx_error_check(status))
    {
        cy_rtos_queue_deinit(&data->queue);
        free(data);
        return status;
    }

    // Populate member functions
    self->_log = logger_cyhal_rtos_log;
    self->_destructor = logger_cyhal_rtos_destroy;
    self->_data = data;
    return IFX_SUCCESS;
}

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
ifx_status_t logger_cyhal_rtos_start(ifx_logger_t *self, cy_thread_t *thread_buffer)
{
    // Validate parameters
    if ((self == NULL) || (self->_data == NULL))
    {
        return IFX_ERROR(LIBLOGGERCYHALRTOS, LOGGERCYHALRTOS_START, IFX_ILLEGAL_ARGUMENT);
    }

    // Start consumer task
    cy_thread_t _thread_hack;
    if (thread_buffer == NULL)
    {
        thread_buffer = &_thread_hack;
    }
    if (cy_rtos_thread_create(thread_buffer, logger_cyhal_rtos_task, "Log consumer", NULL, LOGGER_CYHAL_RTOS_STACK_SIZE, LOGGER_CYHAL_RTOS_PRIORITY, self) !=
        CY_RSLT_SUCCESS)
    {
        return IFX_ERROR(LIBLOGGERCYHALRTOS, LOGGERCYHALRTOS_START, IFX_UNSPECIFIED_ERROR);
    }
    return IFX_SUCCESS;
}

/**
 * \brief \ref ifx_logger_log_callback_t for RTOS logging task.
 *
 * \see ifx_logger_log_callback_t
 */
ifx_status_t logger_cyhal_rtos_log(const ifx_logger_t *self, const char *source, ifx_log_level level, const char *formatter)
{
    // Validate parameter
    if ((self == NULL) || (self->_data == NULL) || (source == NULL) || (formatter == NULL))
    {
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_LOG, IFX_ILLEGAL_ARGUMENT);
    }

    // Double check level
    struct logger_cyhal_rtos_data *data = (struct logger_cyhal_rtos_data *) self->_data;
    if ((level < self->_level) || (level < data->wrapped->_level))
    {
        return IFX_SUCCESS;
    }

    // Add data to queue
    struct logger_cyhal_rtos_log_data *queueable = malloc(sizeof(struct logger_cyhal_rtos_log_data));
    if (queueable == NULL)
    {
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_LOG, IFX_OUT_OF_MEMORY);
    }
    queueable->source = malloc(strlen(source) + 1);
    if (queueable->source == NULL)
    {
        free(queueable);
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_LOG, IFX_OUT_OF_MEMORY);
    }
    queueable->formatter = malloc(strlen(formatter) + 1);
    if (queueable->formatter == NULL)
    {
        free(queueable->source);
        free(queueable);
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_LOG, IFX_OUT_OF_MEMORY);
    }
    strcpy(queueable->source, source);
    strcpy(queueable->formatter, formatter);
    queueable->level = level;
    if (cy_rtos_queue_put(&data->queue, &queueable, 0U) != CY_RSLT_SUCCESS)
    {
        free(queueable->formatter);
        free(queueable->source);
        free(queueable);
        return IFX_ERROR(LIB_LOGGER, IFX_LOGGER_LOG, IFX_UNSPECIFIED_ERROR);
    }

    return IFX_SUCCESS;
}

/**
 * \brief \ref ifx_logger_destroy_callback_t for RTOS logging utility.
 *
 * \see ifx_logger_destroy_callback_t
 */
void logger_cyhal_rtos_destroy(ifx_logger_t *self)
{
    if (self != NULL)
    {
        if (self->_data != NULL)
        {
            struct logger_cyhal_rtos_data *data = (struct logger_cyhal_rtos_data *) self->_data;
            struct logger_cyhal_rtos_log_data *to_clean = NULL;
            while (cy_rtos_queue_get(&data->queue, &to_clean, 0U) == CY_RSLT_SUCCESS)
            {
                if (to_clean != NULL)
                {
                    if (to_clean->source != NULL) free(to_clean->source);
                    if (to_clean->formatter != NULL) free(to_clean->formatter);
                    free(to_clean);
                }
                to_clean = NULL;
            }
            cy_rtos_queue_deinit(&data->queue);
            data->wrapped = NULL;
            free(data);
        }
        self->_data = NULL;
        cy_rtos_thread_exit();
    }
}

/**
 * \brief RTOS task consuming data being logged by all different tasks.
 *
 * \param[in] arg RTOS logger object (as ifx_logger_t).
 */
void logger_cyhal_rtos_task(void *arg)
{
    // Validate parameters
    if (arg == NULL)
    {
        CY_ASSERT(0);
    }
    ifx_logger_t *self = (ifx_logger_t *) arg;
    struct logger_cyhal_rtos_data *data = (struct logger_cyhal_rtos_data *) self->_data;
    if ((data == NULL) || (data->wrapped == NULL) || (data->wrapped->_log == NULL))
    {
        CY_ASSERT(0);
    }

    while (1)
    {
        // Wait for log data to be available
        struct logger_cyhal_rtos_log_data *queued = NULL;
        if (cy_rtos_queue_get(&data->queue, &queued, 0xffffffffU) == CY_RSLT_SUCCESS)
        {
            if (queued != NULL)
            {
                data->wrapped->_log(data->wrapped, queued->source, queued->level, queued->formatter);
                if (queued->source != NULL) free(queued->source);
                if (queued->formatter != NULL) free(queued->formatter);
                free(queued);
            }
            queued = NULL;
        }
    }
}
#endif
