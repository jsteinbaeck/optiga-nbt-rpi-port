// SPDX-FileCopyrightText: 2024 Infineon Technologies AG
// SPDX-License-Identifier: MIT

/**
 * \file timer-rpi.c
 * \brief Timer API implementation for NBT framework based on Raspberry PI Linux OS.
 * \details This version uses the POSIX timers.
 */


#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <signal.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "infineon/ifx-error.h"
#include "infineon/ifx-timer.h"

/* Timer._start structure */
struct posix_timer_rpi {
    timer_t timerId;
    struct sigevent sev;
    struct sigaction sa;
    struct itimerspec its;
    bool is_timer_elapsed;
};



/**
 * \brief 
*/
/**
 * \brief Handler to execute when time elapsed. Sets the elapsed flag to true.
 *
 * \param[in] sig signal number of the signal being delivered
 * \param[in] si us Timer duration in [us].
 * \param[in] uc us Timer duration in [us].
 */
static void handler(int sig, siginfo_t *si, void *uc)
{
    (void)sig;
    (void)uc;
    struct posix_timer_rpi *data = (struct posix_timer_rpi *) si->_sifields._rt.si_sigval.sival_ptr;
    data->is_timer_elapsed = true;
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
    if ((timer == NULL) || (us > UINT32_MAX))
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_ILLEGAL_ARGUMENT);
    }

    // Allocate memory for timer information
    struct posix_timer_rpi *rpi_timer = malloc(sizeof(struct posix_timer_rpi));
    if (rpi_timer == NULL)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_OUT_OF_MEMORY);
    }

    /* Interrupt initialization */
    rpi_timer->sev.sigev_notify = SIGEV_SIGNAL; // Linux-specific
    rpi_timer->sev.sigev_signo = SIGRTMIN;
    rpi_timer->sev.sigev_value.sival_ptr = rpi_timer;

    /* specifies signal and handler */
    rpi_timer->sa.sa_flags = SA_SIGINFO;
    rpi_timer->sa.sa_sigaction = handler;

    /* Start and delay initialization */
    rpi_timer->its.it_value.tv_sec  = 0;
    rpi_timer->its.it_value.tv_nsec = us * 1000;
    rpi_timer->its.it_interval.tv_sec  = 0;
    rpi_timer->its.it_interval.tv_nsec = 0;

    /* Set elapsed to false */
    rpi_timer->is_timer_elapsed = false;

    /* Initialize the POSIX timer */
    if (timer_create(CLOCK_REALTIME, &rpi_timer->sev, &rpi_timer->timerId) != 0)
    {
        goto free_timer;   
    }
    
    /* Initialize signal */
    sigemptyset(&rpi_timer->sa.sa_mask);

    /* Register signal handler */
    if (sigaction(SIGRTMIN, &rpi_timer->sa, NULL) == -1){
        goto deinit_timer;
    }

    /* Start the timer */
    if (timer_settime(rpi_timer->timerId, 0, &rpi_timer->its, NULL) != 0)
    {
        goto deinit_timer;
    }

    // Successfully started timer
    timer->_start = rpi_timer;
    return IFX_SUCCESS;

deinit_timer:
    if (timer_delete(rpi_timer->timerId) != 0)
    {
        return IFX_ERROR(LIB_TIMER, IFX_TIMER_SET, IFX_UNSPECIFIED_ERROR);
    }
free_timer:
    free(rpi_timer);
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

    struct posix_timer_rpi *rpi_timer = (struct posix_timer_rpi *) timer->_start;
    return rpi_timer->is_timer_elapsed == true;
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
    
    struct posix_timer_rpi *rpi_timer = (struct posix_timer_rpi *) timer->_start;
    ifx_status_t result = IFX_SUCCESS;

    while (rpi_timer->is_timer_elapsed == false)
        ;

    rpi_timer->is_timer_elapsed = false;

cleanup:
    if (timer_delete(rpi_timer->timerId) != 0)
    {
        result = IFX_ERROR(LIB_TIMER, IFX_TIMER_JOIN, IFX_UNSPECIFIED_ERROR);
    }

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
        struct posix_timer_rpi *rpi_timer = (struct posix_timer_rpi *) timer->_start;
        if (rpi_timer != NULL)
        {
            timer_delete(rpi_timer->timerId);
            free(rpi_timer);
        }

        timer->_start = NULL;
        timer->_duration = 0U;
    }
}
