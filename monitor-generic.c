#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "vamos-buffers/core/arbiter.h"
#include "vamos-buffers/core/event.h"
#include "monitors-utils.h"
#include "vamos-buffers/core/shamon.h"
#include "vamos-buffers/core/signatures.h"
#include "vamos-buffers/core/source.h"
#include "vamos-buffers/streams/stream-generic.h"
#include "vamos-buffers/core/stream.h"
#include "vamos-buffers/core/utils.h"

//#define CHECK_IDS
#define CHECK_IDS_ABORT

static inline void dump_args(vms_stream *stream, vms_event_generic *ev,
                             const char *signature) {
    unsigned char *p = ev->args;
    for (const char *o = signature; *o; ++o) {
        if (o != signature)
            printf(", ");
        if (*o == 'S' || *o == 'L' || *o == 'M') {
            const char *str = vms_stream_get_str(stream, (*(uint64_t *)p));
            const size_t len = strlen(str);
            printf("S[%lu, %lu]('%.*s%s)", (*(uint64_t *)p) >> 32,
                   (*(uint64_t *)p) & 0xffffffff, len > 6 ? 6 : (int)len, str,
                   len > 6 ? "...'" : "'");
            p += sizeof(uint64_t);
            continue;
        }

        size_t size = signature_op_get_size(*o);
        if (*o == 'f') {
            printf("%f", *((float *)p));
        } else if (*o == 'd') {
            printf("%lf", *((double *)p));
        } else if (*o == 't') {
            printf("\033[31m%lu\033[0m", *((uint64_t *)p));
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

shamon *shmn;

sig_atomic_t __run = 1;

static void sig_fatal(int sig) {
    (void)sig;
    fprintf(stderr, "Caught signal %d\n", sig);
    if (shmn) {
        shamon_detach(shmn);
    }
    __run = 0;
}

static void setup_signals() {
    if (signal(SIGINT, sig_fatal) == SIG_ERR) {
        perror("failed setting SIGINT handler");
    }

    if (signal(SIGABRT, sig_fatal) == SIG_ERR) {
        perror("failed setting SIGINT handler");
    }

    if (signal(SIGIOT, sig_fatal) == SIG_ERR) {
        perror("failed setting SIGINT handler");
    }

    if (signal(SIGSEGV, sig_fatal) == SIG_ERR) {
        perror("failed setting SIGINT handler");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "USAGE: prog name:source:shmkey [name:source:shmkey ...]\n");
        return -1;
    }

    vms_event *ev = NULL;
    shmn = shamon_create(NULL, NULL);
    assert(shmn);

    setup_signals();

#ifdef DUMP_STATS
    size_t total_events_num =
        vms_get_last_special_kind() + 1; /* ids up to 'dropped' are reserved */
#endif
    for (int i = 1; i < argc; ++i) {
        fprintf(stderr, "Connecting stream '%s' ...\n", argv[i]);
        vms_stream *stream = create_stream(argc, argv, i, NULL, NULL);
        assert(stream && "Creating stream failed");
        assert(vms_stream_id(stream) == (size_t)i);
        shamon_add_stream(shmn, stream,
                          /* buffer capacity = */ 4 * 4096);
        vms_stream_register_all_events(stream);
        vms_stream_dump_events(stream);
#ifdef DUMP_STATS
        size_t events_num;
        vms_stream_get_avail_events(stream, &events_num);
        total_events_num += events_num;
#endif
    }

#ifdef DUMP_STATS
    /* count dropped per a stream and different kinds of events */
    uint64_t dropped_count[argc];
    uint64_t dropped_sum_count[argc];
    uint64_t kinds_count[total_events_num][argc];
    memset(kinds_count, 0, total_events_num * argc * sizeof(uint64_t));
    memset(dropped_count, 0, argc * sizeof(uint64_t));
    memset(dropped_sum_count, 0, argc * sizeof(uint64_t));
#endif

    vms_kind kind;
    size_t n = 0, drp = 0, drpn = 0;
    size_t id;
    vms_stream *stream;

#ifdef CHECK_IDS
    size_t stream_id;
    size_t next_id[argc];
    for (int i = 1; i < argc; ++i) next_id[i] = 1;
#endif
    size_t spinned = 0;
    size_t old_n;
    struct event_record *rec;
    struct event_record unknown_rec = {
        .size = 0, .name = "unknown", .signature = "", .kind = 0};

    while (__run && shamon_is_ready(shmn)) {
        old_n = n;

        while ((ev = shamon_get_next_ev(shmn, &stream))) {
            ++n;

            int ind = 5 * vms_stream_id(stream);
            id = vms_event_id(ev);
            kind = vms_event_kind(ev);

            if (vms_event_is_hole(ev)) {
                printf("%*s%s: ", ind, "", vms_stream_get_name(stream));
                printf(
                    "\033[0;31mhole\033[0m(\033[0;34mid: %lu, kind: "
                    "%lu\033[0m, ...)\n",
                    id, kind);
                ++drp;
                continue;
            }

#ifdef CHECK_IDS
            stream_id = vms_stream_id(stream);
#endif
#ifdef DUMP_STATS
            assert(kind < total_events_num && "OOB access");
#ifdef CHECK_IDS
            assert(stream_id < (size_t)argc && "OOB access");
            ++kinds_count[kind][stream_id];
#endif
#endif

            rec = vms_stream_get_event_record(stream, kind);
            rec = rec ? rec : &unknown_rec;
            printf("%*s%s: ", ind, "", vms_stream_get_name(stream));
            printf("\033[0;35m%s\033[0m(\033[0;34mid: %lu, kind: %lu\033[0m",
                   rec->name, id, kind);

#ifdef CHECK_IDS
            if (id != next_id[stream_id]) {
                printf("%*s\033[0;31mWrong ID: %lu, should be %lu\033[0m\n",
                       ind, "", id, next_id[stream_id]);
#ifdef CHECK_IDS_ABORT
                abort();
#endif
                next_id[stream_id] = id;
            }
#endif

            if (vms_event_is_hole(ev)) {
                printf("%*shole\n", ind, "");
                /*
                printf("%*s'dropped(%lu)'\n", ind, "", ((vms_event_dropped
                *)ev)->n); drpn += ((vms_event_dropped *)ev)->n;
                */
#ifdef DUMP_STATS
#ifdef CHECK_IDS
                assert(stream_id < (size_t)argc && "OOB access");
                dropped_sum_count[stream_id] += ((vms_event_dropped *)ev)->n;
                ++dropped_count[stream_id];
#endif
#endif
#ifdef CHECK_IDS
#error "Not implemented"
                /*
                next_id[stream_id] += ((vms_event_dropped *)ev)->n;
                */
#endif
                ++drp;
                continue;
            }

#ifdef CHECK_IDS
            ++next_id[stream_id];
#endif
            if (*rec->signature) {
                printf(", ");
                dump_args(stream, (vms_event_generic *)ev,
                          (const char *)rec->signature);
            }
            printf(")\n");
        }

        if (n == old_n) {
            ++spinned;
        } else {
            spinned = 0;
        }

        if (spinned > 1000) {
            sleep_ns(100);
            if (spinned > 2000) {
                sleep_ms(10);
                if (spinned > 2050) {
                    sleep_ms(40);
                }
            }
        }
    }
    fflush(stdout);
    fflush(stderr);
    printf(
        "Processed %lu events, %lu dropped events (sum of args: %lu)... "
        "totally came: %lu\n",
        n, drp, drpn, n + drpn - drp);

#ifdef DUMP_STATS
    size_t streams_num;
#ifdef CHECK_IDS
    size_t totally_came = 0;
    size_t evs_num;
    vms_stream **streams =
#endif
        shamon_get_streams(shmn, &streams_num);
    vms_vector *buffers = shamon_get_buffers(shmn);
    for (size_t i = 0; i < streams_num; ++i) {
#ifdef CHECK_IDS
        vms_stream *stream = streams[i];
        stream_id = vms_stream_id(stream);
        assert(stream_id < (size_t)argc && "OOB access");

        printf("-- Stream '%s':\n", vms_stream_get_name(stream));
        printf("  Event 'dropped': %lu (sum of arguments: %lu)\n",
               dropped_count[stream_id], dropped_sum_count[stream_id]);
        totally_came += dropped_count[stream_id];

        struct event_record *evs =
            vms_stream_get_avail_events(stream, &evs_num);
        for (size_t n = 0; n < evs_num; ++n) {
            kind = evs[n].kind;
            assert(kind < total_events_num && "OOB access");
            assert(strcmp(vms_event_kind_name(kind), evs[n].name) == 0 &&
                   "Inconsistent names");
            printf("  Event '%s': %lu\n", evs[n].name,
                   kinds_count[kind][stream_id]);
            totally_came += kinds_count[kind][stream_id];
        }
#endif

        vms_arbiter_buffer *buff =
            (vms_arbiter_buffer *)vms_vector_at(buffers, i);
        vms_arbiter_buffer_dump_stats(buff);
    }
#ifdef CHECK_IDS
    assert(totally_came == n);
#endif
#endif

    shamon_destroy(shmn);
}
