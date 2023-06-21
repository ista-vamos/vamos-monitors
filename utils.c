#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "monitors-utils.h"
#include "vamos-buffers/core/stream.h"
#include "vamos-buffers/streams/streams.h"

vms_stream *create_stream(int argc, char *argv[], int arg_i,
                          const char *expected_stream_name,
                          const vms_stream_hole_handling *hole_handling) {
    assert(arg_i < argc && "Index too large");

    char streamname[256];
    const char *dc = strchr(argv[arg_i], ':');
    if (!dc) {
        fprintf(stderr, "Failed to parse the name of stream.");
        return NULL;
    }
    strncpy(streamname, argv[arg_i], dc - argv[arg_i]);
    streamname[dc - argv[arg_i]] = 0;

    vms_stream *stream =
        vms_stream_create_from_argv(streamname, argc, argv, hole_handling);
    assert(stream);
    if (expected_stream_name &&
        strcmp(vms_stream_get_name(stream), expected_stream_name) != 0) {
        fprintf(stderr,
                "A wrong source was specified for this monitor.\n"
                "Got '%s' but expected '%s'",
                vms_stream_get_name(stream), expected_stream_name);
        vms_stream_destroy(stream);
        return NULL;
    }

    return stream;
}
