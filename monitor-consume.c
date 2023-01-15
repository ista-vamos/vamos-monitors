#include <assert.h>
#include <immintrin.h> /* _mm_pause */
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#include "vamos-buffers/core/arbiter.h"
#include "vamos-buffers/core/event.h"
#include "monitors-utils.h"
#include "vamos-buffers/core/signatures.h"
#include "vamos-buffers/core/stream.h"

#define SLEEP_NS_INIT (50)
#define SLEEP_THRESHOLD_NS (10000000)

static int buffer_manager_thrd(void *data) {
    shm_arbiter_buffer *buffer = (shm_arbiter_buffer *)data;
    shm_stream *stream = shm_arbiter_buffer_stream(buffer);

    // wait for buffer->active
    while (!shm_arbiter_buffer_active(buffer)) _mm_pause();

    printf("Running fetch & autodrop for stream %s\n",
           shm_stream_get_name(stream));

    const size_t ev_size = shm_stream_event_size(stream);
    void *ev, *out;
    while (1) {
        ev = stream_fetch(stream, buffer);
        if (!ev) {
            break;
        }

        out = shm_arbiter_buffer_write_ptr(buffer);
        assert(out && "No space in the buffer");
        memcpy(out, ev, ev_size);
        shm_arbiter_buffer_write_finish(buffer);
        shm_stream_consume(stream, 1);
    }

    // TODO: we should check if the stream is finished and remove it
    // in that case
    printf("BMM for stream %lu (%s) exits\n", shm_stream_id(stream),
           shm_stream_get_name(stream));
    thrd_exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: prog source-spec\n");
        return -1;
    }

    shm_stream *stream = create_stream(argc, argv, 1, NULL, NULL);
    assert(stream && "Creating stream failed");

    shm_stream_register_all_events(stream);
    shm_stream_dump_events(stream);

    shm_arbiter_buffer *buffer =
        shm_arbiter_buffer_create(stream, shm_stream_event_size(stream),
                                  /*capacity=*/4 * 4096);
    thrd_t tid;
    thrd_create(&tid, buffer_manager_thrd, (void *)buffer);
    shm_arbiter_buffer_set_active(buffer, true);

    size_t n = 0, tmp, trials = 0;
    while (1) {
        tmp = shm_arbiter_buffer_size(buffer);
        if (tmp > 0) {
            n += shm_arbiter_buffer_drop(buffer, tmp);
            trials = 0;
        } else {
            ++trials;
        }

        if (trials > 1000) {
            if (!shm_stream_is_ready(stream))
                break;
        }
    }
    printf("Processed %lu events\n", n);

    thrd_join(tid, NULL);

    shm_stream_destroy(stream);
    shm_arbiter_buffer_free(buffer);

    return 0;
}
