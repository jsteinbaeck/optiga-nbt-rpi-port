// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file timer-freertos.c
 * \brief Timer API implementation for NBT framework based on FreeRTOS abstraction.
 * \details This version is meant to be used by RTOS-based systems.
 * \details Selection is taken based on definition of `FREERTOS` component or `NBT_TIMER_CUSTOM` macro.
 */
#if (defined(COMPONENT_FREERTOS) && !defined(NBT_TIMER_CUSTOM))

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"

#include "infineon/ifx-error.h"
#include "infineon/ifx-timer.h"

/** \struct timer_freertos_data
 * \brief Data required for FreeRTOS based timer implementation.
 */
struct timer_freertos_data
{
    /**
     * \brief FreeRTOS timer used for cleanup later.
     */
    TimerHandle_t timer;

    /**
     * \brief Binary semaphore used to detect if timer has elapsed.
     */
    SemaphoreHandle_t sleeper;
};

/**
 * \brief Callback triggered when FreeRTOS timer has elapsed.
 *
 * \details FreeRTOS timer is started by ifx_timer_set() and stores the required information in given xTimer's ID.
 * It then wakes up any waiting threads by unlocking the mutex acquired during timer_set().
 *
 * \param[in] xTimer FreeRTOS timer holding required information.
 * \see pvTimerGetTimerID()
 */
static void timer_freertos_wakeup(TimerHandle_t xTimer)
{
    ifx_timer_t *timer = (ifx_timer_t *) pvTimerGetTimerID(xTimer);
    if (timer != NULL)
    {
        struct timer_freertos_data *data = (struct timer_freertos_data *) timer->_start;
        if ((data != NULL) && (data->sleeper != NULL))
        {
            while (xSemaphoreGive(data->sleeper) != pdPASS)
                ;
        }
    }
}

/**
 * \brief Sets Timer for given amount of [us].
 *
 * \param[in] timer Timer object to be set.
 * \param[in] us us Timer duration in [us].
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other value in
 * case of error.
 * \relates ifx_timer_t
 */
ifx_status_t ifx_timer_set(ifx_timer_t *timer, uint64_t us)
{
    // Validate parameters
    uint64_t ms = (uint64_t) ceil(((double) us) / 1000.0);
    if ((timer == NULL) || (ms > UINT32_MAX))
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_ILLEGAL_ARGUMENT);
    }

    // Allocate memory for timer information
    struct timer_freertos_data *data = calloc(1U, sizeof(struct timer_freertos_data));
    if (data == NULL)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_OUT_OF_MEMORY);
    }

    // Set up mutex used for sleeping while waiting on timer
    data->sleeper = xSemaphoreCreateBinary();
    if (data->sleeper == NULL)
    {
        free(data);
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_UNSPECIFIED_ERROR);
    }

    // Actually start FreeRTOS timer
    data->timer = xTimerCreate(NULL, pdMS_TO_TICKS(ms), pdFALSE, timer, timer_freertos_wakeup);
    if (data->timer == NULL)
    {
        vSemaphoreDelete(data->sleeper);
        free(data);
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_UNSPECIFIED_ERROR);
    }
    if (xTimerStart(data->timer, 0U) != pdPASS)
    {
        while (xTimerDelete(data->timer, 0U) != pdPASS)
            ;
        vSemaphoreDelete(data->sleeper);
        free(data);
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_UNSPECIFIED_ERROR);
    }

    // Cache data for later use
    timer->_start = data;
    timer->_duration = ms;

    return IFX_SUCCESS;
}

/**
 * \brief Checks if Timer has elapsed.
 *
 * \details Per definition timers that have not previously been set are
 * considered elapsed.
 *
 * \param[in] timer Timer object to be checked.
 * \return bool \c true if timer has elapsed.
 * \relates ifx_timer_t
 */
bool ifx_timer_has_elapsed(const ifx_timer_t *timer)
{
    if ((timer == NULL) || (timer->_start == NULL))
    {
        return true;
    }
    struct timer_freertos_data *data = (struct timer_freertos_data *) timer->_start;
    if (data->timer == NULL)
    {
        return true;
    }
    return xTimerIsTimerActive(data->timer) == pdFALSE;
}

/**
 * \brief Waits for Timer to finish.
 *
 * \param[in] timer Timer to be joined (wait until finished).
 * \return ifx_status_t \c IFX_SUCCESS if successful, any other value in
 * case of error.
 * \relates ifx_timer_t
 */
ifx_status_t ifx_timer_join(const ifx_timer_t *timer)
{
    if (timer == NULL)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_JOIN, IFX_ILLEGAL_ARGUMENT);
    }
    if (timer->_start == NULL)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_JOIN, IFX_TIMER_NOT_SET);
    }
    struct timer_freertos_data *data = (struct timer_freertos_data *) timer->_start;
    if (data->sleeper == NULL)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_JOIN, IFX_TIMER_NOT_SET);
    }
    if (xSemaphoreTake(data->sleeper, portMAX_DELAY) != pdPASS)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_JOIN, IFX_UNSPECIFIED_ERROR);
    }
    return IFX_SUCCESS;
}

/**
 * \brief Frees memory associated with Timer object (but not object itself).
 *
 * \details Timer objects may contain dynamically allocated data that needs
 * special functionality to be freed. Users would need to know the concrete type
 * based on the linked implementation. This would negate all benefits of having
 * a generic interface. Calling this function will ensure that all dynamically
 * allocated members have been freed.
 *
 * \param[in] timer Timer object whose data shall be freed.
 * \relates ifx_timer_t
 */
void ifx_timer_destroy(ifx_timer_t *timer)
{
    if (timer != NULL)
    {
        struct timer_freertos_data *data = (struct timer_freertos_data *) timer->_start;
        if (data != NULL)
        {
            if (data->timer != NULL)
            {
                while (xTimerDelete(data->timer, 0U) != pdPASS)
                    ;
                data->timer = NULL;
            }
            if (data->sleeper != NULL)
            {
                vSemaphoreDelete(data->sleeper);
                data->sleeper = NULL;
            }
            free(data);
        }
        timer->_start = NULL;
        timer->_duration = 0U;
    }
}

#endif // (defined(COMPONENT_FREERTOS) && !defined(NBT_TIMER_CUSTOM))
