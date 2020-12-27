enum quester_or_behaviour
{
    DO_NOTHING = 0,
    COMPLETE_INCOMING_INCOMPLETE_TASKS,
    FAIL_INCOMING_INCOMPLETE_TASKS,
    KILL_INCOMING_INCOMPLETE_TASKS,
};

struct quester_or_task_data
{
    enum quester_or_behaviour behaviour;
    bool only_once;
};

struct quester_or_task_dynamic_data
{
    int activated_by_id;
};

void quester_or_task_on_start(struct quester_context *ctx, int id,
        struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data,
        int started_from_id);
void quester_or_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data);
enum quester_tick_result quester_or_task_tick(struct quester_context *ctx, int id,
        struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data);
void quester_or_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, struct quester_or_task_data *static_node_data,
        struct quester_or_task_dynamic_data *data);

struct quester_and_task_dynamic_data
{
    // TODO: dynamic maybe? or a smarter solution altogether
    int completed_dependency_count;
    int completed_dependencies[1024];
    bool all_dependencies_completed;
};

void quester_and_task_on_start(struct quester_context *ctx, int id, void *_,
        struct quester_and_task_dynamic_data *data, int started_from_id);
enum quester_tick_result quester_and_task_tick(struct quester_context *ctx, int id, void *_,
        struct quester_and_task_dynamic_data *data);
void quester_and_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        void *_, struct quester_and_task_dynamic_data *data);
void quester_and_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, void *_, struct quester_and_task_dynamic_data *data);

struct quester_placeholder_static_data
{
    char description[8192];
};

struct quester_placeholder_dynamic_data
{
    enum quester_tick_result tick_result;
};

void quester_placeholder_on_start(struct quester_context *ctx, int id,
        struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *dynamic_data, int started_from_id);
enum quester_tick_result quester_placeholder_tick(struct quester_context *ctx, int id,
        struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *dynamic_data);
void quester_placeholder_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *dynamic_data);
void quester_placeholder_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *data);

#define QUESTER_IMPLEMENT_BUILTIN_NODES                                                            \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_PLACEHOLDER_TASK,                                       \
            struct quester_placeholder_static_data, struct quester_placeholder_dynamic_data,       \
            quester_placeholder_on_start, quester_placeholder_tick, quester_placeholder_display,   \
            quester_placeholder_prop_edit_display)                                                 \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_AND_TASK, void, struct quester_and_task_dynamic_data,   \
            quester_and_task_on_start, quester_and_task_tick, quester_and_task_display,            \
            quester_and_task_prop_edit_display)                                                    \
    QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_OR_TASK, struct quester_or_task_data,                   \
            struct quester_or_task_dynamic_data, quester_or_task_on_start, quester_or_task_tick,   \
            quester_or_task_display, quester_or_task_prop_edit_display)
