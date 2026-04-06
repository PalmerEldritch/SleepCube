#include "sc_trace.h"

#include <inttypes.h>
#include "esp_log.h"
#include "esp_timer.h"

#if CONFIG_SC_TRACE_TIMING
static const char *TAG = "sc_trace";
#endif

void sc_trace_mark(const char *task_name, const char *event_name, int32_t value)
{
#if CONFIG_SC_TRACE_TIMING
    if ((task_name == NULL) || (event_name == NULL)) {
        return;
    }
    const uint64_t ts_us = (uint64_t)esp_timer_get_time();
    ESP_LOGI(TAG, "ts_us=%" PRIu64 " task=%s evt=%s v=%" PRId32, ts_us, task_name, event_name, value);
#else
    (void)task_name;
    (void)event_name;
    (void)value;
#endif
}
