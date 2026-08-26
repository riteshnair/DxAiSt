// [DXAIT-COMPONENT: dxcore]
// [DXAIT-SUBSYSTEM: logging test]
// [DXAIT-TEST: DLL-scoped environment modes and C99 ABI]

#include "dxait/dx_core_api.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <string>

namespace {

void set_environment(const char* name, const char* value) {
#ifdef _WIN32
    const int result = _putenv_s(name, value);
#else
    const int result = setenv(name, value, 1);
#endif
    assert(result == 0);
    (void) result;
}

void clear_environment(const char* name) {
#ifdef _WIN32
    const int result = _putenv_s(name, "");
#else
    const int result = unsetenv(name);
#endif
    assert(result == 0);
    (void) result;
}

} // namespace

int main() {
    // [DXAIT-TRACE] The test exercises each independent mode and verifies
    // that the component name is part of the environment-variable key.
    set_environment("dx12_dxcore_debug", "1");
    set_environment("dx12_dxcore_trace", "1");
    set_environment("dx12_dxcore_perf", "0");
    set_environment("dx12_dxmemory_debug", "1");

    dx_component_logger_t* logger = nullptr;
    assert(dx_component_logger_create("dxcore", &logger) == 0);
    assert(logger != nullptr);
    const uint32_t modes = dx_component_logger_modes(logger);
    (void) modes;
    assert((modes & DX_COMPONENT_LOG_DEBUG) != 0u);
    assert((modes & DX_COMPONENT_LOG_TRACE) != 0u);
    assert((modes & DX_COMPONENT_LOG_PERF) == 0u);
    assert(dx_component_logger_enabled(logger, DX_COMPONENT_LOG_DEBUG) == 1);
    assert(dx_component_logger_enabled(logger, DX_COMPONENT_LOG_TRACE) == 1);
    assert(dx_component_logger_enabled(logger, DX_COMPONENT_LOG_PERF) == 0);

    dx_component_logger_write(logger,
                               DX_COMPONENT_LEVEL_DEBUG,
                               "dxcore logger test",
                               __FILE__,
                               __LINE__);
    dx_component_logger_write(logger,
                               DX_COMPONENT_LEVEL_TRACE,
                               "dxcore trace test",
                               __FILE__,
                               __LINE__);
    dx_component_logger_destroy(logger);

    dx_component_logger_t* disabled = nullptr;
    assert(dx_component_logger_create("dxmemory", &disabled) == 0);
    assert(disabled != nullptr);
    assert(dx_component_logger_enabled(disabled, DX_COMPONENT_LOG_DEBUG) == 1);
    dx_component_logger_destroy(disabled);


    clear_environment("dx12_dxcore_debug");
    clear_environment("dx12_dxcore_trace");
    clear_environment("dx12_dxcore_perf");
    clear_environment("dx12_dxmemory_debug");
    return 0;
}
