#ifndef DXAIT_DX_GRAPH_API_H
#define DXAIT_DX_GRAPH_API_H

// [DXAIT-COMPONENT: dxgraph]
// [DXAIT-SUBSYSTEM: operator registry and execution plan ABI]
// [DXAIT-ABI: C99]

#include <stdint.h>

#if defined(_WIN32)
#  if defined(DXAIT_DXGRAPH_BUILD)
#    define DXAIT_GRAPH_API __declspec(dllexport)
#  elif defined(DXAIT_DXGRAPH_USE)
#    define DXAIT_GRAPH_API __declspec(dllimport)
#  else
#    define DXAIT_GRAPH_API
#  endif
#  define DXAIT_GRAPH_CALL __cdecl
#else
#  if defined(__GNUC__) || defined(__clang__)
#    define DXAIT_GRAPH_API __attribute__((visibility("default")))
#  else
#    define DXAIT_GRAPH_API
#  endif
#  define DXAIT_GRAPH_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dx_operator_registry_t dx_operator_registry_t;
typedef struct dx_graph_t dx_graph_t;
typedef struct dx_plan_t dx_plan_t;

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_operator_registry_create(
    dx_operator_registry_t** out_registry);
DXAIT_GRAPH_API void DXAIT_GRAPH_CALL dx_operator_registry_destroy(
    dx_operator_registry_t* registry);
DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_operator_register(
    dx_operator_registry_t* registry,
    const char* name);
DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_operator_registry_freeze(
    dx_operator_registry_t* registry);
DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_operator_is_registered(
    const dx_operator_registry_t* registry,
    const char* name);

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_graph_create(
    const dx_operator_registry_t* registry,
    dx_graph_t** out_graph);
DXAIT_GRAPH_API void DXAIT_GRAPH_CALL dx_graph_destroy(dx_graph_t* graph);
DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_graph_add_node(
    dx_graph_t* graph,
    const char* operator_name,
    const uint32_t* dependency_ids,
    uint32_t dependency_count,
    uint32_t* out_node_id);
DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_graph_compile(
    const dx_graph_t* graph,
    dx_plan_t** out_plan);
DXAIT_GRAPH_API void DXAIT_GRAPH_CALL dx_plan_destroy(dx_plan_t* plan);
DXAIT_GRAPH_API uint32_t DXAIT_GRAPH_CALL dx_plan_node_count(const dx_plan_t* plan);
DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_plan_node_at(
    const dx_plan_t* plan,
    uint32_t order_index,
    uint32_t* out_node_id,
    char* operator_name,
    uint32_t operator_name_capacity);

#ifdef __cplusplus
}
#endif

#endif /* DXAIT_DX_GRAPH_API_H */
