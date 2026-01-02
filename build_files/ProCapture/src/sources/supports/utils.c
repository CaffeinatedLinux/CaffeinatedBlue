////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////
#include "utils.h"

int xi_thread_request_init(struct xi_thread_request_ack *threq)
{
    int ret = 0;

    os_memset(threq, 0, sizeof(*threq));

    threq->m_event_request = os_event_alloc();
    if (threq->m_event_request == NULL) {
        ret = OS_RETURN_NOMEM;
        goto revent_err;
    }
    threq->m_event_complete = os_event_alloc();
    if (threq->m_event_complete == NULL) {
        ret = OS_RETURN_NOMEM;
        goto cevent_err;
    }

    threq->mutex = os_mutex_alloc();
    if (threq->mutex == NULL) {
        ret = OS_RETURN_NOMEM;
        goto mutex_err;
    }

    return OS_RETURN_SUCCESS;

mutex_err:
    os_event_free(threq->m_event_complete);
cevent_err:
    os_event_free(threq->m_event_request);
revent_err:
    threq->m_event_request = NULL;
    threq->m_event_complete = NULL;
    threq->mutex = NULL;
    return ret;
}

void xi_thread_request_deinit(struct xi_thread_request_ack *threq)
{
    if (threq->m_event_request != NULL)
        os_event_free(threq->m_event_request);
    if (threq->m_event_complete != NULL)
        os_event_free(threq->m_event_complete);
    if (threq->mutex != NULL)
        os_mutex_free(threq->mutex);

    threq->m_event_request = NULL;
    threq->m_event_complete = NULL;
    threq->mutex = NULL;
}

// Client side routines
int xi_thread_request_request(struct xi_thread_request_ack *threq, u32 cmd,
        void *param1, void *param2, void *param3)
{
    os_mutex_lock(threq->mutex);
    threq->m_cmd = cmd;
    threq->m_param1 = param1;
    threq->m_param2 = param2;
    threq->m_param3 = param3;

    os_event_set(threq->m_event_request);
    os_event_wait(threq->m_event_complete, -1);
    os_mutex_unlock(threq->mutex);

    return threq->m_response;
}

// Server side routines
os_event_t xi_thread_request_get_request_event(struct xi_thread_request_ack *threq)
{
    return threq->m_event_request;
}

u32 xi_thread_request_get_request_command(
        struct xi_thread_request_ack *threq, void **param1, void **param2, void **param3)
{
    if (param1)
        *param1 = threq->m_param1;
    if (param2)
        *param2 = threq->m_param2;
    if (param3)
        *param3 = threq->m_param3;

    return threq->m_cmd;
}

int32_t xi_thread_request_wait_for_request(struct xi_thread_request_ack *threq,
        u32 *cmd, void **param1, void **param2, void **param3, int32_t timeout)
{
    int32_t ret = 0;

    ret = os_event_wait(threq->m_event_request, timeout);

    if (cmd)
        *cmd = threq->m_cmd;
    if (param1)
        *param1 = threq->m_param1;
    if (param2)
        *param2 = threq->m_param2;
    if (param3)
        *param3 = threq->m_param3;

    return ret;
}

void xi_thread_request_ack(struct xi_thread_request_ack *threq, int response)
{
    threq->m_response = response;
    os_event_set(threq->m_event_complete);
}
