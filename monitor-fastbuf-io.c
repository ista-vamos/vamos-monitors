#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "event.h"
#include "shamon.h"
#include "stream-fastbuf-io.h"
#include "utils.h"

/*
bool is_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}
*/

int main(int argc, char *argv[]) {
    shm_event *ev = NULL;
    shamon *shmn = shamon_create(NULL, NULL);

    if (argc == 1) {
        fprintf(stderr, "USAGE: prog file1 file2 ...\n");
        return -1;
    }

    // attach to monitors of IO of given processes
    for (int i = 1; i < argc; ++i) {
        pid_t pid = atoi(argv[i]);
        printf("Attaching stream of fastbuf of %d\n", pid);
        shm_stream *stream = shm_create_io_stream(pid);
        shamon_add_stream(shmn, stream);
        assert(stream);
        puts(" OK");
    }

    while (1) {
        while ((ev = shamon_get_next_ev(shmn))) {
            puts("--------------------");
            shm_stream *stream = shm_event_stream(ev);
            printf("\033[0;34mEvent id %lu on stream '%s'\033[0m\n",
                   shm_event_id(ev), shm_stream_get_name(stream));
            shm_kind kind = shm_event_kind(ev);
            printf("Event kind %lu ('%s')\n", kind, shm_event_kind_name(kind));
            printf("Event size %lu\n", shm_event_size(ev));
            shm_event_io *shm_ev = (shm_event_io *)ev;
            assert(shm_ev->str_ref.size < INT_MAX);
            printf("Event time %lu\n", shm_ev->time);
            printf("Data: fd: %d, size: %lu:\n'%.*s'\n", shm_ev->fd,
                   shm_ev->str_ref.size, (int)shm_ev->str_ref.size,
                   shm_ev->str_ref.data);
            puts("--------------------");
        }
        sleep_ms(100);
    }
    shamon_destroy(shmn);
}
