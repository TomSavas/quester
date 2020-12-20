#include <stdbool.h>

struct quester_context;

struct quester_context *quester_init(int capacity);
void quester_free(struct quester_context *ctx);

// saving/loading
//void quester_dump_bin(struct quester_context *ctx, const char *dir_path, const char *filename);
//struct quester_context *quester_load_bin(const char *dir_path, const char *filename);

// saving/loading
void quester_dump_static_bin(struct quester_context *ctx, const char *dir_path, const char *filename);
// Clears dynamic state
void quester_load_static_bin(struct quester_context **ctx, const char *dir_path, const char *filename);

void quester_dump_dynamic_bin(struct quester_context *ctx, const char *dir_path, const char *filename);
void quester_load_dynamic_bin(struct quester_context **ctx, const char *dir_path, const char *filename);

//void quester_dump_static_bin(struct quester_context *ctx, 

// editing
union quester_node *quester_add_node(struct quester_context *ctx);

void quester_reset_dynamic_state(struct quester_context *ctx);

// running
void quester_run(struct quester_context *ctx);

struct quester_node_implementation
{
    const char name[256];
    void (*on_start)(void* /*static_node_data*/, void* /*tracking_node_data*/);
    bool (*is_completed)(void* /*static_node_data*/, void* /*tracking_node_data*/);
    void (*nk_display)(void* /*static_node_data*/, void* /*tracking_node_data*/);

    const int static_data_size;
    const int tracking_data_size;
};

#define QUESTER_IMPLEMENT_NODE(node_type_enum, static_node_data_struct, tracking_node_data_struct, \
        on_start_func, is_completed_func, nk_display_func)                                         \
                                                                                                   \
    void node_type_enum##_on_start_typecorrect_wrapper(void *static_node_data,                     \
            void *tracking_node_data)                                                              \
    {                                                                                              \
        on_start_func((static_node_data_struct*)static_node_data,                                  \
                (tracking_node_data_struct*)tracking_node_data);                                   \
    }                                                                                              \
                                                                                                   \
    bool node_type_enum##_is_completed_typecorrect_wrapper(void *static_node_data,                 \
            void *tracking_node_data)                                                              \
    {                                                                                              \
        return is_completed_func((static_node_data_struct*)static_node_data,                       \
                (tracking_node_data_struct*)tracking_node_data);                                   \
    }                                                                                              \
                                                                                                   \
    void node_type_enum##_nk_display_typecorrect_wrapper(void *static_node_data,                   \
            void *tracking_node_data)                                                              \
    {                                                                                              \
        nk_display_func((static_node_data_struct*)static_node_data,                                \
                (tracking_node_data_struct*)tracking_node_data);                                   \
    }                                                                                              \
                                                                                                   \
    const int static_data_size_##node_type_enum = sizeof(static_node_data_struct);                 \
    const int tracking_data_size_##node_type_enum = sizeof(tracking_node_data_struct);             \

// NOTE: possible to reduce .static_data_size / .tracking_data_size to true compile-time constants?
#define QUESTER_NODE_IMPLEMENTATION(node_type_enum)                                                \
    {                                                                                              \
        .name               = #node_type_enum,                                                     \
        .on_start           = node_type_enum##_on_start_typecorrect_wrapper,                       \
        .is_completed       = node_type_enum##_is_completed_typecorrect_wrapper,                   \
        .nk_display         = node_type_enum##_nk_display_typecorrect_wrapper,                     \
        .static_data_size   = static_data_size_##node_type_enum,                                   \
        .tracking_data_size = tracking_data_size_##node_type_enum                                  \
    }                                                                                              \

#include "temp_tasks.h"

enum quester_node_type
{
    // builtin tasks
    QUESTER_BUILTIN_AND_TASK = 0,
    QUESTER_BUILTIN_OR_TASK,
    QUESTER_BUILTIN_ENTRYPOINT_TASK,

    // user-defined tasks
    TIMER_TASK,
    TEST_TASK,

    QUESTER_NODE_TYPE_COUNT
};

struct quester_node_implementation quester_node_implementations[QUESTER_NODE_TYPE_COUNT] =
{
    {"AND_TASK", NULL, NULL, NULL, 0, 0}, //QUESTER_BUILTIN_AND_TASK, not implemented yet
    {"OR_TASK", NULL, NULL, NULL, 0, 0}, //QUESTER_BUILTIN_OR_TASK, not implemented yet
    {"ENTRYPOINT_TASK", NULL, NULL, NULL, 0, 0}, //QUESTER_BUILTIN_OR_TASK, not implemented yet
    QUESTER_NODE_IMPLEMENTATION(TIMER_TASK),
    QUESTER_NODE_IMPLEMENTATION(TEST_TASK)
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

    int node_count;
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

#define QUESTER_USE_LIBC
#ifdef QUESTER_USE_LIBC
#include <stdio.h>
struct quester_context *quester_init(int capacity)
{
    size_t static_state_size = sizeof(struct quester_static_state) + 
        (sizeof(int) + sizeof(union quester_node) + quester_max_static_data_size()) * capacity;
    size_t dynamic_state_size = sizeof(struct quester_dynamic_state) +
        (sizeof(int) + quester_max_dynamic_data_size()) * capacity;

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
    ctx->dynamic_state->tracked_node_ids = (void*)ctx->dynamic_state + sizeof(struct quester_dynamic_state);
    ctx->dynamic_state->tracked_node_data = (void*)ctx->dynamic_state->tracked_node_ids + sizeof(int) * capacity;
    ctx->dynamic_state->size = dynamic_state_size;
    ctx->dynamic_state->capacity = capacity;
    ctx->static_state->node_count = 0;

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

    dynamic_state->tracked_node_ids = (void*)dynamic_state + sizeof(struct quester_dynamic_state); 
    dynamic_state->tracked_node_data = (void*)dynamic_state->tracked_node_ids + sizeof(int) * dynamic_state->capacity;
}

#endif

union quester_node *quester_add_node(struct quester_context *ctx)
{
    union quester_node *node = &ctx->static_state->all_nodes[ctx->static_state->node_count++];
    ctx->dynamic_state->node_count++;
    // BUG: this is retarded, but I'm too lazy to do it properly now
    node->node.id = ctx->static_state->node_count;

    return node;
}

void quester_reset_dynamic_state(struct quester_context *ctx)
{
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
        if (node->type != QUESTER_BUILTIN_ENTRYPOINT_TASK && !quester_node_implementations[node->type].is_completed(static_node_data, dynamic_node_data))
            continue;

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

            if (!dup)
            {
                int newly_tracked_node_id = quester_find_index(ctx, node->out_node_ids[j]);
                struct node *newly_tracked_node = &static_state->all_nodes[newly_tracked_node_id];
                void *newly_tracked_static_node_data = static_state->static_node_data + 
                    quester_find_static_data_offset(ctx, newly_tracked_node_id);
                void *newly_tracked_dynamic_node_data = dynamic_state->tracked_node_data + 
                    quester_find_dynamic_data_offset(ctx, newly_tracked_node_id);
                quester_node_implementations[newly_tracked_node->type].on_start(newly_tracked_static_node_data, newly_tracked_dynamic_node_data);

                dynamic_state->tracked_node_ids[dynamic_state->tracked_node_count++] = node->out_node_ids[j];
            }
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
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 100;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 1;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TIMER_TASK, 1, "M_00",  "Mission 00" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 100;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 1)) = (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 2;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TIMER_TASK, 2, "M_01",  "Mission 01" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 500;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 100;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 2)) = (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 3;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TEST_TASK, 3, "M_02",  "Mission 02" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 700;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 100;
    *(struct test_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 3)) = (struct test_task){"this is mission 02\0"};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 4;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TIMER_TASK, 4, "M_03",  "Mission 03" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 900;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 100;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 4)) = (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 2;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 5;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[1] = 6;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TIMER_TASK, 5, "SM_00", "Side-Mission 00" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 1100;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 100;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 5)) = (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 0;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = (struct node){ TEST_TASK, 6, "SM_01", "Side-Mission 01" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 1100;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 400;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 100;
    *(struct test_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 6)) = (struct test_task){"this is side-mission 01\0"};
    ctx->static_state->node_count++;

    ctx->static_state->initially_tracked_node_count = 1;
    ctx->static_state->initially_tracked_node_ids[0] = 0;

    ctx->dynamic_state->node_count = ctx->static_state->node_count;
    quester_reset_dynamic_state(ctx);
}
