#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "shamon.h"
#include "stream-stdin.h"

int main(void) {
    shm_event *ev = NULL;
    shamon *shmn = shamon_create(NULL, NULL);
    // custom stream
    shm_stream *stdin_stream = shm_create_stdin_stream();
    shamon_add_stream(shmn, stdin_stream, 3);

    puts("Created STDIN stream");

    while (1) {
        while ((ev = shamon_get_next_ev(shmn))) {
            printf("Event id %lu\n", shm_event_id(ev));
            shm_kind kind = shm_event_kind(ev);
            printf("Event kind %lu ('%s')\n", kind, shm_event_kind_name(kind));
            printf("Event size %lu\n", shm_event_size(ev));
            shm_event_stdin *stdin_ev = (shm_event_stdin *)ev;
            printf("Event time %lu\n", stdin_ev->time);
            printf("Data: fd: %d, size: %lu:'%s'\n", stdin_ev->fd,
                   stdin_ev->str_ref.size, stdin_ev->str_ref.data);
        }
    }

    shamon_destroy(shmn);
}
