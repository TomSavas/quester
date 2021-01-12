enum quester_tick_result_flags
{
    QUESTER_STILL_RUNNING                                 = 0x1,
    QUESTER_FORWARD_TICK_RESULT_TO_IDS                    = 0x2,
    // TODO: implement ticking disabling from the tick result
    //QUESTER_DISABLE_TICKING                               = 0x4,
    //QUESTER_FORWARD_TICK_RESULT_TO_ACTIVE_CONTAINER_NODES = 0x8,
};

struct quester_tick_result
{
    enum quester_tick_result_flags flags;
    enum quester_out_connection_type out_connection_to_trigger;

    enum quester_tick_result_flags flags_to_forward;
    enum quester_out_connection_type out_connections_to_forward_trigger;
    int id_count;
    int ids[64];
};

enum quester_activation_flags
{
    QUESTER_ACTIVATE                   = 0x1,
    QUESTER_FORWARD_CONNECTIONS_TO_IDS = 0x2,
    // TODO: finish implementing
    QUESTER_DISABLE_TICKING            = 0x4,
    QUESTER_ALLOW_REPEATED_ACTIVATIONS = 0x8,
};

struct quester_activation_result
{
    enum quester_activation_flags flags;

    int id_count;
    int ids[64];
};

struct quester_context;
typedef struct quester_tick_result (*quester_tick_func)(struct quester_context* /*ctx*/, int /*id*/,
    void* /*static_node_data*/, void* /*tracking_node_data*/);

typedef struct quester_activation_result (*quester_activation_func)(struct quester_context* /*ctx*/,
    int /*id*/, void* /*static_node_data*/, void* /*tracking_node_data*/, 
    struct quester_connection* /*triggering_connection*/);

// TODO: make API independent
typedef void (*quester_display_func)(struct nk_context* /*nk_ctx*/, struct quester_context* /*ctx*/,
    int /*id*/, void* /*static_node_data*/, void* /*tracking_node_data*/);
typedef void (*quester_prop_edit_display_func)(struct nk_context* /*nk_ctx*/,
    struct quester_context* /*ctx*/, int /*id*/, void* /*static_node_data*/,
    void* /*tracking_node_data*/);

typedef int (*quester_get_size)();

struct quester_node_implementation
{
    const char *name;

    // Called whenever a node that is connected to this node activates any connection that connects
    // the two.
    const quester_activation_func activator;
    // Called whenever a node that is connected to this node activates any other connection
    // than the one(s) that connects the two.
    const quester_activation_func non_activator;

    const quester_tick_func tick;

    const quester_display_func nk_display;
    const quester_prop_edit_display_func nk_prop_edit_display;

    const quester_get_size static_data_size;
    const quester_get_size tracking_data_size;
};

/*
 * Defines a bunch of function wrappers that are "type correct" - they do type casts to avoid
 * compiler errors. 
 * You _HAVE_ to use either this or QUESTER_AUTO_IMPLEMENT_NODE(recommended) macros to register your
 * type with the quester system.
 */
#define QUESTER_IMPLEMENT_NODE(node_type_enum, static_node_data_struct, tracking_node_data_struct, \
        activator_func, non_activator_func, tick_func, nk_display_func, nk_prop_edit_display_func) \
                                                                                                   \
    struct quester_tick_result node_type_enum##_tick_typecorrect_wrapper(                          \
        struct quester_context *ctx, int id, void *static_node_data, void *tracking_node_data)     \
    {                                                                                              \
        return tick_func(ctx, id, (static_node_data_struct*)static_node_data,                      \
            (tracking_node_data_struct*)tracking_node_data);                                       \
    }                                                                                              \
                                                                                                   \
    struct quester_activation_result node_type_enum##_activator_typecorrect_wrapper(               \
        struct quester_context *ctx, int id,                                                       \
        void *static_node_data, void *tracking_node_data,                                          \
        struct quester_connection *triggering_connection)                                          \
    {                                                                                              \
        return activator_func(ctx, id, (static_node_data_struct*)static_node_data,                 \
            (tracking_node_data_struct*)tracking_node_data, triggering_connection);                \
    }                                                                                              \
                                                                                                   \
    struct quester_activation_result node_type_enum##_non_activator_typecorrect_wrapper(           \
        struct quester_context *ctx, int id, void *static_node_data, void *tracking_node_data,     \
        struct quester_connection *triggering_connection)                                          \
    {                                                                                              \
        return non_activator_func(ctx, id, (static_node_data_struct*)static_node_data,             \
            (tracking_node_data_struct*)tracking_node_data, triggering_connection);                \
    }                                                                                              \
                                                                                                   \
    void node_type_enum##_nk_display_typecorrect_wrapper(struct nk_context *nk_ctx,                \
        struct quester_context *ctx, int id, void *static_node_data, void *tracking_node_data)     \
    {                                                                                              \
        nk_display_func(nk_ctx, ctx, id, (static_node_data_struct*)static_node_data,               \
            (tracking_node_data_struct*)tracking_node_data);                                       \
    }                                                                                              \
                                                                                                   \
    void node_type_enum##_nk_prop_edit_display_typecorrect_wrapper(struct nk_context *nk_ctx,      \
        struct quester_context *ctx, int id, void *static_node_data, void *tracking_node_data)     \
    {                                                                                              \
        nk_prop_edit_display_func(nk_ctx, ctx, id, (static_node_data_struct*)static_node_data,     \
            (tracking_node_data_struct*)tracking_node_data);                                       \
    }                                                                                              \
                                                                                                   \
    int static_data_size_##node_type_enum() { return sizeof(static_node_data_struct); }            \
    int tracking_data_size_##node_type_enum() { return sizeof(tracking_node_data_struct); }

/*
 * Saves you the typing that you'd have to do with QUESTER_IMPLEMENT_NODE, but poses a constraint -
 * your functions have all to be of format <prefix>_<quester_function_suffix>, e.g. if your 
 * task is called some_test_task, then your functions should have names like:
 * some_test_task_activator, some_test_task_non_activator, some_test_task_tick, etc.
 */
#define QUESTER_AUTO_IMPLEMENT_NODE(node_type_enum, prefix)                                        \
    QUESTER_IMPLEMENT_NODE(node_type_enum, prefix##_definition, prefix##_data, prefix##_activator, \
    prefix##_non_activator, prefix##_tick, prefix##_display, prefix##_prop_edit)

#define QUESTER_NODE_IMPLEMENTATION(node_type_enum)                                                \
    {                                                                                              \
        .name                 = #node_type_enum,                                                   \
        .activator            = node_type_enum##_activator_typecorrect_wrapper,                    \
        .non_activator        = node_type_enum##_non_activator_typecorrect_wrapper,                \
        .tick                 = node_type_enum##_tick_typecorrect_wrapper,                         \
        .nk_display           = node_type_enum##_nk_display_typecorrect_wrapper,                   \
        .nk_prop_edit_display = node_type_enum##_nk_prop_edit_display_typecorrect_wrapper,         \
        .static_data_size     = static_data_size_##node_type_enum,                                 \
        .tracking_data_size   = tracking_data_size_##node_type_enum                                \
    }                                                                                              