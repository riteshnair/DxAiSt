# [DXAIT-SHADERS] Reproducible HLSL -> DXIL/CSO artifact wiring.
# This module never creates fake artifacts: if DXC is unavailable, the target is
# omitted and the configure log records the missing validation dependency.

function(dxait_configure_shader_artifacts)
    option(DXAIT_BUILD_SHADER_ARTIFACTS
           "Build DxAiSt HLSL kernels as SM 6.7 DXIL/CSO artifacts when DXC is available"
           ON)

    if (NOT DXAIT_BUILD_SHADER_ARTIFACTS)
        message(STATUS "DXAiSt shader artifacts: disabled")
        return()
    endif()

    set(_dxc_candidates)
    if (WIN32 AND DXAIT_DXC_SDK_ROOT)
        list(APPEND _dxc_candidates
             "${DXAIT_DXC_SDK_ROOT}/bin/x64/dxc.exe"
             "${DXAIT_DXC_SDK_ROOT}/bin/dxc.exe")
    endif()
    find_program(DXAIT_DXC_EXECUTABLE NAMES dxc dxc.exe
                 PATHS ${_dxc_candidates} NO_DEFAULT_PATH)
    if (NOT DXAIT_DXC_EXECUTABLE)
        find_program(DXAIT_DXC_EXECUTABLE NAMES dxc dxc.exe)
    endif()

    if (NOT DXAIT_DXC_EXECUTABLE)
        message(STATUS "DXAiSt shader artifacts: deferred (DXC executable not found)")
        return()
    endif()

    set(_specs
        "src/shaders/dxattention/flash_attn.hlsl|flash_attention_2"
        "src/shaders/dxattention/paged_attention.hlsl|paged_attention_v2"
        "src/shaders/dxblas/gemv_rdna2_wave64.hlsl|gemv_q8_0_rdna2_wave64"
        "src/shaders/dxblas/gemv_rdna4_wmma.hlsl|gemm_rdna4_wmma"
        "src/shaders/dxconv/conv2d.hlsl|conv2d_kernel"
        "src/shaders/dxfft/fft_radix2.hlsl|fft_radix2"
        "src/shaders/dxkv/rope.hlsl|rope_kernel"
        "src/shaders/dxmath/elementwise.hlsl|vec_add"
        "src/shaders/dxmath/silu_swiglu.hlsl|silu_kernel"
        "src/shaders/dxmath/silu_swiglu.hlsl|swiglu_kernel"
        "src/shaders/dxmodel/moe_routing.hlsl|moe_gate_router"
        "src/shaders/dxmodel/speculative_verify.hlsl|speculative_verify_kernel"
        "src/shaders/dxquant/atom_gemm_int4.hlsl|atom_gemm_int4_kernel"
        "src/shaders/dxsparse/spmv_csr.hlsl|spmv_csr_kernel")

    set(_outputs)
    foreach(_spec IN LISTS _specs)
        string(REPLACE "|" ";" _parts "${_spec}")
        list(GET _parts 0 _source)
        list(GET _parts 1 _entry)
        set(_input "${CMAKE_CURRENT_SOURCE_DIR}/${_source}")
        set(_output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${_entry}.cso")
        add_custom_command(
            OUTPUT "${_output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
            COMMAND "${DXAIT_DXC_EXECUTABLE}" -HV 2021 -T cs_6_7 -E "${_entry}"
                    -O3 -Ges -Fo "${_output}" "${_input}"
            DEPENDS "${_input}"
            COMMENT "DXC SM 6.7 ${_entry}"
            VERBATIM)
        list(APPEND _outputs "${_output}")
    endforeach()

    add_custom_target(dxait_shader_artifacts ALL DEPENDS ${_outputs})
    set(DXAIT_SHADER_ARTIFACT_TARGET dxait_shader_artifacts PARENT_SCOPE)
    set(DXAIT_SHADER_ARTIFACT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders" PARENT_SCOPE)
    message(STATUS "DXAiSt shader artifacts: enabled via ${DXAIT_DXC_EXECUTABLE}")
endfunction()
