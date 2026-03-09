#pragma once

#include <stdint.h>
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Emit a structured timing marker for offline scheduling analysis.
 *
 * Log format:
 * `ts_us=<microseconds> task=<task_name> evt=<event_name> v=<value>`
 */
void sc_trace_mark(const char *task_name, const char *event_name, int32_t value);

#if CONFIG_SC_TRACE_TIMING
#define SC_TRACE_MARK(task_name, event_name, value) \
    sc_trace_mark((task_name), (event_name), (int32_t)(value))
#else
#define SC_TRACE_MARK(task_name, event_name, value) do { (void)(task_name); (void)(event_name); (void)(value); } while (0)
#endif

#ifdef __cplusplus
}
#endif
