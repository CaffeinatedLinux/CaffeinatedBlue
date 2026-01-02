////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __TIMER_EVENT_MANAGER_H__
#define __TIMER_EVENT_MANAGER_H__

#include "ospi/ospi.h"

#include "xi-timer.h"
#include "win-types.h"

typedef struct _xi_timer_event_manager {
    xi_timer             **m_ppTimers;
    int                    m_cMaxTimers;
    int                    m_cTimers;
    int                    m_cAlloc;

    struct xi_timestamp   *m_pTimestamp;

    os_spinlock_t          m_lock;
} xi_timer_event_manager;


BOOLEAN xi_timer_event_manager_Create(xi_timer_event_manager *tm, struct xi_timestamp *pTimestamp, int cMaxTimers);

void xi_timer_event_manager_Destroy(xi_timer_event_manager *tm);

xi_timer *xi_timer_event_manager_NewTimer(xi_timer_event_manager *tm,
                                          os_event_t event);

void xi_timer_event_manager_DeleteTimer(xi_timer_event_manager *tm, xi_timer *pTimer);

BOOLEAN xi_timer_event_manager_ScheduleTimer(xi_timer_event_manager *tm, LONGLONG llTime, xi_timer *pTimer);

BOOLEAN xi_timer_event_manager_CancelTimer(xi_timer_event_manager *tm, xi_timer *pTimer);

void xi_timer_event_manager_ProcessAlarm(xi_timer_event_manager *tm, LONGLONG lltime);

void xi_timer_event_manager_Resume(xi_timer_event_manager *tm);

#endif /* __TIMER_EVENT_MANAGER_H__ */

