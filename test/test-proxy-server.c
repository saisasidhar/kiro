/* Copyright (C) 2014 Max Riechelmann <max.riechelmann@googlemail.com>
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
 * SECTION: test-proxy-server
 * @short_description: KIRO GPUDIRECT test proxy server
 * @title: GPUDIRECTproxyserver
 * @filename: test-proxy-server.c
 *
 * Simulates a detector, that has new data ready in random intervals (e.g. ANKA
 * camera).
 * 
 **/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "kiro-server.h"
#include "kiro-trb.h"
#include <gmodule.h>
#include <gio/gio.h>
#include <string.h>
#include <math.h>
#include <time.h>



/**
 * 
 **/
int 
main (void)
{
    // Alocate memory
    void *memory; // Pointer to memory
    unsigned long int *frame; // Pointer to framenumber
    void *data; // Pointer to data
    int data_size = 1024; // Size of data set. Must be multiple of int.
    size_t memory_size = sizeof (frame) + data_size;

    // Allocate memory and point pointers to it.
    memory = malloc (memory_size);
    frame = memory;
    data = memory + sizeof (frame);
    

    // Create server that holds data.
    KiroServer *kiroServer = kiro_server_new ();
    if (0 > kiro_server_start (kiroServer, NULL, "60010", memory, memory_size)) {
        g_critical ("Failed to start server properly");
        goto done;
    }

    // Create random number generator seed.
    srand (time (NULL));

    while (1) {
        // Change data.
        int i;
        int *offset;
        for (i = 0; i < (int) (data_size / sizeof (int)); i++) {
            offset = data + i;
            *offset = rand();
        }
        // Sleep to fake realtime camera.
        sleep (rand() % 10 / 2);

        // Increment frame count.
        *frame += 1;

        g_warning ("Frame: %ld, Data: %d", *frame, *(int *)data);
    }
done:
    kiro_server_free (kiroServer);
    return 0;
}