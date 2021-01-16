struct quester_immediate_dependency_tracker_completion_settings
{
    bool force_out_connections;
    enum quester_out_connection_type out_connections_to_force;
};

struct quester_immediate_dependency_tracker
{
    int completed_dependency_count;
    int completed_dependencies[32];
    bool all_dependencies_completed;
};

#define QUESTER_MAX_ELEMENTS_IN_CONTAINER_NODE 64
#define QUESTER_MAX_BRIDGE_NODES 8

struct quester_container_task_data
{
    int in_bridge_node_count;
    int in_bridge_node_ids[QUESTER_MAX_BRIDGE_NODES];

    // Contains the bridge nodes
    int contained_node_count;
    int contained_nodes[QUESTER_MAX_ELEMENTS_IN_CONTAINER_NODE + QUESTER_MAX_BRIDGE_NODES];
};

struct quester_activation_result quester_container_task_activator(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_,
    struct quester_connection *triggering_connection);
struct quester_activation_result quester_container_task_non_activator(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_,
    struct quester_connection *triggering_connection);
void quester_container_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_);
struct quester_tick_result quester_container_task_tick(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_);
void quester_container_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_container_task_data *static_node_data, void *_);

struct quester_and_task_dynamic_data
{
    struct quester_immediate_dependency_tracker dependencies;
    bool has_failed_dependency;
};

struct quester_activation_result quester_and_task_activator(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_and_task_dynamic_data *data,
    struct quester_connection *triggering_connection);
struct quester_activation_result quester_and_task_non_activator(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_and_task_dynamic_data *data,
    struct quester_connection *triggering_connection);
struct quester_tick_result quester_and_task_tick(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings,
    struct quester_and_task_dynamic_data *data);
void quester_and_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_and_task_dynamic_data *data);
void quester_and_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_and_task_dynamic_data *data);

struct quester_or_task_dynamic_data
{
    struct quester_immediate_dependency_tracker dependencies;
    bool has_succeeded_dependency;
};

struct quester_activation_result quester_or_task_activator(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_or_task_dynamic_data *data,
    struct quester_connection *triggering_connection);
struct quester_activation_result quester_or_task_non_activator(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_or_task_dynamic_data *data,
    struct quester_connection *triggering_connection);
void quester_or_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_or_task_dynamic_data *data);
struct quester_tick_result quester_or_task_tick(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_or_task_dynamic_data *data);
void quester_or_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings,
    struct quester_or_task_dynamic_data *data);

struct quester_placeholder_static_data
{
    char description[128];
};

struct quester_placeholder_dynamic_data
{
    struct quester_tick_result tick_result;
};

struct quester_activation_result quester_placeholder_activator(struct quester_context *ctx, int id,
    struct quester_placeholder_static_data *static_data,
    struct quester_placeholder_dynamic_data *dynamic_data,
    struct quester_connection *triggering_connection);
struct quester_activation_result quester_placeholder_non_activator(struct quester_context *ctx, int id,
    struct quester_placeholder_static_data *static_data,
    struct quester_placeholder_dynamic_data *dynamic_data,
    struct quester_connection *triggering_connection);
struct quester_tick_result quester_placeholder_tick(struct quester_context *ctx, int id,
    struct quester_placeholder_static_data *static_data,
    struct quester_placeholder_dynamic_data *dynamic_data);
void quester_placeholder_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_placeholder_static_data *static_data,
    struct quester_placeholder_dynamic_data *dynamic_data);
void quester_placeholder_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_placeholder_static_data *static_data,
    struct quester_placeholder_dynamic_data *data);

struct quester_activation_result quester_in_bridge_activator(struct quester_context *ctx, int id,
    enum quester_in_connection_type *static_data, void *dynamic_data,
    struct quester_connection *triggering_connection);
struct quester_activation_result quester_in_bridge_non_activator(struct quester_context *ctx, int id,
    enum quester_in_connection_type *static_data, void *dynamic_data,
    struct quester_connection *triggering_connection);
struct quester_tick_result quester_in_bridge_tick(struct quester_context *ctx, int id,
    enum quester_in_connection_type *static_data, void *dynamic_data);
void quester_in_bridge_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    enum quester_in_connection_type *static_data, void *dynamic_data);
void quester_in_bridge_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, enum quester_in_connection_type *static_data, void *data);

struct quester_out_bridge_data
{
    enum quester_out_connection_type represents_container_output;
};

struct quester_activation_result quester_out_bridge_activator(struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data,
    struct quester_connection *triggering_connection);
struct quester_activation_result quester_out_bridge_non_activator(struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data,
    struct quester_connection *triggering_connection);
struct quester_tick_result quester_out_bridge_tick(struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data);
void quester_out_bridge_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data);
void quester_out_bridge_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_out_bridge_data *static_data, void *data);

struct quester_activation_result quester_entrypoint_activator(struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data, struct quester_connection *triggering_connection);
struct quester_activation_result quester_entrypoint_non_activator(struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data, struct quester_connection *triggering_connection);
struct quester_tick_result quester_entrypoint_tick(struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data);
void quester_entrypoint_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data);
void quester_entrypoint_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, void *static_data, void *data);

#define QUESTER_IMPLEMENT_BUILTIN_NODES                                                            \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_CONTAINER_TASK,                                         \
        struct quester_container_task_data, void, quester_container_task_activator,                \
        quester_container_task_non_activator, quester_container_task_tick,                         \
        quester_container_task_display, quester_container_task_prop_edit_display)                  \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_PLACEHOLDER_TASK,                                       \
        struct quester_placeholder_static_data, struct quester_placeholder_dynamic_data,           \
        quester_placeholder_activator, quester_placeholder_non_activator, quester_placeholder_tick, quester_placeholder_display,      \
        quester_placeholder_prop_edit_display)                                                     \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_AND_TASK, struct quester_immediate_dependency_tracker_completion_settings, struct quester_and_task_dynamic_data,   \
        quester_and_task_activator, quester_and_task_non_activator, quester_and_task_tick, quester_and_task_display,               \
        quester_and_task_prop_edit_display)                                                        \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_OR_TASK, struct quester_immediate_dependency_tracker_completion_settings,                   \
        struct quester_or_task_dynamic_data, quester_or_task_activator, quester_or_task_non_activator, quester_or_task_tick,      \
        quester_or_task_display, quester_or_task_prop_edit_display)                                \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_IN_BRIDGE_TASK, enum quester_in_connection_type,        \
        void, quester_in_bridge_activator, quester_in_bridge_non_activator, quester_in_bridge_tick,                                 \
        quester_in_bridge_display, quester_in_bridge_prop_edit_display)                            \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_OUT_BRIDGE_TASK, struct quester_out_bridge_data,        \
        void, quester_out_bridge_activator, quester_out_bridge_non_activator, quester_out_bridge_tick,                               \
        quester_out_bridge_display, quester_out_bridge_prop_edit_display)                          \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_ENTRYPOINT_TASK, void,        \
        void, quester_entrypoint_activator, quester_entrypoint_non_activator, quester_entrypoint_tick,                               \
        quester_entrypoint_display, quester_entrypoint_prop_edit_display)