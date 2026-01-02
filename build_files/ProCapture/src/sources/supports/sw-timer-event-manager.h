////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#ifndef __SW_TIMER_EVENT_MANAGER_H__
#define __SW_TIMER_EVENT_MANAGER_H__

#include "ospi/ospi.h"

#include "xi-timer.h"
#include "win-types.h"

typedef struct _xi_sw_timer_event_manager {
    xi_timer                **m_ppTimers;
    int                     m_cMaxTimers;
    int                     m_cTimers;
    int                     m_cAlloc;

    os_spinlock_t           m_lock;

    struct list_head        m_listTimerEvent;
    os_event_t              event;

    os_timer                timer;
} xi_sw_timer_event_manager;


BOOLEAN xi_sw_timer_event_manager_Create(xi_sw_timer_event_manager *tm,
        os_event_t event, int cMaxTimers);

void xi_sw_timer_event_manager_Destroy(xi_sw_timer_event_manager *tm);

xi_timer *xi_sw_timer_event_manager_NewTimer(xi_sw_timer_event_manager *tm);

void xi_sw_timer_event_manager_DeleteTimer(xi_sw_timer_event_manager *tm, xi_timer *pTimer);

BOOLEAN xi_sw_timer_event_manager_ScheduleTimerRelative(xi_sw_timer_event_manager *tm,
        DWORD timeout, xi_timer *pTimer);

BOOLEAN xi_sw_timer_event_manager_CancelTimer(xi_sw_timer_event_manager *tm, xi_timer *pTimer);

xi_timer *xi_sw_timer_event_manager_GetExpiredTimer(xi_sw_timer_event_manager *tm);

#endif /* __TIMER_EVENT_MANAGER_H__ */

