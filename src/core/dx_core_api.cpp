// [DXAIT-COMPONENT: dxcore]
// [DXAIT-SUBSYSTEM: C99 diagnostics ABI]
// [DXAIT-IMPLEMENTATION: opaque logger bridge]

#include "dxait/dx_core_api.h"
#include "dxait/dx_log.hpp"

#include <memory>
#include <new>
#include <string>

struct dx_component_logger_t {
    std::unique_ptr<dxait::core::ComponentLogger> implementation;
};

namespace {

bool valid_mode(std::uint32_t mode) noexcept {
    return mode == DX_COMPONENT_LOG_DEBUG ||
           mode == DX_COMPONENT_LOG_TRACE ||
           mode == DX_COMPONENT_LOG_PERF ||
           mode == DX_COMPONENT_LOG_DIAGNOSTIC;
}

dxait::core::LogMode to_cpp_mode(std::uint32_t mode) noexcept {
    switch (mode) {
    case DX_COMPONENT_LOG_DEBUG: return dxait::core::LogMode::Debug;
    case DX_COMPONENT_LOG_TRACE: return dxait::core::LogMode::Trace;
    case DX_COMPONENT_LOG_PERF: return dxait::core::LogMode::Perf;
    case DX_COMPONENT_LOG_DIAGNOSTIC: return dxait::core::LogMode::Diagnostic;
    default: return dxait::core::LogMode::None;
    }
}

dxait::core::LogLevel to_cpp_level(std::uint32_t level) noexcept {
    switch (level) {
    case DX_COMPONENT_LEVEL_DEBUG: return dxait::core::LogLevel::Debug;
    case DX_COMPONENT_LEVEL_INFO: return dxait::core::LogLevel::Info;
    case DX_COMPONENT_LEVEL_WARNING: return dxait::core::LogLevel::Warning;
    case DX_COMPONENT_LEVEL_ERROR: return dxait::core::LogLevel::Error;
    case DX_COMPONENT_LEVEL_TRACE: return dxait::core::LogLevel::Trace;
    case DX_COMPONENT_LEVEL_PERFORMANCE: return dxait::core::LogLevel::Performance;
    case DX_COMPONENT_LEVEL_DIAGNOSTIC: return dxait::core::LogLevel::Diagnostic;
    default: return dxait::core::LogLevel::Error;
    }
}

} // namespace

extern "C" {

DXAIT_CORE_API int32_t DXAIT_CORE_CALL dx_component_logger_create(
    const char* component,
    dx_component_logger_t** out_logger) {
    if (component == nullptr || component[0] == '\0' || out_logger == nullptr) {
        return -1;
    }
    *out_logger = nullptr;

    try {
        auto logger = std::make_unique<dx_component_logger_t>();
        logger->implementation = std::make_unique<dxait::core::ComponentLogger>(component);
        // Initialization is best effort. A missing log directory or a locked
        // file is reported through diagnostics only and never blocks runtime use.
        (void)logger->implementation->initialize();
        *out_logger = logger.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    } catch (...) {
        return -4;
    }
}

DXAIT_CORE_API void DXAIT_CORE_CALL dx_component_logger_destroy(
    dx_component_logger_t* logger) {
    delete logger;
}

DXAIT_CORE_API uint32_t DXAIT_CORE_CALL dx_component_logger_modes(
    const dx_component_logger_t* logger) {
    if (logger == nullptr || logger->implementation == nullptr) {
        return DX_COMPONENT_LOG_NONE;
    }
    return static_cast<uint32_t>(logger->implementation->config().modes);
}

DXAIT_CORE_API int32_t DXAIT_CORE_CALL dx_component_logger_enabled(
    const dx_component_logger_t* logger,
    uint32_t mode) {
    if (logger == nullptr || logger->implementation == nullptr || !valid_mode(mode)) {
        return 0;
    }
    return logger->implementation->enabled(to_cpp_mode(mode)) ? 1 : 0;
}

DXAIT_CORE_API void DXAIT_CORE_CALL dx_component_logger_write(
    dx_component_logger_t* logger,
    uint32_t level,
    const char* message,
    const char* source_file,
    int32_t source_line) {
    if (logger == nullptr || logger->implementation == nullptr || message == nullptr) {
        return;
    }
    logger->implementation->write(to_cpp_level(level), message, source_file, source_line);
}

} // extern "C"
