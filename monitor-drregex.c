#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "vamos-buffers/core/event.h"
#include "monitors-utils.h"
#include "vamos-buffers/core/shamon.h"
#include "vamos-buffers/core/signatures.h"
#include "vamos-buffers/core/source.h"
#include "vamos-buffers/streams/stream-drregex.h"

static inline void dump_args(vms_stream *stream, vms_event_drregex *ev,
                             const char *signature) {
    unsigned char *p = ev->args;
    for (const char *o = signature; *o; ++o) {
        printf(", ");
        if (*o == 'S' || *o == 'L' || *o == 'M') {
            printf("S[%lu, %lu](%s)", (*(uint64_t *)p) >> 32,
                   (*(uint64_t *)p) & 0xffffffff,
                   vms_stream_get_str(stream, (*(uint64_t *)p)));
            p += sizeof(uint64_t);
            continue;
        }

        size_t size = signature_op_get_size(*o);
        if (*o == 'f') {
            printf("%f", *((float *)p));
        } else if (*o == 'd') {
            printf("%lf", *((double *)p));
        } else {
            switch (size) {
                case 1:
                    printf("%c", *((char *)p));
                    break;
                case 4:
                    printf("%d", *((int *)p));
                    break;
                case 8:
                    printf("%ld", *((long int *)p));
                    break;
                default:
                    printf("?");
            }
        }
        p += size;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: prog shm1\n");
        return -1;
    }

    vms_event *ev = NULL;
    shamon *shmn = shamon_create(NULL, NULL);
    assert(shmn);

    vms_stream *fstream = create_stream(argc, argv, 1, "drregex-stream", NULL);
    assert(fstream && "Creating stream failed");
    shamon_add_stream(shmn, fstream,
                      /* buffer capacity = */ 4 * 4096);

    vms_stream_register_all_events(fstream);
    vms_stream_dump_events(fstream);

    // vms_kind kind;
    // size_t id, next_id = 1;
    size_t n = 0, drp = 0;  //, drpn = 0;
    vms_stream *stream;
    struct vms_event_record *rec;
    struct vms_event_record unknown_rec = {
        .size = 0, .name = "unknown", .signature = "", .kind = 0};

    while (shamon_is_ready(shmn)) {
        while ((ev = shamon_get_next_ev(shmn, &stream))) {
            ++n;

            /*
            id = vms_event_id(ev);
            if (id != next_id) {
                printf("Wrong ID: %lu, should be %lu\n", id, next_id);
                next_id = id; // reset
            }
            */

            // kind = vms_event_kind(ev);
            if (vms_event_is_hole(ev)) {
                printf("Event 'HOLE'\n");
                /*
                printf("Event 'dropped(%lu)'\n", ((vms_event_dropped *)ev)->n);
                drpn += ((vms_event_dropped *)ev)->n;
                next_id += ((vms_event_dropped *)ev)->n;
                */
                ++drp;
                continue;
            }

            // ++next_id;

            vms_kind kind = vms_event_kind(ev);
            rec = vms_stream_get_event_record(stream, kind);
            rec = rec ? rec : &unknown_rec;
            printf("Event kind %lu ('%s')\n", kind, rec->name);
            puts("--------------------");
            printf("\033[0;34mEvent id %lu\033[0m\n", vms_event_id(ev));
            printf("Event kind %lu ('%s')\n", kind, rec->name);
            printf("Event size %lu\n", rec->size);
            vms_event_drregex *reev = (vms_event_drregex *)ev;
#ifndef DRREGEX_ONLY_ARGS
            printf("{%s, thrd: %lu, fd: %d", reev->write ? "write" : "read",
                   reev->thread, reev->fd);
#else
            printf("{");
#endif
            dump_args(stream, reev, (const char *)rec->signature);
            printf("}\n");
            /*
             */
        }
    }
    /*
    printf("Processed %lu events, %lu dropped events (sum of args: %lu)... "
           "totally came: %lu\n",
           n, drp, drpn, n + drpn - drp);
           */
    printf("Processed %lu events, %lu hole events\n", n, drp);

    shamon_destroy(shmn);
}
