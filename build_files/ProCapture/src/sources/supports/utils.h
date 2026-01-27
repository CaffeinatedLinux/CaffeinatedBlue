////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing) 
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ospi/ospi.h"

/* xi_thread_request_ack */
struct xi_thread_request_ack {
    os_event_t                  m_event_request;
    os_event_t                  m_event_complete;

    u32                         m_cmd;
    void                       *m_param1;
    void                       *m_param2;
    void                       *m_param3;
    int                         m_response;

    /* lock for multi client */
    os_mutex_t                  mutex;
};

int xi_thread_request_init(struct xi_thread_request_ack *threq);

void xi_thread_request_deinit(struct xi_thread_request_ack *threq);

// Client side routines
int xi_thread_request_request(struct xi_thread_request_ack *threq, u32 cmd,
        void *param1, void *param2, void *param3);

// Server side routines
os_event_t xi_thread_request_get_request_event(struct xi_thread_request_ack *threq);

u32 xi_thread_request_get_request_command(struct xi_thread_request_ack *threq,
        void **param1, void **param2, void **param3);

int32_t xi_thread_request_wait_for_request(struct xi_thread_request_ack *threq,
        u32 *cmd, void **param1, void **param2, void **param3, int32_t timeout);

void xi_thread_request_ack(struct xi_thread_request_ack *threq, int response);

