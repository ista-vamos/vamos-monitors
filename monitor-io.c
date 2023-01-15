#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "shamon.h"
#include "stream-fds.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    puts("Started monitor-io");
    shm_event *ev = NULL;
    shamon *shamon = shamon_create(NULL, NULL);

    // attach to monitors of IO of given processes
    shm_stream_fds *fdsstream = NULL;
    for (int i = 1; i < argc; ++i) {
        printf("Opening file '%s' ...", argv[i]);
        fflush(stdout);
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) {
            perror("open failed");
            continue;
        }

        // if (fdsstream == NULL) {
        //     fdsstream = (shm_stream_fds *) shm_create_fds_stream();
        //     assert(fdsstream);
        // }
        fdsstream = (shm_stream_fds *)shm_create_fds_stream();
        shm_stream_fds_add_fd(fdsstream, fd);
        shamon_add_stream(shamon, (shm_stream *)fdsstream, 4096);
        puts(" OK");
    }

    if (fdsstream == NULL) {
        fprintf(stderr, "USAGE: prog file1 file2 ...\n");
        shamon_destroy(shamon);
        return -1;
    }

    // shamon_add_stream(shamon, (shm_stream *)fdsstream);

    while (1) {
        while ((ev = shamon_get_next_ev(shamon))) {
            puts("--------------------");
            shm_stream *stream = shm_event_stream(ev);
            printf("\033[0;34mEvent id %lu on stream '%s'\033[0m\n",
                   shm_event_id(ev), shm_stream_get_name(stream));
            shm_kind kind = shm_event_kind(ev);
            printf("Event kind %lu ('%s')\n", kind, shm_event_kind_name(kind));
            printf("Event size %lu\n", shm_event_size(ev));

            if (shm_event_is_hole(ev)) {
                printf("Dropped %lu events\n", ((shm_event_dropped *)ev)->n);
            } else {
                shm_event_fd_in *fd_ev = (shm_event_fd_in *)ev;
                printf("Event time %lu\n", fd_ev->time);
                assert(fd_ev->str_ref.size < INT_MAX);
                printf("Data: fd: %d, size: %lu:\n'%.*s'\n", fd_ev->fd,
                       fd_ev->str_ref.size, (int)fd_ev->str_ref.size,
                       fd_ev->str_ref.data);
            }
            puts("--------------------");
        }
        sleep_ms(100);
    }
    // TODO: make this a callback of shamon_destroy
    shm_destroy_fds_stream((shm_stream_fds *)fdsstream);
    shamon_destroy(shamon);
}
