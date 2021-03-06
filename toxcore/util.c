/*
 * util.c -- Utilities.
 *
 * This file is donated to the Tox Project.
 * Copyright 2013  plutooo
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <time.h>

/* for CLIENT_ID_SIZE */
#include "DHT.h"

#include "util.h"

uint64_t now()
{
    return time(NULL);
}

uint64_t random_64b()
{
    uint64_t r;

    // This is probably not random enough?
    r = random_int();
    r <<= 32;
    r |= random_int();

    return r;
}

bool id_eq(uint8_t *dest, uint8_t *src)
{
    return memcmp(dest, src, CLIENT_ID_SIZE) == 0;
}

void id_cpy(uint8_t *dest, uint8_t *src)
{
    memcpy(dest, src, CLIENT_ID_SIZE);
}

int load_state(load_state_callback_func load_state_callback, void *outer,
               uint8_t *data, uint32_t length, uint16_t cookie_inner)
{
    if (!load_state_callback || !data) {
#ifdef DEBUG
        fprintf(stderr, "load_state() called with invalid args.\n");
#endif
        return -1;
    }


    uint16_t type;
    uint32_t length_sub, cookie_type;
    uint32_t size32 = sizeof(uint32_t), size_head = size32 * 2;

    while (length >= size_head) {
        length_sub = *(uint32_t *)data;
        cookie_type = *(uint32_t *)(data + size32);
        data += size_head;
        length -= size_head;

        if (length < length_sub) {
            /* file truncated */
#ifdef DEBUG
            fprintf(stderr, "state file too short: %u < %u\n", length, length_sub);
#endif
            return -1;
        }

        if ((cookie_type >> 16) != cookie_inner) {
            /* something is not matching up in a bad way, give up */
#ifdef DEBUG
            fprintf(stderr, "state file garbeled: %04hx != %04hx\n", (cookie_type >> 16), cookie_inner);
#endif
            return -1;
        }

        type = cookie_type & 0xFFFF;

        if (-1 == load_state_callback(outer, data, length_sub, type))
            return -1;

        data += length_sub;
        length -= length_sub;
    }

    return length == 0 ? 0 : -1;
};

#ifdef LOGGING
time_t starttime = 0;
size_t logbufferprelen = 0;
char *logbufferpredata = NULL;
char *logbufferprehead = NULL;
char logbuffer[512];
static FILE *logfile = NULL;
void loginit(uint16_t port)
{
    if (logfile)
        fclose(logfile);

    if (!starttime)
        starttime = now();

    struct tm *tm = localtime(&starttime);

    if (strftime(logbuffer + 32, sizeof(logbuffer) - 32, "%F %T", tm))
        sprintf(logbuffer, "%u-%s.log", ntohs(port), logbuffer + 32);
    else
        sprintf(logbuffer, "%u-%lu.log", ntohs(port), starttime);

    logfile = fopen(logbuffer, "w");

    if (logbufferpredata) {
        if (logfile)
            fprintf(logfile, "%s", logbufferpredata);

        free(logbufferpredata);
        logbufferpredata = NULL;
    }

};
void loglog(char *text)
{
    if (logfile) {
        fprintf(logfile, "%4u %s", (uint32_t)(now() - starttime), text);
        fflush(logfile);

        return;
    }

    /* log messages before file was opened: store */

    size_t len = strlen(text);

    if (!starttime) {
        starttime = now();
        logbufferprelen = 1024 + len - (len % 1024);
        logbufferpredata = malloc(logbufferprelen);
        logbufferprehead = logbufferpredata;
    }

    /* loginit() called meanwhile? (but failed to open) */
    if (!logbufferpredata)
        return;

    if (len + logbufferprehead - logbufferpredata + 16U < logbufferprelen) {
        size_t logpos = logbufferprehead - logbufferpredata;
        size_t lennew = logbufferprelen * 1.4;
        logbufferpredata = realloc(logbufferpredata, lennew);
        logbufferprehead = logbufferpredata + logpos;
        logbufferprelen = lennew;
    }

    int written = sprintf(logbufferprehead, "%4u %s", (uint32_t)(now() - starttime), text);
    logbufferprehead += written;
}

void logexit()
{
    if (logfile) {
        fclose(logfile);
        logfile = NULL;
    }
};
#endif

