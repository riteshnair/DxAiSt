#pragma once

// [DXAIT-COMPONENT: dxcore]
// [DXAIT-SUBSYSTEM: logging]
// [DXAIT-ABI: C++20]
//
// DLL-scoped diagnostics for the native runtime. A component is the DLL name,
// for example dxcore or dxmemory. Environment variables are resolved as:
//   dx12_<dll_component>_debug=1
//   dx12_<dll_component>_trace=1
//   dx12_<dll_component>_perf=1
//   dx12_<dll_component>_diag=1
//
// Logging is deliberately non-fatal. Failure to create a log file must never
// change the result of an inference operation.

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

namespace dxait::core {

enum class LogMode : std::uint32_t {
    None = 0u,
    Debug = 1u << 0u,
    Trace = 1u << 1u,
    Perf = 1u << 2u,
    Diagnostic = 1u << 3u,
};

constexpr LogMode operator|(LogMode lhs, LogMode rhs) noexcept {
    return static_cast<LogMode>(static_cast<std::uint32_t>(lhs) |
                                static_cast<std::uint32_t>(rhs));
}

constexpr bool has_mode(LogMode value, LogMode mode) noexcept {
    return (static_cast<std::uint32_t>(value) & static_cast<std::uint32_t>(mode)) != 0u;
}

enum class LogLevel : std::uint32_t {
    Debug,
    Info,
    Warning,
    Error,
    Trace,
    Performance,
    Diagnostic,
};

struct ComponentLogConfig {
    std::string component;
    LogMode modes{LogMode::None};
    std::filesystem::path log_directory;
    std::filesystem::path log_file;
};

class ComponentLogger final {
public:
    // [DXAIT-CONTRACT] component is the DLL component name, not a class name.
    explicit ComponentLogger(std::string component);
    ~ComponentLogger();

    ComponentLogger(const ComponentLogger&) = delete;
    ComponentLogger& operator=(const ComponentLogger&) = delete;

    bool initialize() noexcept;
    void shutdown() noexcept;

    [[nodiscard]] const ComponentLogConfig& config() const noexcept { return m_config; }
    [[nodiscard]] bool enabled(LogMode mode) const noexcept;
    [[nodiscard]] bool file_open() const noexcept;

    void write(LogLevel level,
               std::string_view message,
               const char* source_file,
               int source_line) noexcept;

private:
    static LogMode read_modes(std::string_view component) noexcept;
    static std::filesystem::path resolve_module_directory() noexcept;
    static std::string timestamp_for_filename() noexcept;
    static std::string timestamp_for_record() noexcept;
    static std::string process_id_string() noexcept;
    static std::string thread_id_string() noexcept;
    static bool environment_enabled(std::string_view name) noexcept;
    static const char* level_name(LogLevel level) noexcept;

    std::string m_component;
    ComponentLogConfig m_config;
    mutable std::mutex m_mutex;
    std::ofstream m_stream;
    bool m_initialized{false};
};

class TraceSpan final {
public:
    TraceSpan(ComponentLogger& logger,
              std::string_view name,
              const char* source_file,
              int source_line) noexcept;
    ~TraceSpan();

    TraceSpan(const TraceSpan&) = delete;
    TraceSpan& operator=(const TraceSpan&) = delete;

private:
    ComponentLogger* m_logger{nullptr};
    std::string m_name;
    const char* m_source_file{nullptr};
    int m_source_line{0};
    std::chrono::steady_clock::time_point m_begin{};
    bool m_enabled{false};
};

} // namespace dxait::core

// [DXAIT-MACRO] Stream formatting is guarded so disabled modes do not format.
#include <sstream>

#define DXAIT_LOG(logger, mode, level, expression)                                  \
    do {                                                                             \
        if ((logger).enabled(mode)) {                                                \
            std::ostringstream dxait_log_stream__;                                  \
            dxait_log_stream__ << expression;                                       \
            (logger).write(level, dxait_log_stream__.str(), __FILE__, __LINE__);    \
        }                                                                            \
    } while (false)

#define DXAIT_DEBUG(logger, expression)                                               \
    DXAIT_LOG(logger, ::dxait::core::LogMode::Debug,                                \
              ::dxait::core::LogLevel::Debug, expression)

#define DXAIT_TRACE(logger, expression)                                               \
    DXAIT_LOG(logger, ::dxait::core::LogMode::Trace,                                \
              ::dxait::core::LogLevel::Trace, expression)

#define DXAIT_PERF(logger, expression)                                                \
    DXAIT_LOG(logger, ::dxait::core::LogMode::Perf,                                 \
              ::dxait::core::LogLevel::Performance, expression)

#define DXAIT_DIAG(logger, expression)                                               \
    DXAIT_LOG(logger, ::dxait::core::LogMode::Diagnostic,                           \
              ::dxait::core::LogLevel::Diagnostic, expression)

#define DXAIT_TRACE_SPAN(logger, name)                                               \
    ::dxait::core::TraceSpan dxait_trace_span__((logger), (name), __FILE__, __LINE__)
