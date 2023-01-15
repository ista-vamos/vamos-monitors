#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "vamos-buffers/core/event.h"
#include "vamos-buffers/core/shamon.h"
#include "vamos-buffers/core/signatures.h"
#include "vamos-buffers/core/source.h"
#include "vamos-buffers/streams/stream-regex.h"
#include "monitors-utils.h"

static inline void dump_args(shm_stream *stream, shm_event_regex *ev,
                             const char *signature) {
    unsigned char *p = ev->args;
    for (const char *o = signature; *o; ++o) {
        if (o != signature)
            printf(", ");
        if (*o == 'S' || *o == 'L' || *o == 'M') {
            printf("S[%lu, %lu](%s)", (*(uint64_t *)p) >> 32,
                   (*(uint64_t *)p) & 0xffffffff,
                   shm_stream_get_str(stream, (*(uint64_t *)p)));
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

    shm_event *ev = NULL;
    shamon *shmn = shamon_create(NULL, NULL);
    assert(shmn);

    shm_stream *fstream = create_stream(argc, argv, 1, "regex-stream", NULL);
    assert(fstream && "Creating stream failed");
    shamon_add_stream(shmn, fstream,
                      /* buffer capacity = */ 4 * 4096);

    shm_stream_register_all_events(fstream);
    shm_stream_dump_events(fstream);

    // shm_kind kind;
    // size_t id, next_id = 1;
    size_t n = 0, drp = 0;  //, drpn = 0;
    shm_stream *stream;
    struct event_record *rec;
    while (shamon_is_ready(shmn)) {
        while ((ev = shamon_get_next_ev(shmn, &stream))) {
            ++n;

            /*
            id = shm_event_id(ev);
            if (id != next_id) {
                printf("Wrong ID: %lu, should be %lu\n", id, next_id);
                next_id = id; // reset
            }
            */

            // kind = shm_event_kind(ev);
            if (shm_event_is_hole(ev)) {
                printf("Event 'HOLE'\n");
                /* printf("Event 'dropped(%lu)'\n",
                ((shm_event_dropped*)ev)->n);
                drpn += ((shm_event_dropped *)ev)->n;
                next_id += ((shm_event_dropped *)ev)->n;
                */
                ++drp;
                continue;
            }

            /*
            ++next_id;
            if (last_id > 0) {
                if (last_id + 1 != id) {
                    fprintf(stderr, "Inconsistent IDs, %lu + 1 != %lu\n",
                            last_id, id);
                    abort();
                }
            }
            last_id = id;
            */
            shm_kind kind = shm_event_kind(ev);
            rec = shm_stream_get_event_record(stream, kind);
            printf("Event kind %lu ('%s')\n", kind, rec->name);
            puts("--------------------");
            printf("\033[0;34mEvent id %lu\033[0m\n", shm_event_id(ev));
            printf("Event kind %lu ('%s')\n", kind, rec->name);
            printf("Event size %lu\n", rec->size);
            shm_event_regex *reev = (shm_event_regex *)ev;
            printf("{");
            dump_args(stream, reev, (const char *)rec->signature);
            printf("}\n");
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
