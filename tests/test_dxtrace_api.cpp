// [DXAIT-COMPONENT: dxtrace]
// [DXAIT-SUBSYSTEM: correlated trace test]
// [DXAIT-TEST: spans, request IDs, export, and lifecycle]

#include "dxait/dx_trace_api.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>

int main() {
    const char* output_path = "dxtrace_test.json";
    dx_trace_session_t* session = nullptr;
    assert(dx_trace_session_create("portable-test", &session) == 0);
    assert(session != nullptr);

    dx_trace_span_t* span = nullptr;
    assert(dx_trace_begin(session, "test", "operation", 42u, &span) == 0);
    assert(span != nullptr);
    std::this_thread::yield();
    assert(dx_trace_end(span) == 0);
    assert(dx_trace_end(span) != 0);
    dx_trace_span_destroy(span);
    assert(dx_trace_event_count(session) == 1u);

    assert(dx_trace_begin(session, "test", "unfinished", 43u, &span) == 0);
    assert(dx_trace_event_count(session) == 2u);
    dx_trace_span_destroy(span);

    assert(dx_trace_export(session, output_path) == 0);
    std::ifstream input(output_path);
    const std::string json((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
    assert(json.find("operation") != std::string::npos);
    assert(json.find("request_id") != std::string::npos);
    assert(json.find("unfinished") == std::string::npos);
    input.close();
    std::remove(output_path);

    assert(dx_trace_begin(nullptr, "test", "invalid", 0u, &span) != 0);
    assert(dx_trace_export(session, nullptr) != 0);
    dx_trace_session_destroy(session);
    return 0;
}
