// [DXAIT-COMPONENT: dxcore]
// [DXAIT-SUBSYSTEM: logging]
// [DXAIT-IMPLEMENTATION: DLL-scoped diagnostics and trace spans]

#include "dxait/dx_log.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <thread>

namespace dxait::core {
namespace {

// [DXAIT-ANCHOR] This address lets a DLL discover its own directory rather
// than accidentally resolving the host executable's directory.
void dxait_module_anchor() noexcept {}

std::string lower_ascii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string json_escape(std::string_view value) {
    std::string result;
    result.reserve(value.size() + 8u);
    for (const char ch : value) {
        switch (ch) {
        case '\\': result += "\\\\"; break;
        case '"': result += "\\\""; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default: result += ch; break;
        }
    }
    return result;
}

} // namespace

ComponentLogger::ComponentLogger(std::string component)
    : m_component(lower_ascii(std::move(component))) {
    m_config.component = m_component;
}

ComponentLogger::~ComponentLogger() {
    shutdown();
}

bool ComponentLogger::initialize() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) {
        return m_stream.is_open() || m_config.modes == LogMode::None;
    }

    m_config.modes = read_modes(m_component);
    m_initialized = true;

    // [DXAIT-FASTPATH] With no mode enabled, do not touch the filesystem.
    if (m_config.modes == LogMode::None) {
        return true;
    }

    try {
        const auto module_directory = resolve_module_directory();
        m_config.log_directory = module_directory / "logs";

        std::error_code error;
        std::filesystem::create_directories(m_config.log_directory, error);
        if (error) {
            // [DXAIT-NONFATAL] Logging must never make inference fail.
            m_config.log_directory = std::filesystem::current_path(error) / "logs";
            if (!error) {
                std::filesystem::create_directories(m_config.log_directory, error);
            }
        }
        if (error) {
            return false;
        }

        const std::string filename = m_component + "_" + timestamp_for_filename() +
                                     "_" + process_id_string() + ".log";
        m_config.log_file = m_config.log_directory / filename;
        m_stream.open(m_config.log_file, std::ios::out | std::ios::app);
        if (!m_stream.is_open()) {
            return false;
        }

        m_stream << "# dxait component=" << m_component
                 << " modes=" << static_cast<std::uint32_t>(m_config.modes)
                 << " pid=" << process_id_string()
                 << " tid=" << thread_id_string() << '\n';
        m_stream.flush();
        return true;
    } catch (...) {
        // [DXAIT-NONFATAL] Filesystem or formatting failures are isolated.
        m_stream.close();
        return false;
    }
}

void ComponentLogger::shutdown() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_stream.is_open()) {
        m_stream.flush();
        m_stream.close();
    }
    m_initialized = false;
}

bool ComponentLogger::enabled(LogMode mode) const noexcept {
    if (!m_initialized) {
        // Initialization is intentionally explicit so DLL startup does not
        // perform filesystem work under the loader lock.
        return false;
    }
    return has_mode(m_config.modes, mode);
}

bool ComponentLogger::file_open() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stream.is_open();
}

void ComponentLogger::write(LogLevel level,
                            std::string_view message,
                            const char* source_file,
                            int source_line) noexcept {
    const LogMode mode = (level == LogLevel::Performance) ? LogMode::Perf
                       : (level == LogLevel::Trace) ? LogMode::Trace
                       : (level == LogLevel::Diagnostic) ? LogMode::Diagnostic
                       : LogMode::Debug;
    if (!enabled(mode)) {
        return;
    }

    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_stream.is_open()) {
            return;
        }
        m_stream << "{\"ts\":\"" << timestamp_for_record()
                 << "\",\"pid\":\"" << process_id_string()
                 << "\",\"tid\":\"" << thread_id_string()
                 << "\",\"component\":\"" << json_escape(m_component)
                 << "\",\"level\":\"" << level_name(level)
                 << "\",\"file\":\"" << json_escape(source_file ? source_file : "")
                 << "\",\"line\":" << source_line
                 << ",\"message\":\"" << json_escape(message) << "\"}\n";
        m_stream.flush();
    } catch (...) {
        // [DXAIT-NONFATAL] A broken log sink is never propagated to callers.
    }
}

LogMode ComponentLogger::read_modes(std::string_view component) noexcept {
    LogMode modes = LogMode::None;
    const std::string prefix = "dx12_" + std::string(component) + "_";
    if (environment_enabled(prefix + "debug")) {
        modes = modes | LogMode::Debug;
    }
    if (environment_enabled(prefix + "trace")) {
        modes = modes | LogMode::Trace;
    }
    if (environment_enabled(prefix + "perf")) {
        modes = modes | LogMode::Perf;
    }
    if (environment_enabled(prefix + "diag")) {
        modes = modes | LogMode::Diagnostic;
    }
    return modes;
}

std::filesystem::path ComponentLogger::resolve_module_directory() noexcept {
#ifdef _WIN32
    HMODULE module = nullptr;
    const auto anchor = reinterpret_cast<LPCWSTR>(&dxait_module_anchor);
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           anchor, &module) != 0) {
        std::array<wchar_t, 32768> buffer{};
        const DWORD length = GetModuleFileNameW(module, buffer.data(),
                                                static_cast<DWORD>(buffer.size()));
        if (length > 0u && length < buffer.size()) {
            return std::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        }
    }
#endif
    std::error_code error;
    const auto current = std::filesystem::current_path(error);
    return error ? std::filesystem::path{} : current;
}

std::string ComponentLogger::timestamp_for_filename() noexcept {
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y%m%d_%H%M%S");
    return output.str();
}

std::string ComponentLogger::timestamp_for_record() noexcept {
    const auto now = std::chrono::system_clock::now();
    const auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - seconds).count();
    const std::time_t time = std::chrono::system_clock::to_time_t(seconds);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    std::ostringstream output;
    output << std::put_time(&local, "%Y-%m-%dT%H:%M:%S")
           << '.' << std::setfill('0') << std::setw(3) << millis;
    return output.str();
}

std::string ComponentLogger::process_id_string() noexcept {
#ifdef _WIN32
    return std::to_string(GetCurrentProcessId());
#else
    return "0";
#endif
}

std::string ComponentLogger::thread_id_string() noexcept {
#ifdef _WIN32
    return std::to_string(GetCurrentThreadId());
#else
    return std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
#endif
}

bool ComponentLogger::environment_enabled(std::string_view name) noexcept {
#ifdef _WIN32
    std::array<char, 128> buffer{};
    const std::string key(name);
    const DWORD length = GetEnvironmentVariableA(key.c_str(), buffer.data(),
                                                  static_cast<DWORD>(buffer.size()));
    if (length == 0u) {
        return false;
    }
    if (length >= buffer.size()) {
        std::string expanded(length + 1u, '\0');
        const DWORD actual = GetEnvironmentVariableA(key.c_str(), expanded.data(),
                                                       static_cast<DWORD>(expanded.size()));
        if (actual == 0u) {
            return false;
        }
        expanded.resize(actual);
        const std::string value = lower_ascii(std::move(expanded));
        return value != "0" && value != "false" && value != "off" && value != "no";
    }
    const std::string value = lower_ascii(std::string(buffer.data(), length));
    return value != "0" && value != "false" && value != "off" && value != "no";
#else
    const std::string key(name);
    const char* raw = std::getenv(key.c_str());
    if (raw == nullptr || raw[0] == '\0') {
        return false;
    }
    const std::string value = lower_ascii(raw);
    return value != "0" && value != "false" && value != "off" && value != "no";
#endif
}

const char* ComponentLogger::level_name(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Debug: return "debug";
    case LogLevel::Info: return "info";
    case LogLevel::Warning: return "warning";
    case LogLevel::Error: return "error";
    case LogLevel::Trace: return "trace";
    case LogLevel::Performance: return "performance";
    case LogLevel::Diagnostic: return "diagnostic";
    }
    return "unknown";
}

TraceSpan::TraceSpan(ComponentLogger& logger,
                     std::string_view name,
                     const char* source_file,
                     int source_line) noexcept
    : m_logger(&logger),
      m_name(name),
      m_source_file(source_file),
      m_source_line(source_line),
      m_begin(std::chrono::steady_clock::now()),
      m_enabled(logger.enabled(LogMode::Trace) || logger.enabled(LogMode::Perf)) {
    if (m_enabled) {
        m_logger->write(LogLevel::Trace, "begin:" + m_name, m_source_file, m_source_line);
    }
}

TraceSpan::~TraceSpan() {
    if (!m_enabled || m_logger == nullptr) {
        return;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - m_begin).count();
    std::ostringstream message;
    message << "end:" << m_name << " duration_us=" << elapsed;
    m_logger->write(LogLevel::Performance, message.str(), m_source_file, m_source_line);
}

} // namespace dxait::core
