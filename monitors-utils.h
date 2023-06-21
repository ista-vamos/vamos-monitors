#ifndef VAMOS_MONITORS_UTILS_H_
#define VAMOS_MONITORS_UTILS_H_

#include "vamos-buffers/core/stream.h"

vms_stream *create_stream(int argc, char *argv[], int arg_i,
                          const char *expected_stream_name,
                          const vms_stream_hole_handling *hole_handling);

#endif
