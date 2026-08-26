# [DXAIT-COMPONENT: build]
# [DXAIT-SUBSYSTEM: Windows SDK discovery]
#
# Windows SDK headers and system libraries are expected to come from the
# installed Visual Studio/Windows SDK toolchain. Optional redistributables are
# discovered from a bundled root or an explicit SDK root.

include_guard(GLOBAL)

function(dxait_configure_windows_sdk)
    if (NOT WIN32)
        set(DXAIT_WINDOWS_SDK_AVAILABLE FALSE PARENT_SCOPE)
        return()
    endif()

    # [DXAIT-CONTRACT] MSVC and clang-cl already receive Windows SDK include
    # and library paths from their selected Visual Studio toolchain.
    if (DXAIT_WINDOWS_SDK_ROOT)
        if (EXISTS "${DXAIT_WINDOWS_SDK_ROOT}/Include")
            include_directories("${DXAIT_WINDOWS_SDK_ROOT}/Include")
            set(DXAIT_WINDOWS_SDK_AVAILABLE TRUE PARENT_SCOPE)
        else()
            message(FATAL_ERROR "DXAIT_WINDOWS_SDK_ROOT does not contain Include: ${DXAIT_WINDOWS_SDK_ROOT}")
        endif()
    else()
        set(DXAIT_WINDOWS_SDK_AVAILABLE TRUE PARENT_SCOPE)
    endif()

    if (NOT DXAIT_AGILITY_SDK_ROOT AND EXISTS "${CMAKE_SOURCE_DIR}/vendor/AgilitySDK")
        set(DXAIT_AGILITY_SDK_ROOT "${CMAKE_SOURCE_DIR}/vendor/AgilitySDK" CACHE PATH
            "DirectX 12 Agility SDK root" FORCE)
    endif()
    if (NOT DXAIT_DXC_SDK_ROOT AND EXISTS "${CMAKE_SOURCE_DIR}/vendor/DXC")
        set(DXAIT_DXC_SDK_ROOT "${CMAKE_SOURCE_DIR}/vendor/DXC" CACHE PATH
            "DXC SDK root" FORCE)
    endif()
    if (NOT DXAIT_DIRECTSTORAGE_SDK_ROOT AND EXISTS "${CMAKE_SOURCE_DIR}/vendor/DirectStorage")
        set(DXAIT_DIRECTSTORAGE_SDK_ROOT "${CMAKE_SOURCE_DIR}/vendor/DirectStorage" CACHE PATH
            "DirectStorage SDK root" FORCE)
    endif()
    if (NOT DXAIT_HIP_SDK_ROOT AND EXISTS "${CMAKE_SOURCE_DIR}/vendor/HIP")
        set(DXAIT_HIP_SDK_ROOT "${CMAKE_SOURCE_DIR}/vendor/HIP" CACHE PATH
            "AMD HIP SDK root" FORCE)
    endif()
endfunction()

function(dxait_print_sdk_status)
    if (WIN32)
        message(STATUS "DXAiSt Windows SDK: installed toolchain or DXAIT_WINDOWS_SDK_ROOT")
    else()
        message(STATUS "DXAiSt Windows SDK: deferred (non-Windows host)")
    endif()
    message(STATUS "DXAiSt DirectX-Headers: ${DXAIT_DIRECTX_HEADERS_ROOT}")
    message(STATUS "DXAiSt Agility SDK root: ${DXAIT_AGILITY_SDK_ROOT}")
    message(STATUS "DXAiSt DXC SDK root: ${DXAIT_DXC_SDK_ROOT}")
    message(STATUS "DXAiSt DirectStorage SDK root: ${DXAIT_DIRECTSTORAGE_SDK_ROOT}")
    message(STATUS "DXAiSt HIP SDK root: ${DXAIT_HIP_SDK_ROOT}")
endfunction()
