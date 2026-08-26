// [DXAIT-COMPONENT: dxgraph]
// [DXAIT-SUBSYSTEM: operator registry and plan test]
// [DXAIT-TEST: registration, dependencies, ordering, and ABI validation]

#include "dxait/dx_graph_api.h"

#include <cassert>
#include <cstring>

int main() {
    dx_operator_registry_t* registry = nullptr;
    assert(dx_operator_registry_create(&registry) == 0);
    assert(registry != nullptr);
    assert(dx_operator_register(registry, "copy") == 0);
    assert(dx_operator_register(registry, "gemm") == 0);
    assert(dx_operator_is_registered(registry, "copy") == 1);
    assert(dx_operator_is_registered(registry, "missing") == 0);
    assert(dx_operator_registry_freeze(registry) == 0);
    assert(dx_operator_register(registry, "late") != 0);

    dx_graph_t* graph = nullptr;
    assert(dx_graph_create(registry, &graph) == 0);
    assert(graph != nullptr);
    uint32_t node0 = 99u;
    uint32_t node1 = 99u;
    (void) node1;
    assert(dx_graph_add_node(graph, "copy", nullptr, 0u, &node0) == 0);
    const uint32_t dependency = node0;
    (void) dependency;
    assert(dx_graph_add_node(graph, "gemm", &dependency, 1u, &node1) == 0);
    assert(node1 == node0 + 1u);
    assert(dx_graph_add_node(graph, "unknown", nullptr, 0u, &node1) != 0);
    const uint32_t invalid_dependency = 999u;
    (void) invalid_dependency;
    assert(dx_graph_add_node(graph, "gemm", &invalid_dependency, 1u, &node1) != 0);

    dx_plan_t* plan = nullptr;
    assert(dx_graph_compile(graph, &plan) == 0);
    assert(plan != nullptr);
    assert(dx_plan_node_count(plan) == 2u);
    char name[16]{};
    (void) name;
    uint32_t node_id = 99u;
    (void) node_id;
    assert(dx_plan_node_at(plan, 0u, &node_id, name, sizeof(name)) == 0);
    assert(node_id == node0 && std::strcmp(name, "copy") == 0);
    assert(dx_plan_node_at(plan, 1u, &node_id, name, sizeof(name)) == 0);
    assert(node_id == node1 && std::strcmp(name, "gemm") == 0);
    assert(dx_plan_node_at(plan, 0u, &node_id, name, 3u) != 0);
    dx_plan_destroy(plan);
    dx_graph_destroy(graph);

    dx_operator_registry_destroy(registry);
    return 0;
}
