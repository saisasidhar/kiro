/* Copyright (C) 2014 Timo Dritschler <timo.dritschler@kit.edu>
   (Karlsruhe Institute of Technology)

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published by the
   Free Software Foundation; either version 2.1 of the License, or (at your
   option) any later version.

   This library is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
   details.

   You should have received a copy of the GNU Lesser General Public License along
   with this library; if not, write to the Free Software Foundation, Inc., 51
   Franklin St, Fifth Floor, Boston, MA 02110, USA
*/

/**
 * SECTION: kiro-client
 * @Short_description: KIRO RDMA Client / Consumer
 * @Title: KiroClient
 *
 * KiroClient implements the client / active / consumer side of the the RDMA
 * Communication Channel. It uses a KIRO-CLIENT to manage data read from the Server.
 */

#ifndef __KIRO_CLIENT_H
#define __KIRO_CLIENT_H

#include <stdint.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define KIRO_TYPE_CLIENT             (kiro_client_get_type())
#define KIRO_CLIENT(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), KIRO_TYPE_CLIENT, KiroClient))
#define KIRO_IS_CLIENT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), KIRO_TYPE_CLIENT))
#define KIRO_CLIENT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), KIRO_TYPE_CLIENT, KiroClientClass))
#define KIRO_IS_CLIENT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass), KIRO_TYPE_CLIENT))
#define KIRO_CLIENT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj), KIRO_TYPE_CLIENT, KiroClientClass))


typedef struct _KiroClient           KiroClient;
typedef struct _KiroClientClass      KiroClientClass;
typedef struct _KiroClientPrivate    KiroClientPrivate;


struct _KiroClient {

    GObject parent;

    /*< private >*/
    KiroClientPrivate *priv;
};

struct _KiroClientClass {

    GObjectClass parent_class;

};



/* GObject and GType functions */
GType       kiro_client_get_type            (void);

/**
 * kiro_client_new:
 *
 * Creates a new, unconnected #KiroClient and returns a pointer to it.
 *
 * Returns: (transfer full): A pointer to a new #KiroClient
 * See also:
 *   kiro_client_free, kiro_client_connect
 */
KiroClient* kiro_client_new                 (void);

/**
 * kiro_client_free:
 * @client: (transfer none): The #KiroClient that is to be freed
 *
 *   Transitions the #KiroServer through all necessary shutdown routines and
 *   frees the object memory.
 *
 * Note:
 *   The memory content that has been transfered from the server is
 *   automatically freed when calling this function. If you want to continue
 *   using the memory after freeing the #KiroClient, make sure to memcpy() it
 *   first, using the informations obtained from kiro_client_get_memory() and
 *   kiro_client_get_memory_size().
 * See also:
 *   kiro_client_new, kiro_client_connect
 */
void        kiro_client_free                (KiroClient *client);


/* client functions */

/**
 * kiro_client_connect:
 * @client: (transfer none): The #KiroClient to connect
 * @dest_addr: (transfer none): The address of the target server
 * @dest_port: (transfer none): The port of the target server
 *
 *   Connects the given #KiroClient to a KIRO server described by @dest_addr and
 *   @dest_port.
 *
 * Returns:
 *   0 if the connection was successful, -1 in case of connection error
 *
 * Note:
 *   When building a connection to the server, memory for the transmission is
 *   created as well.
 * See also:
 *   kiro_server_new
 */
int         kiro_client_connect             (KiroClient *client, const char *dest_addr, const char *dest_port);

/**
 * kiro_client_disconnect:
 * @client: (transfer none): The #KiroClient to disconnect
 *
 *   Disconnects the given #KiroClient from the KIRO server that it is connected
 *   to. If the @client is not connected, this function has no effect.
 *
 * Note:
 *   The memory content that has been transfered from the server is
 *   automatically freed when calling this function. If you want to continue
 *   using the memory after disconnecting the @client, make sure to memcpy() it
 *   first, using the informations obtained from kiro_client_get_memory() and
 *   kiro_client_get_memory_size().
 * See also:
 *   kiro_server_connect
 */
void        kiro_client_disconnect             (KiroClient *client);

/**
 * kiro_client_sync:
 * @client: (transfer none): The #KiroServer to use sync on
 *
 *   This synchronizes the client with the server, clining the memory
 *   provided by the server to the local client. The memory can be accessed by
 *   using kiro_client_get_memory().
 *
 * Returns:
 *   0 if successful, -1 in case of synchronisation error
 * Note:
 *   The server can send a 'reallocation' request to the client, forcing it to
 *   reallocate new memory freeing the old memory in the process. This might
 *   change remote and local memory layout at any time!
 *See also:
 *    kiro_client_sync_partial, kiro_client_get_memory, kiro_cient_connect
 */
int         kiro_client_sync                (KiroClient *client);

/**
 * kiro_client_sync_partial:
 * @client: (transfer none): The #KiroServer to use sync on
 * @remote_offset: remote read offset in bytes
 * @size: ammount of bytes to read. 0 for 'until end'
 * @local_offset: offset for the storage in the local buffer
 *
 *   This synchronizes the client with the server, clining the memory
 *   provided by the server to the local client. The memory can be accessed by
 *   using kiro_client_get_memory(). Uses the offset parameters to determine
 *   which memory region to read from the server and where to store the
 *   information to.
 *
 * Returns:
 *   0 if successful, -1 in case of synchronisation error
 * Note:
 *   The server can send a 'reallocation' request to the client, forcing it to
 *   reallocate new memory freeing the old memory in the process. This might
 *   change remote and local memory layout at any time!
 *See also:
 *    kiro_client_sync, kiro_client_get_memory, kiro_cient_connect
 */
int         kiro_client_sync_partial        (KiroClient *client, gulong remote_offset, gulong size, gulong local_offset);

/**
 * kiro_client_ping_server:
 * @client: (transfer none): The #KiroServer to send the PING from
 *
 *   Sends a PING package to the connected #KiroServer and waits for a PONG
 *   package from that server. The time between sending the PING and receiving
 *   the PONG (in microseconds) is measured and returned by this function.
 *
 * Returns:
 *   A #guint telling the time (in microseconds) how long it took for the
 *   connected #KiroServer to reply
 */
gint        kiro_client_ping_server         (KiroClient *client);

/**
 * kiro_client_get_memory:
 * @client: (transfer none): The #KiroClient to get the memory from
 *
 *    Provides a pointer to the content of the internal memory that was pulled
 *    from the server.
 *
 * Note:
 *    The server can instruct the client to reallocate memory on the next
 *    occurrence of kiro_client_sync(), freeing the old memory. Also, calling
 *    kiro_client_free() will free the client memory as well. If you need to
 *    make sure that the memory from the @client remains accessible after
 *    calling sync and/or free, you need to memcpy() the memory using the
 *    information from kiro_client_get_memory() and
 *    kiro_client_get_memory_size() first.
 *    The returned memory might under NO circumstances be freed by the user!
 * Returns: (transfer none):
 *    A pointer to the current memory of the client.
 * See also:
 *    kiro_client_get_memory_size, kiro_client_sync
 */
void*       kiro_client_get_memory          (KiroClient *client);

/**
 * kiro_client_get_memory_size:
 * @client: (transfer none): The #KiroClient to get the memory size of
 *
 *    Returns the size of the allocated memory of @client, in bytes.
 *
 * Returns:
 *    The size of the given #KiroClient memory in bytes
 * Note:
 *    The server can instruct the client to reallocate memroy on the next
 *    occurrence of kiro_server_sync(), freeing the old memory. This might also
 *    effect the respective memory size.
 * See also:
 *    kiro_client_get_memory, kiro_client_sync
 */
size_t      kiro_client_get_memory_size     (KiroClient *client);

G_END_DECLS

#endif //__KIRO_CLIENT_H
