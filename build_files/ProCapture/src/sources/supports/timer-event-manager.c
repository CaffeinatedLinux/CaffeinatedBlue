////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "timer-event-manager.h"
#include "drivers/timestamp.h"

static BOOLEAN _Add(xi_timer_event_manager *tm, LONGLONG llExpireTime, xi_timer * pTimer);
static BOOLEAN _Remove(xi_timer_event_manager *tm, xi_timer * pTimer);
static xi_timer *_PeekFirst(xi_timer_event_manager *tm);
static xi_timer *_GetFirst(xi_timer_event_manager *tm);
static void _ShiftUp(xi_timer_event_manager *tm, int nIndex, xi_timer *pTimer);
static void _ShiftDown(xi_timer_event_manager *tm, int nIndex, xi_timer *pTimer);


BOOLEAN xi_timer_event_manager_Create(xi_timer_event_manager *tm, struct xi_timestamp *pTimestamp, int cMaxTimers)
{
    xi_timer **ppTimers;

    tm->m_pTimestamp = pTimestamp;
    tm->m_ppTimers = NULL;
    tm->m_cMaxTimers = 0;
    tm->m_cTimers = 0;
    tm->m_cAlloc = 0;

    ppTimers = (xi_timer **)os_malloc(sizeof(xi_timer *) * cMaxTimers);
    if (NULL == ppTimers)
        return FALSE;

    tm->m_ppTimers = ppTimers;
    tm->m_cMaxTimers = cMaxTimers;

    tm->m_lock = os_spin_lock_alloc();
    if (tm->m_lock == NULL) {
        os_free(ppTimers);
        return FALSE;
    }

    return TRUE;
}

void xi_timer_event_manager_Destroy(xi_timer_event_manager *tm)
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

    tm->m_cTimers = 0;
    tm->m_cMaxTimers = 0;
    os_spin_unlock_bh(tm->m_lock);

    os_spin_lock_free(tm->m_lock);
}

xi_timer *xi_timer_event_manager_NewTimer(xi_timer_event_manager *tm,
                                          os_event_t event)
{
    xi_timer *pTimer = NULL;

    if (tm->m_cAlloc >= tm->m_cMaxTimers)
        goto out;

    pTimer = (xi_timer *)os_malloc(sizeof(xi_timer));
    if (NULL == pTimer)
        goto out;

    pTimer->m_nIndex = -1;

    if (event != NULL) {
        pTimer->event = event;
        pTimer->need_free = false;
    } else {
        pTimer->event = os_event_alloc();
        if (pTimer->event == NULL)
            goto event_err;
        pTimer->need_free = true;
    }

    os_spin_lock_bh(tm->m_lock); // malloc maybe sleep
    if (tm->m_cAlloc >= tm->m_cMaxTimers)
        goto clean;
    tm->m_cAlloc++;
    os_spin_unlock_bh(tm->m_lock);

    return pTimer;

clean:
    os_spin_unlock_bh(tm->m_lock);
    if (pTimer->event != NULL && pTimer->need_free) {
        os_event_free(pTimer->event);
    }
event_err:
    os_free(pTimer);
out:
    return NULL;
}

void xi_timer_event_manager_DeleteTimer(xi_timer_event_manager *tm, xi_timer *pTimer)
{
    xi_timer_event_manager_CancelTimer(tm, pTimer);

    os_spin_lock_bh(tm->m_lock);
    tm->m_cAlloc--;
    if (pTimer->event != NULL && pTimer->need_free)
        os_event_free(pTimer->event);
    os_free(pTimer);
    os_spin_unlock_bh(tm->m_lock);
}

BOOLEAN xi_timer_event_manager_ScheduleTimer(xi_timer_event_manager *tm, LONGLONG llTime, xi_timer *pTimer)
{
    os_spin_lock_bh(tm->m_lock);
    if (!_Add(tm, llTime, pTimer)) {
        os_spin_unlock_bh(tm->m_lock);
        return FALSE;
    }

    if (_PeekFirst(tm) == pTimer)
        xi_timestamp_set_alarm_time(tm->m_pTimestamp, llTime);
    os_spin_unlock_bh(tm->m_lock);

    return TRUE;
}

BOOLEAN xi_timer_event_manager_CancelTimer(xi_timer_event_manager *tm, xi_timer *pTimer)
{
    BOOLEAN bRemoveTop;

    os_spin_lock_bh(tm->m_lock);

    bRemoveTop = (pTimer == _PeekFirst(tm));

    if (!_Remove(tm, pTimer)) {
        os_spin_unlock_bh(tm->m_lock);
        return FALSE;
    }

    if (bRemoveTop) {
        pTimer = _PeekFirst(tm);
        xi_timestamp_set_alarm_time(tm->m_pTimestamp, pTimer ? pTimer->m_llExpireTime : TIMESTAMP_INFINITE);
    }
    os_spin_unlock_bh(tm->m_lock);
    return TRUE;
}

void xi_timer_event_manager_ProcessAlarm(xi_timer_event_manager *tm, LONGLONG lltime)
{
    xi_timer *pTimer;
    BOOLEAN bExpireTimeChanged = FALSE;

    os_spin_lock_bh(tm->m_lock);

    pTimer = _PeekFirst(tm);
    if (NULL == pTimer) {
        os_spin_unlock_bh(tm->m_lock);
        return;
    }

    while (pTimer && pTimer->m_llExpireTime <= lltime) {
        _GetFirst(tm);
        os_event_set(pTimer->event);
        pTimer = _PeekFirst(tm);
        bExpireTimeChanged = TRUE;
    }

    if (bExpireTimeChanged)
        xi_timestamp_set_alarm_time(tm->m_pTimestamp, pTimer ? pTimer->m_llExpireTime : TIMESTAMP_INFINITE);

    os_spin_unlock_bh(tm->m_lock);
}

void xi_timer_event_manager_Resume(xi_timer_event_manager *tm)
{
    xi_timer * pTimer;

    os_spin_lock_bh(tm->m_lock);
    pTimer = _PeekFirst(tm);
    if (pTimer)
        xi_timestamp_set_alarm_time(tm->m_pTimestamp, pTimer->m_llExpireTime);
    os_spin_unlock_bh(tm->m_lock);
}


/* local functions */
static BOOLEAN _Add(xi_timer_event_manager *tm, LONGLONG llExpireTime, xi_timer * pTimer)
{
    if (pTimer->m_nIndex >= 0)
        return FALSE;

    if (tm->m_cTimers + 1 > tm->m_cMaxTimers)
        return FALSE;

    pTimer->m_llExpireTime = llExpireTime;
    _ShiftUp(tm, tm->m_cTimers++, pTimer);
    return TRUE;
}

static BOOLEAN _Remove(xi_timer_event_manager *tm, xi_timer * pTimer)
{
    xi_timer *pLastTimer;
    int nParent;

    if (tm->m_cTimers <= 0)
        return FALSE;

    if (pTimer->m_nIndex < 0 || pTimer->m_nIndex >= tm->m_cTimers)
        return FALSE;

    pLastTimer = tm->m_ppTimers[--(tm->m_cTimers)];
    nParent = (pTimer->m_nIndex - 1) / 2;
    if (pTimer->m_nIndex > 0 && tm->m_ppTimers[nParent]->m_llExpireTime > pLastTimer->m_llExpireTime)
        _ShiftUp(tm, pTimer->m_nIndex, pLastTimer);
    else
        _ShiftDown(tm, pTimer->m_nIndex, pLastTimer);

    pTimer->m_nIndex = -1;
    return TRUE;
}

static xi_timer *_PeekFirst(xi_timer_event_manager *tm)
{
    if (tm->m_cTimers <= 0)
        return NULL;

    return tm->m_ppTimers[0];
}

static xi_timer *_GetFirst(xi_timer_event_manager *tm)
{
    xi_timer *pTimerTop;

    if (tm->m_cTimers <= 0)
        return NULL;

    pTimerTop = tm->m_ppTimers[0];
    _ShiftDown(tm, 0, tm->m_ppTimers[--tm->m_cTimers]);
    pTimerTop->m_nIndex = -1;
    return pTimerTop;
}

static void _ShiftUp(xi_timer_event_manager *tm, int nIndex, xi_timer *pTimer)
{
    int nParent = (nIndex - 1) / 2;
    while (nIndex && tm->m_ppTimers[nParent]->m_llExpireTime > pTimer->m_llExpireTime) {
        tm->m_ppTimers[nIndex] = tm->m_ppTimers[nParent];
        tm->m_ppTimers[nIndex]->m_nIndex = nIndex;
        nIndex = nParent;
        nParent = (nIndex - 1) / 2;
    }
    tm->m_ppTimers[nIndex] = pTimer;
    tm->m_ppTimers[nIndex]->m_nIndex = nIndex;
}

static void _ShiftDown(xi_timer_event_manager *tm, int nIndex, xi_timer *pTimer)
{
    int nMinChild = 2 * (nIndex + 1);
    while (nMinChild <= tm->m_cTimers) {
        if (nMinChild == tm->m_cTimers
                || tm->m_ppTimers[nMinChild]->m_llExpireTime > tm->m_ppTimers[nMinChild - 1]->m_llExpireTime)
            nMinChild -= 1;
        if (pTimer->m_llExpireTime <= tm->m_ppTimers[nMinChild]->m_llExpireTime)
            break;
        tm->m_ppTimers[nIndex] = tm->m_ppTimers[nMinChild];
        tm->m_ppTimers[nIndex]->m_nIndex = nIndex;
        nIndex = nMinChild;
        nMinChild = 2 * (nMinChild + 1);
    }
    tm->m_ppTimers[nIndex] = pTimer;
    tm->m_ppTimers[nIndex]->m_nIndex = nIndex;
}

