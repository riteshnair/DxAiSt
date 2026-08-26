cmake_minimum_required(VERSION 3.20)
project(dxait C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DXAITWindowsSDK)
include(DXAITShaders)

# [DXAIT-COMPONENT: build]
# The revamp is built as DLL-oriented components. The legacy static target is
# retained temporarily so existing inventory tests remain available while the
# new stack is assembled.
option(DXAIT_BUILD_REVAMP_CORE "Build the new dx12_dxcore DLL" ON)
option(DXAIT_BUILD_LEGACY "Build the legacy inventory library and tests" ON)
option(DXAIT_BUILD_CORE_TESTS "Build tests for the revamped core DLLs" ON)
option(DXAIT_BUILD_C_API_TESTS "Build C99/C++20 compatibility API tests" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/O2 /Ot /Oi /fp:fast /W4 /WX-)
endif()

# [DXAIT-SDK-DISCOVERY] Empty values mean "use system installation or disable
# the optional dependency". No developer-local paths are permitted.
set(DXAIT_AGILITY_SDK_ROOT "" CACHE PATH "DirectX 12 Agility SDK root")
set(DXAIT_DXC_SDK_ROOT "" CACHE PATH "DXC SDK root")
set(DXAIT_DIRECTSTORAGE_SDK_ROOT "" CACHE PATH "DirectStorage SDK root")
set(DXAIT_HIP_SDK_ROOT "" CACHE PATH "AMD HIP SDK root")
set(DXAIT_WINDOWS_SDK_ROOT "" CACHE PATH "Optional explicit Windows SDK root; normally supplied by Visual Studio")
set(DXAIT_DIRECTX_HEADERS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/DirectX-Headers" CACHE PATH "DirectX-Headers source root")

if (DEFINED ENV{DXAIT_AGILITY_SDK_ROOT} AND NOT DXAIT_AGILITY_SDK_ROOT)
    set(DXAIT_AGILITY_SDK_ROOT "$ENV{DXAIT_AGILITY_SDK_ROOT}")
endif()
if (DEFINED ENV{DXAIT_DXC_SDK_ROOT} AND NOT DXAIT_DXC_SDK_ROOT)
    set(DXAIT_DXC_SDK_ROOT "$ENV{DXAIT_DXC_SDK_ROOT}")
endif()
if (DEFINED ENV{DXAIT_DIRECTSTORAGE_SDK_ROOT} AND NOT DXAIT_DIRECTSTORAGE_SDK_ROOT)
    set(DXAIT_DIRECTSTORAGE_SDK_ROOT "$ENV{DXAIT_DIRECTSTORAGE_SDK_ROOT}")
endif()
if (DEFINED ENV{DXAIT_HIP_SDK_ROOT} AND NOT DXAIT_HIP_SDK_ROOT)
    set(DXAIT_HIP_SDK_ROOT "$ENV{DXAIT_HIP_SDK_ROOT}")
endif()
if (DEFINED ENV{DXAIT_WINDOWS_SDK_ROOT} AND NOT DXAIT_WINDOWS_SDK_ROOT)
    set(DXAIT_WINDOWS_SDK_ROOT "$ENV{DXAIT_WINDOWS_SDK_ROOT}")
endif()

dxait_configure_windows_sdk()
dxait_print_sdk_status()
dxait_configure_shader_artifacts()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if (EXISTS "${DXAIT_DIRECTX_HEADERS_ROOT}/include")
    include_directories("${DXAIT_DIRECTX_HEADERS_ROOT}/include")
endif()
if (EXISTS "${DXAIT_AGILITY_SDK_ROOT}/include")
    include_directories("${DXAIT_AGILITY_SDK_ROOT}/include")
endif()
if (EXISTS "${DXAIT_DXC_SDK_ROOT}/inc")
    include_directories("${DXAIT_DXC_SDK_ROOT}/inc")
endif()
if (EXISTS "${DXAIT_DIRECTSTORAGE_SDK_ROOT}/include")
    include_directories("${DXAIT_DIRECTSTORAGE_SDK_ROOT}/include")
endif()
if (EXISTS "${DXAIT_HIP_SDK_ROOT}/include")
    include_directories("${DXAIT_HIP_SDK_ROOT}/include")
endif()

if (EXISTS "${DXAIT_DXC_SDK_ROOT}/lib/x64")
    link_directories("${DXAIT_DXC_SDK_ROOT}/lib/x64")
endif()
if (EXISTS "${DXAIT_DIRECTSTORAGE_SDK_ROOT}/lib/x64")
    link_directories("${DXAIT_DIRECTSTORAGE_SDK_ROOT}/lib/x64")
endif()

set(DXAIT_SOURCES
    src/dxadapter/dxadapter.cpp
    src/dxruntime/dxruntime.cpp
    src/dxqueue/dxqueue.cpp
    src/dxmem/dxmem.cpp
    src/dxbarrier/dxbarrier.cpp
    src/dxio/dxio.cpp
    src/dxgraph/dxgraph.cpp
    src/dxtrace/dxtrace.cpp
    src/dxjit/dxjit.cpp
    src/dxblas/dxblas.cpp
    src/dxquant/dxquant.cpp
    src/dxkv/dxkv.cpp
    src/dxmath/dxmath.cpp
    src/dxmodel/dxmodel.cpp
    src/dxsched/dxsched.cpp
    src/dxrand/dxrand.cpp
    src/dxattention/dxattention.cpp
    src/dxfft/dxfft.cpp
    src/dxchunk/dxchunk.cpp
    src/dxshard/dxshard.cpp
    src/dxcache/dxcache.cpp
    src/dxcollective/dxcollective.cpp
    src/dxspeculative/dxspeculative.cpp
    src/dxcontext/dxcontext.cpp
    src/dxdb/dxdb.cpp
    src/dxmcp/dxmcp.cpp
    src/dxnetwork/dxnetwork.cpp
    src/dxtriton/dxtriton.cpp
    src/dxla/dxla.cpp
    src/dxstream/dxstream.cpp
    src/dxiocp/dxiocp.cpp
)

if (DXAIT_BUILD_LEGACY)
add_library(dxait STATIC ${DXAIT_SOURCES})
endif()

if (DXAIT_BUILD_REVAMP_CORE)
    add_library(dx12_dxcore SHARED
        src/core/dx_log.cpp
        src/core/dx_core_api.cpp
    )
    target_include_directories(dx12_dxcore
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${DXAIT_DIRECTX_HEADERS_ROOT}/include
    )
    target_compile_definitions(dx12_dxcore PRIVATE DXAIT_DXCORE_BUILD=1)
    set_target_properties(dx12_dxcore PROPERTIES
        OUTPUT_NAME "dx12_dxcore"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )


    add_library(dx12_dxtensor SHARED src/core/dx_tensor_api.cpp)
    target_include_directories(dx12_dxtensor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

    add_library(dx12_dxdevice SHARED src/core/dx_device_api.cpp)
    target_include_directories(dx12_dxdevice
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${DXAIT_DIRECTX_HEADERS_ROOT}/include
    )
    target_compile_definitions(dx12_dxdevice PRIVATE DXAIT_DXDEVICE_BUILD=1)
    target_link_libraries(dx12_dxdevice PRIVATE dx12_dxcore)
    if (WIN32)
        target_link_libraries(dx12_dxdevice PRIVATE dxgi d3d12)
    endif()
    set_target_properties(dx12_dxdevice PROPERTIES
        OUTPUT_NAME "dx12_dxdevice"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxruntime SHARED src/core/dx_runtime_api.cpp)
    target_include_directories(dx12_dxruntime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxruntime PRIVATE DXAIT_DXRUNTIME_BUILD=1)
    target_link_libraries(dx12_dxruntime PRIVATE dx12_dxcore dx12_dxdevice)
    set_target_properties(dx12_dxruntime PROPERTIES
        OUTPUT_NAME "dx12_dxruntime"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxmemory SHARED src/core/dx_memory_api.cpp)
    target_include_directories(dx12_dxmemory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxmemory PRIVATE DXAIT_DXMEMORY_BUILD=1)
    target_link_libraries(dx12_dxmemory PRIVATE dx12_dxcore)
    set_target_properties(dx12_dxmemory PROPERTIES
        OUTPUT_NAME "dx12_dxmemory"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxstream SHARED src/core/dx_stream_api.cpp)
    target_include_directories(dx12_dxstream PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    add_library(dx12_dxshader SHARED src/core/dx_shader_api.cpp)
    target_include_directories(dx12_dxshader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxshader PRIVATE DXAIT_DXSHADER_BUILD=1)
    target_link_libraries(dx12_dxshader PRIVATE dx12_dxcore)
    if (WIN32 AND DXAIT_DXC_SDK_ROOT)
        target_link_directories(dx12_dxshader PRIVATE "${DXAIT_DXC_SDK_ROOT}/lib/x64")
        target_link_libraries(dx12_dxshader PRIVATE dxcompiler)
    endif()
    set_target_properties(dx12_dxshader PROPERTIES
        OUTPUT_NAME "dx12_dxshader"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxmodel SHARED src/core/dx_model_api.cpp)
    target_include_directories(dx12_dxmodel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxmodel PRIVATE DXAIT_DXMODEL_BUILD=1)
    target_link_libraries(dx12_dxmodel PRIVATE dx12_dxcore dx12_dxio)
    set_target_properties(dx12_dxmodel PROPERTIES
        OUTPUT_NAME "dx12_dxmodel"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxops SHARED src/core/dx_ops_api.cpp)
    target_include_directories(dx12_dxops PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxops PRIVATE DXAIT_DXOPS_BUILD=1)
    target_link_libraries(dx12_dxops PRIVATE dx12_dxcore)
    set_target_properties(dx12_dxops PROPERTIES
        OUTPUT_NAME "dx12_dxops"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxio SHARED src/core/dx_io_api.cpp)
    target_include_directories(dx12_dxio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxio PRIVATE DXAIT_DXIO_BUILD=1)
    target_link_libraries(dx12_dxio PRIVATE dx12_dxcore)
    set_target_properties(dx12_dxio PROPERTIES
        OUTPUT_NAME "dx12_dxio"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxtrace SHARED src/core/dx_trace_api.cpp)
    target_include_directories(dx12_dxtrace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxtrace PRIVATE DXAIT_DXTRACE_BUILD=1)
    target_link_libraries(dx12_dxtrace PRIVATE dx12_dxcore)
    set_target_properties(dx12_dxtrace PROPERTIES
        OUTPUT_NAME "dx12_dxtrace"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxgraph SHARED src/core/dx_graph_api.cpp)
    target_include_directories(dx12_dxgraph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxgraph PRIVATE DXAIT_DXGRAPH_BUILD=1)
    target_link_libraries(dx12_dxgraph PRIVATE dx12_dxcore)
    set_target_properties(dx12_dxgraph PROPERTIES
        OUTPUT_NAME "dx12_dxgraph"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    add_library(dx12_dxcapi SHARED src/core/dx_c_api.cpp)
    target_include_directories(dx12_dxcapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_definitions(dx12_dxcapi PRIVATE DXAIT_DXCAPI_BUILD=1)
    target_link_libraries(dx12_dxcapi PRIVATE
        dx12_dxcore
        dx12_dxdevice
        dx12_dxmemory
        dx12_dxstream
        dx12_dxops
    )
    set_target_properties(dx12_dxcapi PROPERTIES
        OUTPUT_NAME "dx12_dxcapi"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_compile_definitions(dx12_dxstream PRIVATE DXAIT_DXSTREAM_BUILD=1)
    target_link_libraries(dx12_dxstream PRIVATE dx12_dxcore)
    set_target_properties(dx12_dxstream PROPERTIES
        OUTPUT_NAME "dx12_dxstream"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    target_compile_definitions(dx12_dxtensor PRIVATE DXAIT_DXTENSOR_BUILD=1)
    set_target_properties(dx12_dxtensor PROPERTIES
        OUTPUT_NAME "dx12_dxtensor"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

if (DXAIT_BUILD_LEGACY)
target_link_libraries(dxait PRIVATE
    d3d12.lib
    dxgi.lib
    dxguid.lib
    dxcompiler.lib
    dstorage.lib
    Winmm.lib
    ws2_32.lib
)
endif()

# [DXAIT-SDK-DISCOVERY] Copy only explicitly supplied redistributable files.
function(copy_sdk_dlls target)
    set(commands)
    if (WIN32 AND EXISTS "${DXAIT_AGILITY_SDK_ROOT}/bin/x64/D3D12Core.dll")
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${target}>/D3D12)
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXAIT_AGILITY_SDK_ROOT}/bin/x64/D3D12Core.dll" $<TARGET_FILE_DIR:${target}>/D3D12/)
    endif()
    if (WIN32 AND EXISTS "${DXAIT_DXC_SDK_ROOT}/bin/x64/dxcompiler.dll")
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXAIT_DXC_SDK_ROOT}/bin/x64/dxcompiler.dll" $<TARGET_FILE_DIR:${target}>/)
    endif()
    if (WIN32 AND EXISTS "${DXAIT_DXC_SDK_ROOT}/bin/x64/dxil.dll")
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXAIT_DXC_SDK_ROOT}/bin/x64/dxil.dll" $<TARGET_FILE_DIR:${target}>/)
    endif()
    if (WIN32 AND EXISTS "${DXAIT_DIRECTSTORAGE_SDK_ROOT}/bin/x64/dstorage.dll")
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXAIT_DIRECTSTORAGE_SDK_ROOT}/bin/x64/dstorage.dll" $<TARGET_FILE_DIR:${target}>/)
    endif()
    if (WIN32 AND EXISTS "${DXAIT_DIRECTSTORAGE_SDK_ROOT}/bin/x64/dstoragecore.dll")
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DXAIT_DIRECTSTORAGE_SDK_ROOT}/bin/x64/dstoragecore.dll" $<TARGET_FILE_DIR:${target}>/)
    endif()
    if (commands)
        add_custom_command(TARGET ${target} POST_BUILD ${commands})
    endif()
endfunction()

if (DXAIT_BUILD_REVAMP_CORE)
    copy_sdk_dlls(dx12_dxcapi)
    copy_sdk_dlls(dx12_dxshader)
endif()

enable_testing()

if (DXAIT_BUILD_CORE_TESTS AND DXAIT_BUILD_REVAMP_CORE)
    add_executable(test_dxcore_logging tests/test_dxcore_logging.cpp)
    target_include_directories(test_dxcore_logging PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxcore_logging PRIVATE dx12_dxcore)
    add_test(NAME test_dxcore_logging COMMAND test_dxcore_logging)

    add_executable(test_dxruntime tests/test_dxruntime.cpp)
    target_include_directories(test_dxruntime PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxruntime PRIVATE dx12_dxruntime)
    add_test(NAME test_dxruntime COMMAND test_dxruntime)

    add_executable(test_dxtensor_desc tests/test_dxtensor_desc.cpp)
    target_include_directories(test_dxtensor_desc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxtensor_desc PRIVATE dx12_dxtensor)
    add_test(NAME test_dxtensor_desc COMMAND test_dxtensor_desc)

    add_executable(test_dxdevice tests/test_dxdevice.cpp)
    target_include_directories(test_dxdevice PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxdevice PRIVATE dx12_dxdevice)
    add_test(NAME test_dxdevice COMMAND test_dxdevice)

    add_executable(test_dxmemory tests/test_dxmemory.cpp)
    target_include_directories(test_dxmemory PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxmemory PRIVATE dx12_dxmemory)
    add_test(NAME test_dxmemory COMMAND test_dxmemory)

    add_executable(test_dxstream tests/test_dxstream.cpp)
    target_include_directories(test_dxstream PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxstream PRIVATE dx12_dxstream)
    add_test(NAME test_dxstream COMMAND test_dxstream)

    add_executable(test_dxgraph_api tests/test_dxgraph_api.cpp)
    target_include_directories(test_dxgraph_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxgraph_api PRIVATE dx12_dxgraph)
    add_test(NAME test_dxgraph_api COMMAND test_dxgraph_api)

    add_executable(test_dxtrace_api tests/test_dxtrace_api.cpp)
    target_include_directories(test_dxtrace_api PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxtrace_api PRIVATE dx12_dxtrace)
    add_test(NAME test_dxtrace_api COMMAND test_dxtrace_api)

    add_executable(test_dxio tests/test_dxio.cpp)
    target_include_directories(test_dxio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxio PRIVATE dx12_dxio)
    add_test(NAME test_dxio COMMAND test_dxio)

    add_executable(test_dxops tests/test_dxops.cpp)
    target_include_directories(test_dxops PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxops PRIVATE dx12_dxops)
    add_test(NAME test_dxops COMMAND test_dxops)

    add_executable(test_dxmodel tests/test_dxmodel.cpp)
    target_include_directories(test_dxmodel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxmodel PRIVATE dx12_dxmodel)
    add_test(NAME test_dxmodel COMMAND test_dxmodel)

    add_executable(test_dxshader tests/test_dxshader.cpp)
    target_include_directories(test_dxshader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxshader PRIVATE dx12_dxshader)
    add_test(NAME test_dxshader COMMAND test_dxshader)

    if (DXAIT_BUILD_C_API_TESTS)
        add_executable(test_dxcapi_c tests/test_dxcapi.c)
        target_include_directories(test_dxcapi_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_link_libraries(test_dxcapi_c PRIVATE dx12_dxcapi)
        add_test(NAME test_dxcapi_c COMMAND test_dxcapi_c)

        add_executable(test_dxcapi_cpp tests/test_dxcapi.cpp)
        target_include_directories(test_dxcapi_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_link_libraries(test_dxcapi_cpp PRIVATE dx12_dxcapi)
        add_test(NAME test_dxcapi_cpp COMMAND test_dxcapi_cpp)
    endif()

    add_executable(test_dxnative tests/test_dxnative.cpp)
    target_include_directories(test_dxnative PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(test_dxnative PRIVATE
        dx12_dxruntime
        dx12_dxtensor
        dx12_dxmemory
        dx12_dxstream
        dx12_dxtrace
    )
    add_test(NAME test_dxnative COMMAND test_dxnative)
endif()

if (DXAIT_BUILD_LEGACY)
    set(TEST_TARGETS
        test_device_init
        test_mem_alloc
        test_dxjit_blas
        test_substrate_full
        test_model_inference
        test_attention_rand
        test_q4_quant
        test_model_formats
        test_streaming_sharding
        test_real_model_loading
        test_large_gguf_spillover
        test_50_50_offloading
        test_sharding_perf
        test_split_load_inference
        test_chat_inference_harness
        test_512k_rag_mcp
        test_dxnetwork
        test_dxtriton
        test_dxfft
        test_dxcache
        test_dxspeculative
        test_dxgraph
        test_dxtrace
        test_blas_gemm
        test_attention_variants
        test_attn_paged
        test_attn_h2o
        test_attn_chunked
        test_attn_ring
        test_attn_ring_multi
        test_dxla
        test_dstream
    )

    set(TOOL_TARGETS
        dxinspect
        dxbench
        dxai_ingest
    )
else()
    set(TEST_TARGETS)
    set(TOOL_TARGETS)
endif()

foreach(t ${TEST_TARGETS} ${TOOL_TARGETS})
    if (t MATCHES "^test_")
        add_executable(${t} tests/${t}.cpp)
    else()
        add_executable(${t} tools/${t}/main.cpp)
    endif()
    target_link_libraries(${t} PRIVATE dxait)
    copy_sdk_dlls(${t})
    if (t MATCHES "^test_")
        add_test(NAME ${t} COMMAND ${t})
    endif()
endforeach()
