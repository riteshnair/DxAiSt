// [DXAIT-COMPONENT: dxstream]
// [DXAIT-SUBSYSTEM: stream/event test]
// [DXAIT-TEST: validation, sequencing, completion, timeout]

#include "dxait/dx_stream_api.h"

#include <cassert>
#include <cstdint>

int main() {
    dx_stream_t* stream = nullptr;
    assert(dx_stream_create(DX_STREAM_HOST, &stream) == 0);
    assert(stream != nullptr);

    dx_event_t* first = nullptr;
    dx_event_t* second = nullptr;
    assert(dx_stream_record_event(stream, &first) == 0);
    assert(dx_stream_record_event(stream, &second) == 0);
    assert(first != nullptr && second != nullptr);
    assert(dx_event_sequence(first) > 0u);
    assert(dx_event_sequence(second) == dx_event_sequence(first) + 1u);
    assert(dx_event_is_complete(first) == 1);
    assert(dx_event_wait(first, 0u) == 0);
    assert(dx_event_wait(second, UINT32_MAX) == 0);

    assert(dx_stream_record_event(nullptr, &first) != 0);
    assert(dx_stream_record_event(stream, nullptr) != 0);
    assert(dx_event_wait(nullptr, 0u) != 0);
    assert(dx_event_is_complete(nullptr) == 0);
    assert(dx_event_sequence(nullptr) == 0u);

    dx_event_destroy(first);
    dx_event_destroy(second);
    dx_stream_destroy(stream);

    assert(dx_stream_create(99u, &stream) != 0);
    return 0;
}
