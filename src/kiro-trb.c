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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "kiro-trb.h"


/*
 * Definition of 'private' structures and members and macro to access them
 */

#define KIRO_TRB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), KIRO_TYPE_TRB, KiroTrbPrivate))

struct _KiroTrbPrivate {

    /* Properties */
    // PLACEHOLDER //

    /* 'Real' private structures */
    /* (Not accessible by properties) */
    int         initialized;    // 1 if Buffer is Valid, 0 otherwise
    void        *mem;            // Access to the actual buffer in Memory
    void        *frame_top;      // First byte of the buffer storage
    void        *current;        // Pointer to the current fill state
    uint64_t    element_size;
    uint64_t    max_elements;
    uint64_t    iteration;      // How many times the buffer has wraped around

    /* easy access */
    uint64_t    buff_size;
};


G_DEFINE_TYPE(KiroTrb, kiro_trb, G_TYPE_OBJECT);


KiroTrb *
kiro_trb_new (void)
{
    return g_object_new (KIRO_TYPE_TRB, NULL);
}


void
kiro_trb_free (KiroTrb *trb)
{
    g_return_if_fail (trb != NULL);
    if (KIRO_IS_TRB (trb))
        g_object_unref (trb);
    else
        g_warning ("Trying to use kiro_trb_free on an object which is not a KIRO TRB. Ignoring...");
}


static
void kiro_trb_init (KiroTrb *self)
{
    g_return_if_fail (self != NULL);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);
    priv->initialized = 0;
}


static void
kiro_trb_finalize (GObject *object)
{
    g_return_if_fail (object != NULL);
    KiroTrb *self = KIRO_TRB (object);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->mem)
        g_free (priv->mem);

    G_OBJECT_CLASS (kiro_trb_parent_class)->finalize (object);
}


static void
kiro_trb_class_init (KiroTrbClass *klass)
{
    g_return_if_fail (klass != NULL);
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    gobject_class->finalize = kiro_trb_finalize;
    g_type_class_add_private (klass, sizeof (KiroTrbPrivate));
}


/* Privat functions */

void
write_header (KiroTrbPrivate *priv)
{
    if (!priv)
        return;

    struct KiroTrbInfo *tmp_info = (struct KiroTrbInfo *)priv->mem;
    tmp_info->buffer_size_bytes = priv->buff_size;
    tmp_info->element_size = priv->element_size;
    tmp_info->offset = (priv->iteration * priv->max_elements) + (((char *)priv->current - priv->frame_top) / priv->element_size);
    memcpy (priv->mem, tmp_info, sizeof (struct KiroTrbInfo));
}



/* TRB functions */

uint64_t
kiro_trb_get_element_size (KiroTrb *self)
{
    g_return_val_if_fail (self != NULL, 0);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->initialized != 1)
        return 0;

    return priv->element_size;
}


uint64_t
kiro_trb_get_max_elements (KiroTrb *self)
{
    g_return_val_if_fail (self != NULL, 0);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->initialized != 1)
        return 0;

    return priv->max_elements;
}


uint64_t
kiro_trb_get_raw_size (KiroTrb *self)
{
    g_return_val_if_fail (self != NULL, 0);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->initialized != 1)
        return 0;

    return priv->buff_size;
}


void *
kiro_trb_get_raw_buffer (KiroTrb *self)
{
    g_return_val_if_fail (self != NULL, NULL);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->initialized != 1)
        return NULL;

    write_header (priv);
    return priv->mem;
}


void *
kiro_trb_get_element (KiroTrb *self, glong element_in)
{
    g_return_val_if_fail (self != NULL, NULL);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->initialized != 1)
        return NULL;

    gulong offset = element_in;
    if (0 <= element_in) {
        offset %= priv->max_elements;
        offset = priv->max_elements - offset;
    }
    else {
        offset *= -1;
        offset %= priv->max_elements;
    }

    gulong relative = ((char *)priv->current - priv->frame_top) + (offset * priv->element_size);
    relative %= (priv->buff_size - sizeof(struct KiroTrbInfo));

    return (char *)priv->frame_top + relative;
}


void
kiro_trb_flush (KiroTrb *self)
{
    g_return_if_fail (self != NULL);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);
    priv->iteration = 0;
    priv->current = priv->frame_top;
    write_header (priv);
}


void
kiro_trb_purge (KiroTrb *self, gboolean free_memory)
{
    g_return_if_fail (self != NULL);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);
    priv->iteration = 0;
    priv->current = NULL;
    priv->initialized = 0;
    priv->max_elements = 0;
    priv->buff_size = 0;
    priv->frame_top = NULL;
    priv->element_size = 0;

    if (free_memory)
        g_free (priv->mem);

    priv->mem = NULL;
}


int
kiro_trb_is_setup (KiroTrb *self)
{
    g_return_val_if_fail (self != NULL, 0);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);
    return priv->initialized;
}


int
kiro_trb_reshape (KiroTrb *self, uint64_t element_size, uint64_t element_count)
{
    g_return_val_if_fail (self != NULL, -1);
    if (element_size < 1 || element_count < 1)
        return -1;

    size_t new_size = (element_size * element_count) + sizeof (struct KiroTrbInfo);
    void *newmem = g_try_malloc0 (new_size);

    if (!newmem)
        return -1;

    ((struct KiroTrbInfo *)newmem)->buffer_size_bytes = new_size;
    ((struct KiroTrbInfo *)newmem)->element_size = element_size;
    ((struct KiroTrbInfo *)newmem)->offset = 0;
    kiro_trb_adopt (self, newmem);
    return 0;
}


int
kiro_trb_push (KiroTrb *self, void *element_in)
{
    g_return_val_if_fail (self != NULL, -1);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->initialized != 1)
        return -1;

    if (((char *)priv->current + priv->element_size) > ((char *)priv->mem + priv->buff_size))
        return -1;

    memcpy (priv->current, element_in, priv->element_size);
    (char *)priv->current += priv->element_size;

    if (priv->current >= (char *)priv->frame_top + (priv->element_size * priv->max_elements)) {
        priv->current = priv->frame_top;
        priv->iteration++;
    }

    write_header (priv);
    return 0;
}


void *
kiro_trb_dma_push (KiroTrb *self)
{
    g_return_val_if_fail (self != NULL, NULL);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->initialized != 1)
        return NULL;

    if (((char *)priv->current + priv->element_size) > ((char *)priv->mem + priv->buff_size))
        return NULL;

    void *mem_out = priv->current;
    (char *)priv->current += priv->element_size;

    if (priv->current >= (char *)priv->frame_top + (priv->element_size * priv->max_elements)) {
        priv->current = priv->frame_top;
        priv->iteration++;
    }

    write_header (priv);
    return mem_out;
}


void
kiro_trb_refresh (KiroTrb *self)
{
    g_return_if_fail (self != NULL);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->initialized != 1)
        return;

    struct KiroTrbInfo *tmp = (struct KiroTrbInfo *)priv->mem;
    priv->buff_size = tmp->buffer_size_bytes;
    priv->element_size = tmp->element_size;
    priv->max_elements = (tmp->buffer_size_bytes - sizeof (struct KiroTrbInfo)) / tmp->element_size;
    priv->iteration = tmp->offset / priv->max_elements;
    priv->frame_top = (char *)priv->mem + sizeof (struct KiroTrbInfo);
    priv->current = (char *)priv->frame_top + ((tmp->offset % priv->max_elements) * priv->element_size);
    priv->initialized = 1;
}


void
kiro_trb_adopt (KiroTrb *self, void *buff_in)
{
    g_return_if_fail (self != NULL);
    if (!buff_in)
        return;

    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);

    if (priv->mem)
        g_free (priv->mem);

    priv->mem = buff_in;
    priv->initialized = 1;
    kiro_trb_refresh (self);
}


int
kiro_trb_clone (KiroTrb *self, void *buff_in)
{
    g_return_val_if_fail (self != NULL, -1);
    KiroTrbPrivate *priv = KIRO_TRB_GET_PRIVATE (self);
    struct KiroTrbInfo *header = (struct KiroTrbInfo *)buff_in;
    void *newmem = g_try_malloc0 (header->buffer_size_bytes);

    if (!newmem)
        return -1;

    memcpy (newmem, buff_in, header->buffer_size_bytes);

    if (priv->mem)
        g_free (priv->mem);

    priv->mem = newmem;
    priv->initialized = 1;
    kiro_trb_refresh (self);
    return 0;
}

