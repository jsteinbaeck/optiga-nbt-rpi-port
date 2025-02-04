// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file timer-cyhal.c
 * \brief Timer API implementation for NBT framework based on ModusToolbox HAL.
 * \details This version uses the ModusToolbox HAL directly, for FreeRTOS-based systems timer-freertos is used.
 * \details Selection is taken based on definition of `FREERTOS` component or `NBT_TIMER_CUSTOM` macro.
 */

#if (!defined(COMPONENT_FREERTOS) && !defined(NBT_TIMER_CUSTOM))

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "cyhal.h"

#include "infineon/ifx-error.h"
#include "infineon/ifx-timer.h"

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
    if ((timer == NULL) || (us > UINT32_MAX))
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_ILLEGAL_ARGUMENT);
    }

    // Allocate memory for timer information
    cyhal_timer_t *cy_timer = malloc(sizeof(cyhal_timer_t));
    if (cy_timer == NULL)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_OUT_OF_MEMORY);
    }

    // Initialize and start CYHAL timer
    if (cyhal_timer_init(cy_timer, NC, NULL) != CY_RSLT_SUCCESS)
    {
        goto free_timer;
    }
    if (cyhal_timer_set_frequency(cy_timer, 1000000U) != CY_RSLT_SUCCESS)
    {
        goto deinit_timer;
    }
    cyhal_timer_cfg_t timer_config = {
        .is_continuous = false, .direction = CYHAL_TIMER_DIR_DOWN, .is_compare = false, .period = us, .compare_value = 0U, .value = us};
    if (cyhal_timer_configure(cy_timer, &timer_config) != CY_RSLT_SUCCESS)
    {
        goto deinit_timer;
    }
    if (cyhal_timer_start(cy_timer) != CY_RSLT_SUCCESS)
    {
        goto deinit_timer;
    }

    // Successfully started timer
    timer->_start = cy_timer;
    return IFX_SUCCESS;

deinit_timer:
    cyhal_timer_free(cy_timer);
free_timer:
    free(cy_timer);
    return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_UNSPECIFIED_ERROR);
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
    cyhal_timer_t *cy_timer = (cyhal_timer_t *) timer->_start;
    return cyhal_timer_read(cy_timer) == 0U;
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
    cyhal_timer_t *cy_timer = (cyhal_timer_t *) timer->_start;
    uint32_t remaining_us = cyhal_timer_read(cy_timer);
    uint32_t ms_to_sleep = remaining_us / 1000U;
    uint32_t us_to_sleep = remaining_us % 1000U;
    ifx_status_t result = IFX_SUCCESS;
    if (ms_to_sleep > 0U)
    {
        if (cyhal_system_delay_ms(ms_to_sleep) != 0U)
        {
            result = IFX_ERROR(LIB_TIMER, IFX_TIMER_JOIN, IFX_UNSPECIFIED_ERROR);
            goto cleanup;
        }
    }
    if (us_to_sleep > 0)
    {
        cyhal_system_delay_us(us_to_sleep);
    }
cleanup:
    cyhal_timer_free(cy_timer);
    return result;
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
        cyhal_timer_t *cy_timer = (cyhal_timer_t *) timer->_start;
        if (cy_timer != NULL)
        {
            cyhal_timer_free(cy_timer);
            free(cy_timer);
        }
        timer->_start = NULL;
        timer->_duration = 0U;
    }
}

#endif // (!defined(COMPONENT_FREERTOS) && !defined(NBT_TIMER_CUSTOM))
