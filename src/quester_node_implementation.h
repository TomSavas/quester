// Such values have been chosen to enable returning a bool
// from user tick functions if they don't need "failed" behaviour
enum quester_tick_result
{
    QUESTER_FAILED = -1,
    QUESTER_RUNNING = 0,
    QUESTER_COMPLETED = 1
};

struct quester_context;
typedef enum quester_tick_result (*quester_tick_func)(struct quester_context* /*ctx*/, int /*id*/,
            void* /*static_node_data*/, void* /*tracking_node_data*/);

typedef void (*quester_on_start_func)(struct quester_context* /*ctx*/, int /*id*/,
        void* /*static_node_data*/, void* /*tracking_node_data*/, int /*started_from_id*/);

typedef void (*quester_display_func)(struct nk_context* /*nk_ctx*/, struct quester_context* /*ctx*/,
        int /*id*/, void* /*static_node_data*/, void* /*tracking_node_data*/);
typedef void (*quester_prop_edit_display_func)(struct nk_context* /*nk_ctx*/,
        struct quester_context* /*ctx*/, int /*id*/, void* /*static_node_data*/,
        void* /*tracking_node_data*/);

struct quester_node_implementation
{
    const char name[256];

    quester_tick_func tick;

    quester_on_start_func on_start;
    //quester_on_completion_func on_completion;
    //quester_on_failure_func on_failure;

    quester_display_func nk_display;
    quester_prop_edit_display_func nk_prop_edit_display;

    const int static_data_size;
    const int tracking_data_size;
};

#define QUESTER_IMPLEMENT_NODE(node_type_enum, static_node_data_struct, tracking_node_data_struct, \
        on_start_func, tick_func, nk_display_func, nk_prop_edit_display_func)                      \
                                                                                                   \
    enum quester_tick_result node_type_enum##_tick_typecorrect_wrapper(struct quester_context *ctx,\
            int id, void *static_node_data, void *tracking_node_data)                              \
    {                                                                                              \
        return tick_func(ctx, id, (static_node_data_struct*)static_node_data,                      \
                (tracking_node_data_struct*)tracking_node_data);                                   \
    }                                                                                              \
                                                                                                   \
    void node_type_enum##_on_start_typecorrect_wrapper(struct quester_context *ctx, int id,        \
            void *static_node_data, void *tracking_node_data, int started_from_id)                 \
    {                                                                                              \
        on_start_func(ctx, id, (static_node_data_struct*)static_node_data,                         \
                (tracking_node_data_struct*)tracking_node_data, started_from_id);                  \
    }                                                                                              \
                                                                                                   \
    void node_type_enum##_nk_display_typecorrect_wrapper(struct nk_context *nk_ctx,                \
            struct quester_context *ctx, int id, void *static_node_data, void *tracking_node_data) \
    {                                                                                              \
        nk_display_func(nk_ctx, ctx, id, (static_node_data_struct*)static_node_data,               \
                (tracking_node_data_struct*)tracking_node_data);                                   \
    }                                                                                              \
                                                                                                   \
    void node_type_enum##_nk_prop_edit_display_typecorrect_wrapper(struct nk_context *nk_ctx,      \
            struct quester_context *ctx, int id, void *static_node_data, void *tracking_node_data) \
    {                                                                                              \
        nk_prop_edit_display_func(nk_ctx, ctx, id, (static_node_data_struct*)static_node_data,     \
                (tracking_node_data_struct*)tracking_node_data);                                   \
    }                                                                                              \
                                                                                                   \
    const int static_data_size_##node_type_enum = sizeof(static_node_data_struct);                 \
    const int tracking_data_size_##node_type_enum = sizeof(tracking_node_data_struct);             

// NOTE: possible to reduce .static_data_size / .tracking_data_size to true compile-time constants?
// Yeah okay, here's the solution to try in the future: 
    //int static_data_size_##node_type_enum() { return sizeof(static_node_data_struct); }          \
    //int tracking_data_size_##node_type_enum() { return sizeof(tracking_node_data_struct); }      \
// and then just call those functions when we need the size and pray that the compiler is smart
// enough to optimize out those function calls into.
// Now that I think though it probably won't be able to do that because it will access the function
// through a pointer... Fuck me, no tcc for now I suppose.

#define QUESTER_NODE_IMPLEMENTATION(node_type_enum)                                                \
    {                                                                                              \
        .name                 = #node_type_enum,                                                   \
        .tick                 = node_type_enum##_tick_typecorrect_wrapper,                         \
        .on_start             = node_type_enum##_on_start_typecorrect_wrapper,                     \
        .nk_display           = node_type_enum##_nk_display_typecorrect_wrapper,                   \
        .nk_prop_edit_display = node_type_enum##_nk_prop_edit_display_typecorrect_wrapper,         \
        .static_data_size     = static_data_size_##node_type_enum,                                 \
        .tracking_data_size   = tracking_data_size_##node_type_enum                                \
    }                                                                                              
