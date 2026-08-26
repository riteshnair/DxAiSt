// [DXAIT-COMPONENT: dxgraph]
// [DXAIT-SUBSYSTEM: operator registry and execution plan ABI]
// [DXAIT-IMPLEMENTATION: registry, graph validation, and topological compile]

#include "dxait/dx_graph_api.h"
#include "dxait/dx_core_api.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct dx_operator_registry_t {
    dx_component_logger_t* logger{nullptr};
    mutable std::mutex mutex;
    std::unordered_set<std::string> operators;
    bool frozen{false};

    ~dx_operator_registry_t() {
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }
};

struct dx_graph_node_t {
    uint32_t id{0u};
    std::string operator_name;
    std::vector<uint32_t> dependencies;
};

struct dx_graph_t {
    dx_component_logger_t* logger{nullptr};
    const dx_operator_registry_t* registry{nullptr};
    std::vector<dx_graph_node_t> nodes;

    ~dx_graph_t() {
        dx_component_logger_destroy(logger);
        logger = nullptr;
    }
};

struct dx_plan_t {
    std::vector<uint32_t> order;
    std::unordered_map<uint32_t, std::string> names;
};

namespace {

bool valid_text(const char* text) noexcept {
    return text != nullptr && text[0] != '\0';
}

bool contains_node(const dx_graph_t& graph, uint32_t id) noexcept {
    return id < graph.nodes.size() && graph.nodes[id].id == id;
}

bool visit_node(const dx_graph_t& graph,
               uint32_t id,
               std::vector<uint8_t>& marks,
               std::vector<uint32_t>& order) {
    if (!contains_node(graph, id)) {
        return false;
    }
    if (marks[id] == 1u) {
        return false;
    }
    if (marks[id] == 2u) {
        return true;
    }
    marks[id] = 1u;
    for (const uint32_t dependency : graph.nodes[id].dependencies) {
        if (!visit_node(graph, dependency, marks, order)) {
            return false;
        }
    }
    marks[id] = 2u;
    order.push_back(id);
    return true;
}

void log_debug(dx_component_logger_t* logger, const std::string& message) noexcept {
    dx_component_logger_write(logger, DX_COMPONENT_LEVEL_DEBUG, message.c_str(), __FILE__, __LINE__);
}

} // namespace

extern "C" {

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_operator_registry_create(
    dx_operator_registry_t** out_registry) {
    if (out_registry == nullptr) {
        return -1;
    }
    *out_registry = nullptr;
    try {
        auto registry = std::make_unique<dx_operator_registry_t>();
        if (dx_component_logger_create("dxgraph", &registry->logger) != 0) {
            return -2;
        }
        *out_registry = registry.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    } catch (...) {
        return -4;
    }
}

DXAIT_GRAPH_API void DXAIT_GRAPH_CALL dx_operator_registry_destroy(
    dx_operator_registry_t* registry) {
    delete registry;
}

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_operator_register(
    dx_operator_registry_t* registry,
    const char* name) {
    if (registry == nullptr || !valid_text(name)) {
        return -1;
    }
    std::lock_guard<std::mutex> lock(registry->mutex);
    if (registry->frozen) {
        return -2;
    }
    try {
        registry->operators.emplace(name);
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    }
}

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_operator_registry_freeze(
    dx_operator_registry_t* registry) {
    if (registry == nullptr) {
        return -1;
    }
    std::lock_guard<std::mutex> lock(registry->mutex);
    registry->frozen = true;
    return 0;
}

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_operator_is_registered(
    const dx_operator_registry_t* registry,
    const char* name) {
    if (registry == nullptr || !valid_text(name)) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(registry->mutex);
    return registry->operators.find(name) != registry->operators.end() ? 1 : 0;
}

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_graph_create(
    const dx_operator_registry_t* registry,
    dx_graph_t** out_graph) {
    if (registry == nullptr || out_graph == nullptr) {
        return -1;
    }
    *out_graph = nullptr;
    try {
        auto graph = std::make_unique<dx_graph_t>();
        if (dx_component_logger_create("dxgraph", &graph->logger) != 0) {
            return -2;
        }
        graph->registry = registry;
        *out_graph = graph.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    } catch (...) {
        return -4;
    }
}

DXAIT_GRAPH_API void DXAIT_GRAPH_CALL dx_graph_destroy(dx_graph_t* graph) {
    delete graph;
}

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_graph_add_node(
    dx_graph_t* graph,
    const char* operator_name,
    const uint32_t* dependency_ids,
    uint32_t dependency_count,
    uint32_t* out_node_id) {
    if (graph == nullptr || !valid_text(operator_name) || out_node_id == nullptr ||
        (dependency_count != 0u && dependency_ids == nullptr)) {
        return -1;
    }
    if (!dx_operator_is_registered(graph->registry, operator_name)) {
        return -2;
    }
    for (uint32_t index = 0u; index < dependency_count; ++index) {
        if (dependency_ids[index] >= graph->nodes.size()) {
            return -3;
        }
    }
    try {
        dx_graph_node_t node;
        node.id = static_cast<uint32_t>(graph->nodes.size());
        node.operator_name = operator_name;
        if (dependency_count != 0u) {
            node.dependencies.assign(dependency_ids, dependency_ids + dependency_count);
        }
        graph->nodes.push_back(std::move(node));
        *out_node_id = graph->nodes.back().id;
        log_debug(graph->logger, "graph_add_node id=" + std::to_string(*out_node_id));
        return 0;
    } catch (const std::bad_alloc&) {
        return -4;
    }
}

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_graph_compile(
    const dx_graph_t* graph,
    dx_plan_t** out_plan) {
    if (graph == nullptr || out_plan == nullptr) {
        return -1;
    }
    *out_plan = nullptr;
    try {
        auto plan = std::make_unique<dx_plan_t>();
        std::vector<uint8_t> marks(graph->nodes.size(), 0u);
        for (uint32_t id = 0u; id < graph->nodes.size(); ++id) {
            if (!visit_node(*graph, id, marks, plan->order)) {
                return -2;
            }
            plan->names.emplace(id, graph->nodes[id].operator_name);
        }
        *out_plan = plan.release();
        return 0;
    } catch (const std::bad_alloc&) {
        return -3;
    }
}

DXAIT_GRAPH_API void DXAIT_GRAPH_CALL dx_plan_destroy(dx_plan_t* plan) {
    delete plan;
}

DXAIT_GRAPH_API uint32_t DXAIT_GRAPH_CALL dx_plan_node_count(const dx_plan_t* plan) {
    return plan == nullptr ? 0u : static_cast<uint32_t>(plan->order.size());
}

DXAIT_GRAPH_API int32_t DXAIT_GRAPH_CALL dx_plan_node_at(
    const dx_plan_t* plan,
    uint32_t order_index,
    uint32_t* out_node_id,
    char* operator_name,
    uint32_t operator_name_capacity) {
    if (plan == nullptr || out_node_id == nullptr || operator_name == nullptr ||
        operator_name_capacity == 0u || order_index >= plan->order.size()) {
        return -1;
    }
    const uint32_t node_id = plan->order[order_index];
    const auto iterator = plan->names.find(node_id);
    if (iterator == plan->names.end() || iterator->second.size() + 1u > operator_name_capacity) {
        return -2;
    }
    *out_node_id = node_id;
    std::memcpy(operator_name, iterator->second.c_str(), iterator->second.size() + 1u);
    return 0;
}

} // extern "C"
