////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "sw-timer-event-manager.h"

static BOOLEAN _Add(xi_sw_timer_event_manager *tm, os_clock_kick_t llExpireTime, xi_timer * pTimer);
static BOOLEAN _Remove(xi_sw_timer_event_manager *tm, xi_timer * pTimer);
static xi_timer *_PeekFirst(xi_sw_timer_event_manager *tm);
static xi_timer *_GetFirst(xi_sw_timer_event_manager *tm);
static void _ShiftUp(xi_sw_timer_event_manager *tm, int nIndex, xi_timer *pTimer);
static void _ShiftDown(xi_sw_timer_event_manager *tm, int nIndex, xi_timer *pTimer);


static void _set_alarm_time(xi_sw_timer_event_manager *tm, os_clock_kick_t alarm_time);
void xi_sw_timer_event_manager_ProcessAlarm(void *data);

BOOLEAN xi_sw_timer_event_manager_Create(xi_sw_timer_event_manager *tm,
        os_event_t event, int cMaxTimers)
{
    xi_timer **ppTimers;

    tm->m_ppTimers = NULL;
    tm->m_cMaxTimers = 0;
    tm->m_cTimers = 0;
    tm->m_cAlloc = 0;

    tm->event = event;

    ppTimers = (xi_timer **)os_malloc(sizeof(xi_timer *) * cMaxTimers);
    if (NULL == ppTimers)
        return FALSE;

    tm->m_lock = os_spin_lock_alloc();
    if (tm->m_lock == NULL) {
        os_free(ppTimers);
        return FALSE;
    }

    tm->timer = os_timer_alloc(xi_sw_timer_event_manager_ProcessAlarm, tm);
    if (tm->timer == NULL) {
        os_free(ppTimers);
        os_spin_lock_free(tm->m_lock);
        tm->m_lock = NULL;
        return FALSE;
    }

    tm->m_ppTimers = ppTimers;
    tm->m_cMaxTimers = cMaxTimers;

    INIT_LIST_HEAD(&tm->m_listTimerEvent);

    return TRUE;
}

void xi_sw_timer_event_manager_Destroy(xi_sw_timer_event_manager *tm)
{
    xi_timer *pTimer = NULL;

    os_spin_lock_bh(tm->m_lock);

    pTimer = _GetFirst(tm);
    while (pTimer) {
        os_free(pTimer);
        pTimer = _GetFirst(tm);
    }

    if (tm->m_ppTimers) {
        os_free(tm->m_ppTimers);
        tm->m_ppTimers = NULL;
    }
    os_spin_unlock_bh(tm->m_lock);

    os_spin_lock_free(tm->m_lock);

    if (tm->timer != NULL) {
        os_timer_free(tm->timer);
        tm->timer = NULL;
    }

    tm->m_cTimers = 0;
    tm->m_cMaxTimers = 0;
}

xi_timer *xi_sw_timer_event_manager_NewTimer(xi_sw_timer_event_manager *tm)
{
    xi_timer *pTimer = NULL;

    pTimer = (xi_timer *)os_malloc(sizeof(xi_timer));
    if (NULL == pTimer)
        goto out;

    pTimer->m_nIndex = -1;
    pTimer->event = os_event_alloc();
    if (pTimer->event == NULL)
        goto event_err;

    INIT_LIST_HEAD(&pTimer->user_node);

    os_spin_lock_bh(tm->m_lock); // malloc maybe sleep
    if (tm->m_cAlloc >= tm->m_cMaxTimers)
        goto clean;
    tm->m_cAlloc++;
    os_spin_unlock_bh(tm->m_lock);

    return pTimer;

clean:
    os_spin_unlock_bh(tm->m_lock);
    if (pTimer->event != NULL) {
        os_event_free(pTimer->event);
    }
event_err:
    os_free(pTimer);
out:
    return NULL;
}

void xi_sw_timer_event_manager_DeleteTimer(xi_sw_timer_event_manager *tm, xi_timer *pTimer)
{
    xi_sw_timer_event_manager_CancelTimer(tm, pTimer);

    os_spin_lock_bh(tm->m_lock);
    tm->m_cAlloc--;
    if (pTimer->event != NULL)
        os_event_free(pTimer->event);
    os_free(pTimer);
    os_spin_unlock_bh(tm->m_lock);
}

BOOLEAN xi_sw_timer_event_manager_ScheduleTimerRelative(xi_sw_timer_event_manager *tm,
        DWORD timeout, xi_timer *pTimer)
{
    os_clock_kick_t alarm_time;

    xi_sw_timer_event_manager_CancelTimer(tm, pTimer);

    os_spin_lock_bh(tm->m_lock);
    alarm_time = os_get_clock_kick() + os_msecs_to_clock_kick(timeout);
    if (!_Add(tm, alarm_time, pTimer)) {
        os_spin_unlock_bh(tm->m_lock);
        return FALSE;
    }

    if (_PeekFirst(tm) == pTimer)
        _set_alarm_time(tm, alarm_time);
    os_spin_unlock_bh(tm->m_lock);

    return TRUE;
}

BOOLEAN xi_sw_timer_event_manager_CancelTimer(xi_sw_timer_event_manager *tm, xi_timer *pTimer)
{
    BOOLEAN bRemoveTop;

    os_spin_lock_bh(tm->m_lock);

    bRemoveTop = (pTimer == _PeekFirst(tm));

    if (!_Remove(tm, pTimer)) {
        if (!list_empty(&pTimer->user_node)) {
            list_del_init(&pTimer->user_node);
            os_spin_unlock_bh(tm->m_lock);
            return TRUE;
        }
        os_spin_unlock_bh(tm->m_lock);
        return FALSE;
    }

    if (bRemoveTop) {
        pTimer = _PeekFirst(tm);
        if (pTimer != NULL)
            _set_alarm_time(tm, pTimer->m_llExpireTime);
        else
            os_timer_cancel(tm->timer);
    }
    os_spin_unlock_bh(tm->m_lock);
    return TRUE;
}

xi_timer *xi_sw_timer_event_manager_GetExpiredTimer(xi_sw_timer_event_manager *tm)
{
    xi_timer *pTimer = NULL;

    os_spin_lock_bh(tm->m_lock);
    if (!list_empty(&tm->m_listTimerEvent)) {
        pTimer = list_entry(tm->m_listTimerEvent.next,
                xi_timer, user_node);
        list_del_init(&pTimer->user_node);
    }
    os_spin_unlock_bh(tm->m_lock);

    return pTimer;
}

void xi_sw_timer_event_manager_ProcessAlarm(void *data)
{
    xi_sw_timer_event_manager *tm = (xi_sw_timer_event_manager *)data;
    xi_timer *pTimer;
    BOOLEAN bExpireTimeChanged = FALSE;

    os_spin_lock_bh(tm->m_lock);

    pTimer = _PeekFirst(tm);
    if (NULL == pTimer) {
        os_spin_unlock_bh(tm->m_lock);
        return;
    }

    while (pTimer &&
            os_clock_kick_before_eq((os_clock_kick_t)pTimer->m_llExpireTime, os_get_clock_kick())) {
        _GetFirst(tm);
        if (tm->event) {
            list_add_tail(&pTimer->user_node, &tm->m_listTimerEvent);
            os_event_set(tm->event);
        } else {
            os_event_set(pTimer->event);
        }
        pTimer = _PeekFirst(tm);
        bExpireTimeChanged = TRUE;
    }

    if (bExpireTimeChanged) {
        if (pTimer != NULL)
            _set_alarm_time(tm, pTimer->m_llExpireTime);
        else
            os_timer_cancel(tm->timer);
    }

    os_spin_unlock_bh(tm->m_lock);
}


/* local functions */
static void _set_alarm_time(xi_sw_timer_event_manager *tm, os_clock_kick_t alarm_time)
{
    if (os_clock_kick_before(alarm_time, os_get_clock_kick()))
        alarm_time = os_get_clock_kick();

    os_timer_schedule_clock_kick(tm->timer, alarm_time);
}

static BOOLEAN _Add(xi_sw_timer_event_manager *tm, os_clock_kick_t llExpireTime, xi_timer * pTimer)
{
    if (pTimer->m_nIndex >= 0)
        return FALSE;

    if (tm->m_cTimers + 1 > tm->m_cMaxTimers)
        return FALSE;

    pTimer->m_llExpireTime = llExpireTime;
    _ShiftUp(tm, tm->m_cTimers++, pTimer);
    return TRUE;
}

static BOOLEAN _Remove(xi_sw_timer_event_manager *tm, xi_timer * pTimer)
{
    xi_timer *pLastTimer;
    int nParent;

    if (tm->m_cTimers <= 0)
        return FALSE;

    if (pTimer->m_nIndex < 0 || pTimer->m_nIndex >= tm->m_cTimers)
        return FALSE;

    pLastTimer = tm->m_ppTimers[--(tm->m_cTimers)];
    nParent = (pTimer->m_nIndex - 1) / 2;
    if (pTimer->m_nIndex > 0
            && os_clock_kick_after((os_clock_kick_t)tm->m_ppTimers[nParent]->m_llExpireTime,
                                   (os_clock_kick_t)pLastTimer->m_llExpireTime))
        _ShiftUp(tm, pTimer->m_nIndex, pLastTimer);
    else
        _ShiftDown(tm, pTimer->m_nIndex, pLastTimer);

    pTimer->m_nIndex = -1;
    return TRUE;
}

static xi_timer *_PeekFirst(xi_sw_timer_event_manager *tm)
{
    if (tm->m_cTimers <= 0)
        return NULL;

    return tm->m_ppTimers[0];
}

static xi_timer *_GetFirst(xi_sw_timer_event_manager *tm)
{
    xi_timer *pTimerTop;

    if (tm->m_cTimers <= 0)
        return NULL;

    pTimerTop = tm->m_ppTimers[0];
    _ShiftDown(tm, 0, tm->m_ppTimers[--tm->m_cTimers]);
    pTimerTop->m_nIndex = -1;
    return pTimerTop;
}

static void _ShiftUp(xi_sw_timer_event_manager *tm, int nIndex, xi_timer *pTimer)
{
    int nParent = (nIndex - 1) / 2;
    while (nIndex && os_clock_kick_after((os_clock_kick_t)tm->m_ppTimers[nParent]->m_llExpireTime,
                                         (os_clock_kick_t)pTimer->m_llExpireTime)) {
        tm->m_ppTimers[nIndex] = tm->m_ppTimers[nParent];
        tm->m_ppTimers[nIndex]->m_nIndex = nIndex;
        nIndex = nParent;
        nParent = (nIndex - 1) / 2;
    }
    tm->m_ppTimers[nIndex] = pTimer;
    tm->m_ppTimers[nIndex]->m_nIndex = nIndex;
}

static void _ShiftDown(xi_sw_timer_event_manager *tm, int nIndex, xi_timer *pTimer)
{
    int nMinChild = 2 * (nIndex + 1);
    while (nMinChild <= tm->m_cTimers) {
        if (nMinChild == tm->m_cTimers
                || os_clock_kick_after((os_clock_kick_t)tm->m_ppTimers[nMinChild]->m_llExpireTime,
                                       (os_clock_kick_t)tm->m_ppTimers[nMinChild - 1]->m_llExpireTime)
                )
            nMinChild -= 1;
        if (os_clock_kick_before_eq((os_clock_kick_t)pTimer->m_llExpireTime,
                                    (os_clock_kick_t)tm->m_ppTimers[nMinChild]->m_llExpireTime)
                )
            break;
        tm->m_ppTimers[nIndex] = tm->m_ppTimers[nMinChild];
        tm->m_ppTimers[nIndex]->m_nIndex = nIndex;
        nIndex = nMinChild;
        nMinChild = 2 * (nMinChild + 1);
    }
    tm->m_ppTimers[nIndex] = pTimer;
    tm->m_ppTimers[nIndex]->m_nIndex = nIndex;
}

