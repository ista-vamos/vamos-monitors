#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "shamon.h"
#include "stream-fds.h"
#include "utils.h"

size_t n = 0;
size_t bytes[8] = {0};

static void update_bytes(unsigned char b) {
    for (int i = 0; i < 8; ++i) {
        if (b & (1 << i)) {
            ++bytes[i];
        }
    }
    ++n;
}

static void print_bytes_rat() {
    for (int i = 0; i < 8; ++i) {
        printf(" %5.2f", (double)(bytes[i]) / n);
    }
    putchar('\n');
}

int main(int argc, char *argv[]) {
    puts("Started monitor-io");
    shm_event *ev = NULL;
    shamon *shamon = shamon_create(NULL, NULL);

    shm_stream_fds *fdsstream = NULL;
    int fd = open("/dev/random", O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return 1;
    }
    fdsstream = (shm_stream_fds *)shm_create_fds_stream();
    shm_stream_fds_add_fd(fdsstream, fd);
    shamon_add_stream(shamon, (shm_stream *)fdsstream);
    puts(" OK");

    if (fdsstream == NULL) {
        fprintf(stderr, "USAGE: prog file1 file2 ...\n");
        shamon_destroy(shamon);
        return -1;
    }

    size_t m = 0;
    while (shamon_is_ready(shamon)) {
        while ((ev = shamon_get_next_ev(shamon))) {
            if (shm_event_is_hole(ev)) {
                // printf("Dropped %lu events\n", ((shm_event_dropped*)ev)->n);
                continue;
            }
            if (++m > 1000) {
                m = 0;
                print_bytes_rat();
            }
            shm_event_fd_in *fd_ev = (shm_event_fd_in *)ev;
            for (int i = 0; i < fd_ev->str_ref.size; ++i) {
                // if (isdigit(fd_ev->str_ref.data[i]))
                update_bytes(fd_ev->str_ref.data[i]);
            }
        }
    }

    print_bytes_rat();

    shamon_destroy(shamon);
}
