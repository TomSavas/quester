#include <stdbool.h>

struct quester_context;

struct quester_context *quester_init(int capacity);
void quester_free(struct quester_context *ctx);

// saving/loading
void quester_dump_static_bin(struct quester_context *ctx, const char *dir_path, const char *filename);
// Clears dynamic state
void quester_load_static_bin(struct quester_context **ctx, const char *dir_path, const char *filename);

void quester_dump_dynamic_bin(struct quester_context *ctx, const char *dir_path, const char *filename);
void quester_load_dynamic_bin(struct quester_context **ctx, const char *dir_path, const char *filename);

// editing
union quester_node *quester_add_node(struct quester_context *ctx);
void quester_add_connection(struct quester_context *ctx, int from_node_id, int to_node_id);

void quester_reset_dynamic_state(struct quester_context *ctx);

// running
void quester_run(struct quester_context *ctx);

// Such values have been chosen to enable returning a bool
// from user tick functions if they don't need "failed" behaviour
enum quester_tick_result
{
    QUESTER_FAILED = -1,
    QUESTER_RUNNING = 0,
    QUESTER_COMPLETED = 1
};

struct quester_node_implementation
{
    const char name[256];
    enum quester_tick_result (*tick)(struct quester_context* /*ctx*/, int /*id*/, void* /*static_node_data*/, void* /*tracking_node_data*/);

    void (*on_start)(struct quester_context* /*ctx*/, int /*id*/, void* /*static_node_data*/, void* /*tracking_node_data*/, int /*started_from_id*/);
    //void (*on_completion)(void* /*static_node_data*/, void* /*tracking_node_data*/);
    //void (*on_failure)(void* /*static_node_data*/, void* /*tracking_node_data*/);

    void (*nk_display)(struct nk_context* /*nk_ctx*/, struct quester_context* /*ctx*/, int /*id*/, void* /*static_node_data*/, void* /*tracking_node_data*/);
    void (*nk_prop_edit_display)(struct nk_context* /*nk_ctx*/, struct quester_context* /*ctx*/, int /*id*/, void* /*static_node_data*/, void* /*tracking_node_data*/);

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
            struct nk_context *ctx, int id, void *static_node_data, void *tracking_node_data)      \
    {                                                                                              \
        nk_display_func(nk_ctx, ctx, id, (static_node_data_struct*)static_node_data,               \
                (tracking_node_data_struct*)tracking_node_data);                                   \
    }                                                                                              \
                                                                                                   \
    void node_type_enum##_nk_prop_edit_display_typecorrect_wrapper(struct nk_context *nk_ctx,      \
            struct nk_context *ctx, int id, void *static_node_data, void *tracking_node_data)      \
    {                                                                                              \
        nk_prop_edit_display_func(nk_ctx, ctx, id, (static_node_data_struct*)static_node_data,     \
                (tracking_node_data_struct*)tracking_node_data);                                   \
    }                                                                                              \
                                                                                                   \
    const int static_data_size_##node_type_enum = sizeof(static_node_data_struct);                 \
    const int tracking_data_size_##node_type_enum = sizeof(tracking_node_data_struct);             \

// NOTE: possible to reduce .static_data_size / .tracking_data_size to true compile-time constants?
#define QUESTER_NODE_IMPLEMENTATION(node_type_enum)                                                \
    {                                                                                              \
        .name                 = #node_type_enum,                                                   \
        .tick                 = node_type_enum##_tick_typecorrect_wrapper,                         \
        .on_start             = node_type_enum##_on_start_typecorrect_wrapper,                     \
        .nk_display           = node_type_enum##_nk_display_typecorrect_wrapper,                   \
        .nk_prop_edit_display = node_type_enum##_nk_prop_edit_display_typecorrect_wrapper,         \
        .static_data_size     = static_data_size_##node_type_enum,                                 \
        .tracking_data_size   = tracking_data_size_##node_type_enum                                \
    }                                                                                              \

#include "temp_tasks.h"

enum quester_node_type
{
    // builtin tasks
    QUESTER_BUILTIN_AND_TASK = 0,
    QUESTER_BUILTIN_OR_TASK,
    QUESTER_BUILTIN_PLACEHOLDER_TASK,
    QUESTER_BUILTIN_ENTRYPOINT_TASK,

    // user-defined tasks
    TIMER_TASK,
    TEST_TASK,

    QUESTER_NODE_TYPE_COUNT
};

struct node 
{
    enum quester_node_type type;
    int id;

    char mission_id[32];
    char name[128];

    int in_node_count;
    int in_node_ids[100];
    int out_node_count;
    int out_node_ids[100];
};

struct editor_node
{
    struct node node;
    struct nk_rect bounds;
};

union quester_node
{
    struct editor_node editor_node;  
    struct node node;
};

// TODO: needs a better name 
// Contains quest/node definitions + the initial state
struct quester_static_state
{
    size_t size;
    int capacity;

    int initially_tracked_node_count;
    int *initially_tracked_node_ids;

    int node_count;
    union quester_node *all_nodes;
    void *static_node_data;
};

enum quester_control_flags
{
    QUESTER_CTL_DISABLE_ACTIVATION = 1
};

// TODO: needs a better name
// Contains tracking data for nodes. This would be saved along with 
// the player savefile. 
//
// Potentially could save all of the completed task state as well for 
// debugging purposes, maybe controlled with something like
// QUESTER_DEBUG_KEEP_COMPLETED_DYNAMIC_STATE ?
struct quester_dynamic_state
{
    size_t size;
    int capacity;

    int tracked_node_count;
    int *tracked_node_ids;

    // debug/editor only
    int failed_node_count;
    int *failed_node_ids;

    int completed_node_count;
    int *completed_node_ids;
    // debug/editor only

    int node_count;
    enum quester_control_flags *ctl_flags;
    void *tracked_node_data;
};

struct quester_context
{
    size_t size;
    bool execution_paused;

    struct quester_static_state *static_state;
    struct quester_dynamic_state *dynamic_state;
};

int quester_find_index(struct quester_context *ctx, int node_id);
int quester_max_static_data_size();
int quester_max_dynamic_data_size();
int quester_find_static_data_offset(struct quester_context *ctx, int node_id);
int quester_find_dynamic_data_offset(struct quester_context *ctx, int node_id);
void quester_fill_with_test_data(struct quester_context *ctx);

void quester_empty() {}
enum quester_tick_result quester_empty_tick() { return 0; }

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

bool quester_is_completed(struct quester_context *ctx, int id)
{
    for (int i = 0; i < ctx->dynamic_state->completed_node_count; i++)
        if (ctx->dynamic_state->completed_node_ids[i] == id)
            return true;

    return false;
}

void quester_complete_task(struct quester_context *ctx, int id);

void quester_or_task_on_start(struct quester_context *ctx, int id, struct quester_or_task_data *static_node_data,
        struct quester_or_task_dynamic_data *data, int started_from_id)
{
    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    switch(static_node_data->behaviour)
    {
        case COMPLETE_INCOMING_INCOMPLETE_TASKS:
            for (int j = 0; j < node->in_node_count; j++)
            {
                int in_id = node->in_node_ids[j];
                struct node *in_node = &ctx->static_state->all_nodes[quester_find_index(ctx, in_id)].node;

                // TEMPORARY
                quester_complete_task(ctx, in_id);

                // TODO: factor out tracked node removal
                for (int k = 0; k < ctx->dynamic_state->tracked_node_count; k++)
                    if (ctx->dynamic_state->tracked_node_ids[k] == in_id)
                    {
                        ctx->dynamic_state->tracked_node_ids[k] = ctx->dynamic_state->tracked_node_ids[--ctx->dynamic_state->tracked_node_count];
                        break;
                    }
            }
            break;
        case FAIL_INCOMING_INCOMPLETE_TASKS:
        //TODO
        case KILL_INCOMING_INCOMPLETE_TASKS:
            for (int j = 0; j < node->in_node_count; j++)
            {
                // TODO: factor out tracked node removal

                int in_id = node->in_node_ids[j];
                for (int k = 0; k < ctx->dynamic_state->tracked_node_count; k++)
                    if (ctx->dynamic_state->tracked_node_ids[k] == in_id)
                    {
                        ctx->dynamic_state->tracked_node_ids[k] = ctx->dynamic_state->tracked_node_ids[--ctx->dynamic_state->tracked_node_count];
                        break;
                    }
            }
            break;
        case DO_NOTHING:
        default:
            break;
    }

    if (static_node_data->only_once)
        ctx->dynamic_state->ctl_flags[id] |= QUESTER_CTL_DISABLE_ACTIVATION;
}

void quester_or_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id, struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data)
{
    char *actions[] =  { "do nothing", "complete", "fail", "kill" };

    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "Incoming incomplete task action:", NK_TEXT_LEFT);
    static_node_data->behaviour = nk_combo(nk_ctx, actions, 4, static_node_data->behaviour, 25, nk_vec2(200, 200));
    // NOTE: this shit is utterly stupid, the checkbox is inverted
    nk_checkbox_label(nk_ctx, "Activate only once", &static_node_data->only_once);
}

enum quester_tick_result quester_or_task_tick(struct quester_context *ctx, int id, struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data)
{
    return QUESTER_COMPLETED;
}

void quester_or_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id, struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data)
{
}

QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_OR_TASK, struct quester_or_task_data, struct quester_or_task_dynamic_data, quester_or_task_on_start, quester_or_task_tick, quester_or_task_display, quester_or_task_prop_edit_display)

struct quester_and_task_dynamic_data
{
    // TODO: dynamic maybe? or a smarter solution altogether
    int completed_dependency_count;
    int completed_dependencies[1024];
    bool all_dependencies_completed;
};

void quester_and_task_on_start(struct quester_context *ctx, int id, void *_,
        struct quester_and_task_dynamic_data *data, int started_from_id)
{
    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    bool duplicate = false;
    for (int i = 0; i < data->completed_dependency_count; i++)
    {
        if (data->completed_dependencies[i] == started_from_id)
            duplicate = true;
    }

    if (!duplicate)
    {
        data->completed_dependencies[data->completed_dependency_count++] = started_from_id;
        printf("completed: %d\n", started_from_id);
    }
    else
    {
        printf("duplicate: %d\n", started_from_id);
    }
    
    data->all_dependencies_completed = data->completed_dependency_count == node->in_node_count;
}

enum quester_tick_result quester_and_task_tick(struct quester_context *ctx, int id, void *_, struct quester_and_task_dynamic_data *data)
{
    return data->all_dependencies_completed;
}

void quester_and_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id, void *_, struct quester_and_task_dynamic_data *data)
{
}

void quester_and_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id, void *_, struct quester_and_task_dynamic_data *data)
{
}

QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_AND_TASK, void, struct quester_and_task_dynamic_data, quester_and_task_on_start, quester_and_task_tick, quester_and_task_display, quester_and_task_prop_edit_display) 

struct quester_placeholder_static_data
{
    char description[8192];
};

struct quester_placeholder_dynamic_data
{
    enum quester_tick_result tick_result;
};

void quester_placeholder_on_start(struct quester_context *ctx, int id, struct quester_placeholder_static_data *static_data, struct quester_placeholder_dynamic_data *dynamic_data, int started_from_id)
{
    dynamic_data->tick_result = QUESTER_RUNNING;
}

enum quester_tick_result quester_placeholder_tick(struct quester_context *ctx, int id, struct quester_placeholder_static_data *static_data, struct quester_placeholder_dynamic_data *dynamic_data)
{
    return dynamic_data->tick_result;
}

void quester_placeholder_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id, struct quester_placeholder_static_data *static_data, struct quester_placeholder_dynamic_data *dynamic_data)
{
    nk_layout_row_dynamic(nk_ctx, 25, 2);
    if (nk_button_label(nk_ctx, "Manual complete"))
    {
        dynamic_data->tick_result = QUESTER_COMPLETED;
    }

    if (nk_button_label(nk_ctx, "Manual failure"))
    {
        dynamic_data->tick_result = QUESTER_FAILED;
    }

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "DESCRIPTION", NK_TEXT_CENTERED);
    nk_layout_row_dynamic(nk_ctx, 1000, 1);
    // NOTE: instead of using nk_text_wrap we use this, to enable newlines and stuff
    nk_edit_string_zero_terminated(nk_ctx, NK_EDIT_BOX | NK_EDIT_READ_ONLY, static_data->description, sizeof(static_data->description), nk_filter_default);
}

void quester_placeholder_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        struct quester_placeholder_static_data *static_data, struct quester_placeholder_dynamic_data *data)
{
    nk_layout_row_dynamic(nk_ctx, 100, 1);
    nk_edit_string_zero_terminated(nk_ctx, NK_EDIT_BOX, static_data->description, sizeof(static_data->description), nk_filter_default);
}

QUESTER_IMPLEMENT_NODE(QUESTER_BUILTIN_PLACEHOLDER_TASK, struct quester_placeholder_static_data,
        struct quester_placeholder_dynamic_data, quester_placeholder_on_start, quester_placeholder_tick,
        quester_placeholder_display, quester_placeholder_prop_edit_display)

struct quester_node_implementation quester_node_implementations[QUESTER_NODE_TYPE_COUNT] =
{
    //{"AND_TASK", NULL, NULL, NULL, 0, 0}, //QUESTER_BUILTIN_AND_TASK, not implemented yet
    //{"OR_TASK", NULL, NULL, quester_or_task_display, 0, 0}, //QUESTER_BUILTIN_OR_TASK, not implemented yet
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_AND_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_OR_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_PLACEHOLDER_TASK),
    {"ENTRYPOINT_TASK", quester_empty_tick, quester_empty, quester_empty, quester_empty, 0, 0}, //QUESTER_BUILTIN_OR_TASK, not implemented yet
    //QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_ENTRYPOINT_TASK),
    QUESTER_NODE_IMPLEMENTATION(TIMER_TASK),
    QUESTER_NODE_IMPLEMENTATION(TEST_TASK)
};

void quester_complete_task(struct quester_context *ctx, int id)
{
    //struct node *node = &ctx->static_state->all_nodes[quester_find_index(ctx, id)].node;
    //void *static_node_data = ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, id);
    //void *dynamic_node_data = ctx->dynamic_state->tracked_node_data + quester_find_dynamic_data_offset(ctx, id);

    //quester_node_implementations[node->type].on_completion(static_node_data, dynamic_node_data);
}

#define QUESTER_USE_LIBC
#ifdef QUESTER_USE_LIBC
#include <stdio.h>
struct quester_context *quester_init(int capacity)
{
    size_t static_state_size = sizeof(struct quester_static_state) + 
        (sizeof(int) + sizeof(union quester_node) + quester_max_static_data_size()) * capacity;
    size_t dynamic_state_size = sizeof(struct quester_dynamic_state) +
        (sizeof(int) + sizeof(int) + sizeof(int) + sizeof(enum quester_control_flags) + quester_max_dynamic_data_size()) * capacity;

    size_t total_ctx_size = sizeof(struct quester_context) + static_state_size + dynamic_state_size;
    struct quester_context *ctx = malloc(total_ctx_size);
    memset(ctx, 0, total_ctx_size);
    ctx->size = total_ctx_size;

    ctx->execution_paused = false;

    ctx->static_state = (void*)ctx + sizeof(struct quester_context);
    ctx->static_state->initially_tracked_node_ids = (void*)ctx->static_state + sizeof(struct quester_static_state);
    ctx->static_state->all_nodes = (void*)ctx->static_state->initially_tracked_node_ids + sizeof(int) * capacity;
    ctx->static_state->static_node_data = (void*)ctx->static_state->all_nodes + sizeof(union quester_node) * capacity;
    ctx->static_state->size = static_state_size;
    ctx->static_state->capacity = capacity;
    ctx->static_state->initially_tracked_node_count = 0;
    ctx->static_state->node_count = 0;

    ctx->dynamic_state = (void*)ctx->static_state + static_state_size;
    ctx->dynamic_state->failed_node_ids = (void*)ctx->dynamic_state + sizeof(struct quester_dynamic_state);
    ctx->dynamic_state->completed_node_ids = (void*)ctx->dynamic_state->failed_node_ids + sizeof(int) * capacity;
    ctx->dynamic_state->ctl_flags = (void*)ctx->dynamic_state->completed_node_ids + sizeof(int) * capacity;
    ctx->dynamic_state->tracked_node_ids = (void*)ctx->dynamic_state->ctl_flags + sizeof(enum quester_control_flags) * capacity;
    ctx->dynamic_state->tracked_node_data = (void*)ctx->dynamic_state->tracked_node_ids + sizeof(int) * capacity;
    ctx->dynamic_state->size = dynamic_state_size;
    ctx->dynamic_state->capacity = capacity;
    ctx->dynamic_state->failed_node_count = 0;
    ctx->dynamic_state->completed_node_count = 0;
    ctx->dynamic_state->node_count = 0;

    quester_fill_with_test_data(ctx);

    return ctx;
}

void quester_free(struct quester_context *ctx)
{
    free(ctx);
}

void quester_dump_static_bin(struct quester_context *ctx, const char *dir_path, const char *filename)
{
    char *filepath = malloc(sizeof(char) * (strlen(dir_path) + strlen(filename) + 6));
    filepath[0] = '\0';
    strcat(filepath, dir_path);
    strcat(filepath, filename);
    strcat(filepath, ".s_bin");

    FILE *f = fopen(filepath, "w");
    fwrite(ctx->static_state, ctx->static_state->size, 1, f);
    fclose(f);

    free(filepath);
}

void quester_load_static_bin(struct quester_context **ctx, const char *dir_path, const char *filename)
{
    char *filepath = malloc(sizeof(char) * (strlen(dir_path) + strlen(filename) + 6));
    filepath[0] = '\0';
    strcat(filepath, dir_path);
    strcat(filepath, filename);
    strcat(filepath, ".s_bin");

    FILE *f = fopen(filepath, "r");

    size_t static_state_size;
    fread(&static_state_size, sizeof(size_t), 1, f);

    if ((*ctx)->static_state->size < static_state_size)
    {
        size_t new_size = sizeof(struct quester_context) + static_state_size + (*ctx)->dynamic_state->size;
        *ctx = realloc(*ctx, new_size);

        // move dynamic_state to give more space to static_state
        struct quester_dynamic_state *new_dynamic_state = (void*)*ctx + sizeof(struct quester_context) + static_state_size;
        memmove(new_dynamic_state, (*ctx)->dynamic_state, (*ctx)->dynamic_state->size);
        (*ctx)->dynamic_state = new_dynamic_state;
    }
    struct quester_static_state *static_state = (*ctx)->static_state;
    memset(static_state, 0, static_state_size);
    static_state->size = static_state_size;

    fread((void*)static_state + sizeof(size_t), static_state_size - sizeof(size_t), 1, f);

    static_state->initially_tracked_node_ids = (void*)static_state + sizeof(struct quester_static_state); 
    static_state->all_nodes = (void*)static_state->initially_tracked_node_ids + sizeof(int) * static_state->capacity;
    static_state->static_node_data = (void*)static_state->all_nodes + sizeof(union quester_node) * static_state->capacity;

    quester_reset_dynamic_state(*ctx);
}

void quester_dump_dynamic_bin(struct quester_context *ctx, const char *dir_path, const char *filename)
{
    char *filepath = malloc(sizeof(char) * (strlen(dir_path) + strlen(filename) + 6));
    filepath[0] = '\0';
    strcat(filepath, dir_path);
    strcat(filepath, filename);
    strcat(filepath, ".d_bin");

    FILE *f = fopen(filepath, "w");
    fwrite(ctx->dynamic_state, ctx->dynamic_state->size, 1, f);
    fclose(f);

    free(filepath);
}

void quester_load_dynamic_bin(struct quester_context **ctx, const char *dir_path, const char *filename)
{
    char *filepath = malloc(sizeof(char) * (strlen(dir_path) + strlen(filename) + 6));
    filepath[0] = '\0';
    strcat(filepath, dir_path);
    strcat(filepath, filename);
    strcat(filepath, ".d_bin");

    FILE *f = fopen(filepath, "r");

    size_t dynamic_state_size;
    fread(&dynamic_state_size, sizeof(size_t), 1, f);

    if ((*ctx)->dynamic_state->size < dynamic_state_size)
    {
        size_t new_size = sizeof(struct quester_context) + (*ctx)->static_state->size + dynamic_state_size;
        *ctx = realloc(*ctx, new_size);

        (*ctx)->dynamic_state = (void*)*ctx + sizeof(struct quester_context) + (*ctx)->static_state->size;
    }
    struct quester_dynamic_state *dynamic_state = (*ctx)->dynamic_state;
    memset(dynamic_state, 0, dynamic_state_size);
    dynamic_state->size = dynamic_state_size;

    fread((void*)dynamic_state + sizeof(size_t), dynamic_state_size - sizeof(size_t), 1, f);

    (*ctx)->dynamic_state->failed_node_ids = (void*)(*ctx)->dynamic_state + sizeof(struct quester_dynamic_state);
    (*ctx)->dynamic_state->completed_node_ids = (void*)(*ctx)->dynamic_state->failed_node_ids + sizeof(int) * dynamic_state->capacity;
    (*ctx)->dynamic_state->ctl_flags = (void*)(*ctx)->dynamic_state->completed_node_ids + sizeof(int) * dynamic_state->capacity;
    (*ctx)->dynamic_state->tracked_node_ids = (void*)(*ctx)->dynamic_state->ctl_flags + sizeof(enum quester_control_flags) * dynamic_state->capacity;
    (*ctx)->dynamic_state->tracked_node_data = (void*)(*ctx)->dynamic_state->tracked_node_ids + sizeof(int) * dynamic_state->capacity;
}

#endif

union quester_node *quester_add_node(struct quester_context *ctx)
{
    union quester_node *node = &ctx->static_state->all_nodes[ctx->static_state->node_count];
    ctx->dynamic_state->node_count++;
    // BUG: this is retarded, but I'm too lazy to do it properly now
    node->node.id = ctx->static_state->node_count++;

    return node;
}

void quester_add_connection(struct quester_context *ctx, int from_node_id, int to_node_id)
{
    int from_node_index = quester_find_index(ctx, from_node_id);
    int to_node_index = quester_find_index(ctx, to_node_id);

    struct node *from_node = &ctx->static_state->all_nodes[from_node_index].node;
    struct node *to_node = &ctx->static_state->all_nodes[to_node_index].node;

    from_node->out_node_ids[from_node->out_node_count++] = to_node_id;
    to_node->in_node_ids[to_node->in_node_count++] = from_node_id;

    //ctx->static_state->all_nodes[from_node_index]->node.out_node_ids[ctx->static_state->all_nodes[from_node_index]->node.out_node_count++] = to_node_id;
    //ctx->static_state->all_nodes[to_node_index]->node.in_node_ids[ctx->static_state->all_nodes[to_node_index]->node.in_node_count++] = from_node_id;
}   

void quester_reset_dynamic_state(struct quester_context *ctx)
{
    ctx->dynamic_state->failed_node_count = 0;
    memset(ctx->dynamic_state->failed_node_ids, 0, sizeof(int) * ctx->dynamic_state->capacity);

    ctx->dynamic_state->completed_node_count = 0;
    memset(ctx->dynamic_state->completed_node_ids, 0, sizeof(int) * ctx->dynamic_state->capacity);

    memset(ctx->dynamic_state->ctl_flags, 0, sizeof(enum quester_control_flags) * ctx->dynamic_state->capacity);

    ctx->dynamic_state->tracked_node_count = ctx->static_state->initially_tracked_node_count;
    memcpy(ctx->dynamic_state->tracked_node_ids, ctx->static_state->initially_tracked_node_ids, sizeof(int) * ctx->static_state->initially_tracked_node_count);

    memset(ctx->dynamic_state->tracked_node_data, 0, ctx->dynamic_state->node_count * quester_max_dynamic_data_size());
}

void quester_run(struct quester_context *ctx)
{
    if (ctx->execution_paused)
        return;

    struct quester_static_state *static_state = ctx->static_state;
    struct quester_dynamic_state *dynamic_state = ctx->dynamic_state;

    const int starting_tracked_node_count = dynamic_state->tracked_node_count;
    for (int i = 0; i < starting_tracked_node_count; i++)
    {
        int node_id = dynamic_state->tracked_node_ids[i];
        int node_index = quester_find_index(ctx, node_id);
        if (node_index == -1)
            continue;
        struct node *node = &static_state->all_nodes[node_index].node;

        void *static_node_data = static_state->static_node_data + quester_find_static_data_offset(ctx, node_id);
        void *dynamic_node_data = dynamic_state->tracked_node_data + quester_find_dynamic_data_offset(ctx, node_id);

        if (node->type != QUESTER_BUILTIN_ENTRYPOINT_TASK && !quester_node_implementations[node->type].tick(ctx, node_id, static_node_data, dynamic_node_data) != QUESTER_RUNNING)
            continue;

        // NOTE: for now assume that it's completed and not failed
        dynamic_state->completed_node_ids[dynamic_state->completed_node_count++] = node_id;

        dynamic_state->tracked_node_ids[i] = dynamic_state->tracked_node_ids[--dynamic_state->tracked_node_count];
        for (int j = 0; j < node->out_node_count; j++)
        {
            int dup = 0;
            for (int k = 0; k < dynamic_state->tracked_node_count; k++)
            {
                if (dynamic_state->tracked_node_ids[k] == node->out_node_ids[j])
                {
                    dup = 1;
                    break;
                }
            }

            int newly_tracked_node_id = quester_find_index(ctx, node->out_node_ids[j]);
            struct node *newly_tracked_node = &static_state->all_nodes[newly_tracked_node_id];
            // err wrong?
            enum quester_control_flags ctl_flags = dynamic_state->ctl_flags[newly_tracked_node_id];

            if (ctl_flags & QUESTER_CTL_DISABLE_ACTIVATION)
                continue;

            if (!dup)
                dynamic_state->tracked_node_ids[dynamic_state->tracked_node_count++] = node->out_node_ids[j];

            // NOTE: for the time being, call on_start every time, even if it's a dup
            void *newly_tracked_static_node_data = static_state->static_node_data + 
                quester_find_static_data_offset(ctx, newly_tracked_node_id);
            void *newly_tracked_dynamic_node_data = dynamic_state->tracked_node_data + 
                quester_find_dynamic_data_offset(ctx, newly_tracked_node_id);
            quester_node_implementations[newly_tracked_node->type].on_start(ctx, newly_tracked_node_id, newly_tracked_static_node_data, newly_tracked_dynamic_node_data, node_id);
        }
    }
}

//************************************************************************************************/
// Utilities 
//************************************************************************************************/
int quester_find_index(struct quester_context *ctx, int node_id)
{
    for (int i = 0; i < ctx->static_state->node_count; i++)
        if (ctx->static_state->all_nodes[i].node.id == node_id)
            return i;

    return -1;
}

int quester_max_static_data_size()
{
    static int max = -1;

    if (max == -1)
    {
        max = quester_node_implementations[0].static_data_size;
        for (int i = 1; i < QUESTER_NODE_TYPE_COUNT; i++)
            if (quester_node_implementations[i].static_data_size > max)
                max = quester_node_implementations[i].static_data_size;
    }

    return max;
}

int quester_max_dynamic_data_size()
{
    static int max = -1;

    if (max == -1)
    {
        max = quester_node_implementations[0].tracking_data_size;
        for (int i = 1; i < QUESTER_NODE_TYPE_COUNT; i++)
            if (quester_node_implementations[i].tracking_data_size > max)
                max = quester_node_implementations[i].tracking_data_size;
    }

    return max;
}

int quester_find_static_data_offset(struct quester_context *ctx, int node_id)
{
    return node_id * quester_max_static_data_size();
}

int quester_find_dynamic_data_offset(struct quester_context *ctx, int node_id)
{
    return node_id * quester_max_dynamic_data_size();
}

void quester_fill_with_test_data(struct quester_context *ctx)
{
    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ QUESTER_BUILTIN_ENTRYPOINT_TASK, 0, "Ent",  "Entrypoint" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 100;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 1;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TIMER_TASK, 1, "M_00",  "Mission 00" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 350;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 1)) = (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 2;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TIMER_TASK, 2, "M_01",  "Mission 01" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 600;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 2)) = (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 3;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TEST_TASK, 3, "M_02",  "Mission 02" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 850;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct test_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 3)) = (struct test_task){"this is mission 02\0"};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 4;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TIMER_TASK, 4, "M_03",  "Mission 03" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 1100;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 4)) = (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 2;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 5;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[1] = 6;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TIMER_TASK, 5, "SM_00", "Side-Mission 00" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 1450;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 5)) = (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 0;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TEST_TASK, 6, "SM_01", "Side-Mission 01" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 1450;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 400;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct test_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 6)) = (struct test_task){"this is side-mission 01\0"};
    ctx->static_state->node_count++;

    ctx->static_state->initially_tracked_node_count = 1;
    ctx->static_state->initially_tracked_node_ids[0] = 0;

    ctx->dynamic_state->node_count = ctx->static_state->node_count;
    quester_reset_dynamic_state(ctx);
}
