/****************************************************************************
 * net/netlink/netlink.h
 *
 *   Copyright (C) 2019 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __NET_NETLINK_NETLINK_H
#define __NET_NETLINK_NETLINK_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <queue.h>
#include <semaphore.h>
#include <signal.h>
#include <poll.h>

#include <netpacket/netlink.h>
#include <nuttx/net/netlink.h>

#include "devif/devif.h"
#include "socket/socket.h"

#ifdef CONFIG_NET_NETLINK

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NETLINK_NO_WAITER ((pid_t)-1)

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

/* This "connection" structure describes the underlying state of the socket. */

struct netlink_conn_s
{
  /* Common prologue of all connection structures. */

  dq_entry_t node;                   /* Supports a doubly linked list */

  /* This is a list of NetLink connection callbacks.  Each callback
   * represents a thread that is stalled, waiting for a device-specific
   * event.
   */

  FAR struct devif_callback_s *list; /* NetLink callbacks */

  /* NetLink-specific content follows */

  uint32_t pid;                      /* Port ID (if bound) */
  uint32_t groups;                   /* Multicast groups mask (if bound) */
  uint8_t crefs;                     /* Reference counts on this instance */
  uint8_t protocol;                  /* See NETLINK_* definitions */

  /* poll() support */

  FAR sem_t *pollsem;                /* Used to wakeup poll() */
  FAR pollevent_t *pollevent;        /* poll() wakeup event */
  struct sigaction oact;             /* Previous signal action */
  
  /* Threads waiting for a response */

  pid_t waiter[CONFIG_NETLINK_MAXPENDING];

  /* Queued response data */

  sq_queue_t resplist;               /* Singly linked list of responses*/
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#  define EXTERN extern "C"
extern "C"
{
#else
#  define EXTERN extern
#endif

EXTERN const struct sock_intf_s g_netlink_sockif;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct sockaddr_nl;  /* Forward reference */

/****************************************************************************
 * Name: netlink_initialize()
 *
 * Description:
 *   Initialize the NetLink connection structures.  Called once and only
 *   from the networking layer.
 *
 ****************************************************************************/

void netlink_initialize(void);

/****************************************************************************
 * Name: netlink_alloc()
 *
 * Description:
 *   Allocate a new, uninitialized NetLink connection structure.  This is
 *   normally something done by the implementation of the socket() API
 *
 ****************************************************************************/

FAR struct netlink_conn_s *netlink_alloc(void);

/****************************************************************************
 * Name: netlink_free()
 *
 * Description:
 *   Free a NetLink connection structure that is no longer in use. This should
 *   be done by the implementation of close().
 *
 ****************************************************************************/

void netlink_free(FAR struct netlink_conn_s *conn);

/****************************************************************************
 * Name: netlink_nextconn()
 *
 * Description:
 *   Traverse the list of allocated NetLink connections
 *
 * Assumptions:
 *   This function is called from NetLink device logic.
 *
 ****************************************************************************/

FAR struct netlink_conn_s *netlink_nextconn(FAR struct netlink_conn_s *conn);

/****************************************************************************
 * Name: netlink_active()
 *
 * Description:
 *   Find a connection structure that is the appropriate connection for the
 *   provided NetLink address
 *
 ****************************************************************************/

FAR struct netlink_conn_s *netlink_active(FAR struct sockaddr_nl *addr);

/****************************************************************************
 * Name: netlink_tryget_response
 *
 * Description:
 *   Return the next response from the head of the pending response list.
 *   Responses are returned one-at-a-time in FIFO order.
 *
 *   Note:  The network will be momentarily locked to support exclusive
 *   access to the pending response list.
 *
 * Returned Value:
 *   The next response from the head of the pending response list is
 *   returned.  NULL will be returned if the pending response list is
 *   empty
 *
 ****************************************************************************/

FAR struct netlink_response_s *
  netlink_tryget_response(FAR struct socket *psock);

/****************************************************************************
 * Name: netlink_get_response
 *
 * Description:
 *   Return the next response from the head of the pending response list.
 *   Responses are returned one-at-a-time in FIFO order.
 *
 *   Note:  The network will be momentarily locked to support exclusive
 *   access to the pending response list.
 *
 * Returned Value:
 *   The next response from the head of the pending response list is
 *   always returned.  This function will block until a response is
 *   received if the pending response list is empty.
 *
 ****************************************************************************/

FAR struct netlink_response_s *
  netlink_get_response(FAR struct socket *psock);

/****************************************************************************
 * Name: netlink_check_response
 *
 * Description:
 *   Return true is a response is pending now.
 *
 * Returned Value:
 *   True: A response is available; False; No response is available.
 *
 ****************************************************************************/

bool netlink_check_response(FAR struct socket *psock);

/****************************************************************************
 * Name: netlink_notify_response
 *
 * Description:
 *   Notify a thread until a response be available.  The thread will be
 *   notified via CONFIG_NETLINK_SIGNAL when the response becomes available.
 *
 * Returned Value:
 *   Zero (OK) is returned if the response is already available.  Not signal
 *     will be sent.
 *   One is returned if the notification was successfully setup.
 *   A negated errno value is returned on any failure.
 *
 ****************************************************************************/

int netlink_notify_response(FAR struct socket *psock);

/****************************************************************************
 * Name: netlink_notify_cancel
 *
 * Description:
 *   Cancel a notification previously created with netlink_notify_response().
 *
 * Returned Value:
 *   Zero (OK) is always returned.
 *
 ****************************************************************************/

int netlink_notify_cancel(FAR struct socket *psock);

/****************************************************************************
 * Name: netlink_route_sendto()
 *
 * Description:
 *   Perform the sendto() operation for the NETLINK_ROUTE protocol.
 *
 ****************************************************************************/

#ifdef CONFIG_NETLINK_ROUTE
ssize_t netlink_route_sendto(FAR struct socket *psock,
                             FAR const struct nlmsghdr *nlmsg,
                             size_t len, int flags,
                             FAR const struct sockaddr_nl *to,
                             socklen_t tolen);
#endif

/****************************************************************************
 * Name: netlink_route_recvfrom()
 *
 * Description:
 *   Perform the recvfrom() operation for the NETLINK_ROUTE protocol.
 *
 ****************************************************************************/

#ifdef CONFIG_NETLINK_ROUTE
ssize_t netlink_route_recvfrom(FAR struct socket *psock,
                               FAR struct nlmsghdr *nlmsg,
                               size_t len, int flags,
                               FAR struct sockaddr_nl *from);
#endif

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_NET_NETLINK */
#endif /* __NET_NETLINK_NETLINK_H */
