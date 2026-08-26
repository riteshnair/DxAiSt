// [DXAIT-COMPONENT: dxtrace]
// [DXAIT-SUBSYSTEM: correlated trace ABI]
// [DXAIT-IMPLEMENTATION: host span capture and Chrome Trace export]

#include "dxait/dx_trace_api.h"
#include "dxait/dx_core_api.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <new>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

struct dx_trace_event_t {
    std::string category;
    std::string name;
    uint64_t request_id{0u};
    uint64_t thread_id{0u};
    uint64_t begin_us{0u};
    uint64_t duration_us{0u};
    bool complete{false};
};

struct dx_trace_session_t {
    dx_component_logger_t* logger{nullptr};
    std::string name;
    std::chrono::steady_clock::time_point origin;
    mutable std::mutex mutex;
    std::vector<std::unique_ptr<dx_trace_event_t>> events;

    ~dx_trace_session_t() {
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }
};

struct dx_trace_span_t {
    dx_trace_session_t* session{nullptr};
    dx_trace_event_t* event{nullptr};
    bool ended{false};
};

namespace {

bool valid_text(const char* text) noexcept {
    return text != nullptr && text[0] != '\0';
}

uint64_t thread_id() noexcept {
#ifdef _WIN32
    return static_cast<uint64_t>(GetCurrentThreadId());
#else
    return static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
}

std::string json_escape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8u);
    for (const char ch : value) {
        if (ch == '\\') escaped += "\\\\";
        else if (ch == '"') escaped += "\\\"";
        else if (ch == '\n') escaped += "\\n";
        else if (ch == '\r') escaped += "\\r";
        else escaped += ch;
    }
    return escaped;
}

uint64_t elapsed_us(const dx_trace_session_t& session) noexcept {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - session.origin).count());
}

} // namespace

extern "C" {

DXAIT_TRACE_API int32_t DXAIT_TRACE_CALL dx_trace_session_create(
    const char* name,
    dx_trace_session_t** out_session) {
    if (!valid_text(name) || out_session == nullptr) {
        return -1;
    }
    *out_session = nullptr;
    try {
        auto session = std::make_unique<dx_trace_session_t>();
        session->name = name;
        session->origin = std::chrono::steady_clock::now();
        if (dx_component_logger_create("dxtrace", &session->logger) != 0) {
            return -2;
        }
        *out_session = session.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    } catch (...) {
        return -4;
    }
}

DXAIT_TRACE_API void DXAIT_TRACE_CALL dx_trace_session_destroy(
    dx_trace_session_t* session) {
    delete session;
}

DXAIT_TRACE_API int32_t DXAIT_TRACE_CALL dx_trace_begin(
    dx_trace_session_t* session,
    const char* category,
    const char* name,
    uint64_t request_id,
    dx_trace_span_t** out_span) {
    if (session == nullptr || !valid_text(category) || !valid_text(name) || out_span == nullptr) {
        return -1;
    }
    *out_span = nullptr;
    try {
        auto event = std::make_unique<dx_trace_event_t>();
        event->category = category;
        event->name = name;
        event->request_id = request_id;
        event->thread_id = thread_id();
        event->begin_us = elapsed_us(*session);
        auto span = std::make_unique<dx_trace_span_t>();
        span->session = session;
        span->event = event.get();
        {
            std::lock_guard<std::mutex> lock(session->mutex);
            session->events.push_back(std::move(event));
        }
        *out_span = span.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -2;
    } catch (...) {
        return -3;
    }
}

DXAIT_TRACE_API int32_t DXAIT_TRACE_CALL dx_trace_end(dx_trace_span_t* span) {
    if (span == nullptr || span->session == nullptr || span->event == nullptr || span->ended) {
        return -1;
    }
    {
        std::lock_guard<std::mutex> lock(span->session->mutex);
        if (span->ended || span->event->complete) {
            return -2;
        }
        span->event->duration_us = elapsed_us(*span->session) - span->event->begin_us;
        span->event->complete = true;
        span->ended = true;
    }
    dx_component_logger_write(span->session->logger,
                               DX_COMPONENT_LEVEL_PERFORMANCE,
                               "trace_span_complete",
                               __FILE__,
                               __LINE__);
    return 0;
}

DXAIT_TRACE_API void DXAIT_TRACE_CALL dx_trace_span_destroy(dx_trace_span_t* span) {
    delete span;
}

DXAIT_TRACE_API int32_t DXAIT_TRACE_CALL dx_trace_export(
    const dx_trace_session_t* session,
    const char* path) {
    if (session == nullptr || !valid_text(path)) {
        return -1;
    }
    try {
        std::ofstream output(path, std::ios::out | std::ios::trunc);
        if (!output.is_open()) {
            return -2;
        }
        std::lock_guard<std::mutex> lock(session->mutex);
        output << "{\"traceEvents\":[";
        bool first = true;
        for (const auto& event : session->events) {
            if (!event->complete) {
                continue;
            }
            if (!first) {
                output << ',';
            }
            first = false;
            output << "{\"name\":\"" << json_escape(event->name)
                   << "\",\"cat\":\"" << json_escape(event->category)
                   << "\",\"ph\":\"X\",\"ts\":" << event->begin_us
                   << ",\"dur\":" << event->duration_us
                   << ",\"pid\":1,\"tid\":" << event->thread_id
                   << ",\"args\":{\"request_id\":" << event->request_id << "}}";
        }
        output << "]}\n";
        return output.good() ? 0 : -3;
    } catch (...) {
        return -4;
    }
}

DXAIT_TRACE_API uint64_t DXAIT_TRACE_CALL dx_trace_event_count(
    const dx_trace_session_t* session) {
    if (session == nullptr) {
        return 0u;
    }
    std::lock_guard<std::mutex> lock(session->mutex);
    return static_cast<uint64_t>(session->events.size());
}

} // extern "C"
