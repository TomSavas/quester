#include <stdbool.h>

struct quester_context;

struct quester_context *quester_init(int capacity);
void quester_free(struct quester_context *ctx);

// saving/loading
void quester_dump_bin(struct quester_context *ctx, const char *filepath);
struct quester_context *quester_load_bin(const char *filepath);

// editing
union quester_node *quester_add_node(struct quester_context *ctx);

// running
void quester_run(struct quester_context *ctx);

struct quester_node_implementation
{
    const char name[256];
    void (*on_start)(void*);
    bool (*is_completed)(void*);
    void (*nk_display)(void*);
    int data_size;
};

#define QUESTER_IMPLEMENT_NODE(node_type_enum, node_struct, on_start_func,                         \
        is_completed_func, nk_display_func)                                                        \
                                                                                                   \
    void on_start_func##_typecorrect_wrapper(void *node_data)                                      \
    {                                                                                              \
        on_start_func((node_struct*)node_data);                                                    \
    }                                                                                              \
                                                                                                   \
    bool is_completed_func##_typecorrect_wrapper(void *node_data)                                  \
    {                                                                                              \
        return is_completed_func((node_struct*)node_data);                                         \
    }                                                                                              \
                                                                                                   \
    void nk_display_func##_typecorrect_wrapper(void *node_data)                                    \
    {                                                                                              \
        nk_display_func((node_struct*)node_data);                                                  \
    }                                                                                              \
                                                                                                   \
    void(* const on_start_##node_type_enum)(void*) = on_start_func##_typecorrect_wrapper;          \
    bool(* const is_completed_##node_type_enum)(void*) = is_completed_func##_typecorrect_wrapper;  \
    void(* const nk_display_##node_type_enum)(void*) = nk_display_func##_typecorrect_wrapper;      \
    const int data_size_##node_type_enum = sizeof(node_struct);                                    \

#define QUESTER_NODE_IMPLEMENTATION(node_type_enum)                                                \
    {                                                                                              \
        .name = #node_type_enum,                                                                   \
        .on_start = on_start_##node_type_enum,                                                     \
        .is_completed = is_completed_##node_type_enum,                                             \
        .nk_display   = nk_display_##node_type_enum,                                               \
        .data_size    = data_size_##node_type_enum                                                 \
    }                                                                                              \

#include "temp_tasks.h"

enum quester_node_type
{
    // builtin tasks
    QUESTER_BUILTIN_AND_TASK = 0,
    QUESTER_BUILTIN_OR_TASK,

    // user-defined tasks
    TIMER_TASK,
    TEST_TASK,

    QUESTER_NODE_TYPE_COUNT
};

struct quester_node_implementation quester_node_implementations[QUESTER_NODE_TYPE_COUNT] =
{
    {"AND_TASK", NULL, NULL, NULL, 0}, //QUESTER_BUILTIN_AND_TASK, not implemented yet
    {"OR_TASK", NULL, NULL, NULL, 0}, //QUESTER_BUILTIN_OR_TASK, not implemented yet
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

struct quester_context
{
    size_t ctx_size;
    int capacity;

    bool execution_paused;

    int tracked_node_count;
    int *tracked_node_ids;

    int node_count;
    union quester_node *all_nodes;
    void *user_data;
};

int quester_find_index(struct quester_context *ctx, int node_id);
int quester_calc_user_data_item_size();
int quester_find_user_data_offset(struct quester_context *ctx, int node_id);
void quester_assign_pointers(struct quester_context *ctx);
void quester_fill_with_test_data(struct quester_context *ctx);

#define QUESTER_USE_LIBC
#ifdef QUESTER_USE_LIBC
#include <stdio.h>
struct quester_context *quester_init(int capacity)
{
    size_t ctx_size = sizeof(struct quester_context) + (sizeof(int) + sizeof(union quester_node) + quester_calc_user_data_item_size()) * capacity;
    struct quester_context *ctx = malloc(ctx_size);
    memset(ctx, 0, ctx_size);
    ctx->ctx_size = ctx_size;
    ctx->capacity = capacity;
    ctx->execution_paused = false;
    ctx->node_count = 0;

    quester_assign_pointers(ctx);
    quester_fill_with_test_data(ctx);

    return ctx;
}

void quester_free(struct quester_context *ctx)
{
    free(ctx);
}

struct quester_context *quester_resize(struct quester_context *ctx, int new_capacity, bool free_old_ctx)
{
    struct quester_context *new_ctx = quester_init(new_capacity);

    new_ctx->tracked_node_count = ctx->tracked_node_count;
    new_ctx->node_count = ctx->node_count;
    quester_assign_pointers(ctx);

    memcpy(new_ctx->tracked_node_ids, ctx->tracked_node_ids, sizeof(int) * ctx->tracked_node_count);
    new_ctx->tracked_node_count = ctx->tracked_node_count;

    memcpy(new_ctx->all_nodes, ctx->all_nodes, sizeof(union quester_node) * ctx->node_count);
    new_ctx->node_count = ctx->node_count;

    // BUG: this might break once calc_user_data_item_size returns a different
    // size between for the same save file.
    memcpy(new_ctx->user_data, ctx->user_data, quester_calc_user_data_item_size() * ctx->node_count);

    if (free_old_ctx)
        quester_free(ctx);
    return new_ctx;
}

struct quester_context *quester_shrink(struct quester_context *ctx, bool free_old_ctx)
{
    return quester_resize(ctx, ctx->node_count, free_old_ctx);
}

void quester_dump_bin(struct quester_context *ctx, const char *filepath)
{
    FILE *f = fopen(filepath, "w");
    fwrite(ctx, ctx->ctx_size, 1, f);
    fclose(f);
}

struct quester_context *quester_load_bin(const char *filepath)
{
    FILE *f = fopen(filepath, "r");

    size_t ctx_size;
    fread(&ctx_size, sizeof(size_t), 1, f);
    struct quester_context *ctx = malloc(ctx_size);
    ctx->ctx_size = ctx_size;

    fread((void*)ctx + sizeof(size_t), ctx->ctx_size - sizeof(size_t), 1, f);
    fclose(f);

    quester_assign_pointers(ctx);

    return ctx;
}
#endif

union quester_node *quester_add_node(struct quester_context *ctx)
{
    union quester_node *node = &ctx->all_nodes[ctx->node_count++];
    // BUG: this is retarded, but I'm too lazy to do it properly now
    node->node.id = ctx->node_count;

    return node;
}

void quester_run(struct quester_context *ctx)
{
    if (ctx->execution_paused)
        return;

    const int starting_tracked_node_count = ctx->tracked_node_count;
    for (int i = 0; i < starting_tracked_node_count; i++)
    {
        int node_id = ctx->tracked_node_ids[i];
        int node_index = quester_find_index(ctx, node_id);
        if (node_index == -1)
            continue;
        struct node *node = &ctx->all_nodes[node_index].node;

        void *node_data = ctx->user_data + quester_find_user_data_offset(ctx, node_id);
        if (!quester_node_implementations[node->type].is_completed(node_data))
            continue;

        ctx->tracked_node_ids[i] = ctx->tracked_node_ids[--ctx->tracked_node_count];
        for (int j = 0; j < node->out_node_count; j++)
        {
            int dup = 0;
            for (int k = 0; k < ctx->tracked_node_count; k++)
            {
                if (ctx->tracked_node_ids[k] == node->out_node_ids[j])
                {
                    dup = 1;
                    break;
                }
            }

            if (!dup)
            {
                int newly_tracked_node_id = quester_find_index(ctx, node->out_node_ids[j]);
                struct node *newly_tracked_node = &ctx->all_nodes[newly_tracked_node_id];
                void *newly_tracked_node_data = ctx->user_data + quester_find_user_data_offset(ctx, newly_tracked_node_id);
                quester_node_implementations[newly_tracked_node->type].on_start(newly_tracked_node_data);

                ctx->tracked_node_ids[ctx->tracked_node_count++] = node->out_node_ids[j];
            }
        }
    }
}

//************************************************************************************************/
// Utilities 
//************************************************************************************************/
int quester_find_index(struct quester_context *ctx, int node_id)
{
    for (int i = 0; i < ctx->node_count; i++)
        if (ctx->all_nodes[i].node.id == node_id)
            return i;

    return -1;
}

int quester_calc_user_data_item_size()
{
    static int max = -1;

    if (max == -1)
    {
        max = quester_node_implementations[0].data_size;
        for (int i = 1; i < QUESTER_NODE_TYPE_COUNT; i++)
            if (quester_node_implementations[i].data_size > max)
                max = quester_node_implementations[i].data_size;
    }

    return max;
}

int quester_find_user_data_offset(struct quester_context *ctx, int node_id)
{
    return node_id * quester_calc_user_data_item_size();
}

void quester_assign_pointers(struct quester_context *ctx)
{
    ctx->tracked_node_ids = (void*)ctx + sizeof(struct quester_context);
    ctx->all_nodes = (void*)ctx->tracked_node_ids + sizeof(int) * ctx->capacity;
    ctx->user_data = (void*)ctx->all_nodes + sizeof(union quester_node) * ctx->capacity;
}

void quester_fill_with_test_data(struct quester_context *ctx)
{
    ctx->all_nodes[ctx->node_count].node = (struct node){ TIMER_TASK, 0, "M_00",  "Mission 00" };
    ctx->all_nodes[ctx->node_count].editor_node.bounds.x = 300;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.y = 300;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.w = 150;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.h = 100;
    *(struct timer_task*)(ctx->user_data + quester_find_user_data_offset(ctx, 0)) = (struct timer_task){0, 100};
    ctx->all_nodes[ctx->node_count].node.out_node_count = 1;
    ctx->all_nodes[ctx->node_count].node.out_node_ids[0] = 1;
    ctx->node_count++;

    ctx->all_nodes[ctx->node_count].node = (struct node){ TIMER_TASK, 1, "M_01",  "Mission 01" };
    ctx->all_nodes[ctx->node_count].editor_node.bounds.x = 500;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.y = 300;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.w = 150;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.h = 100;
    *(struct timer_task*)(ctx->user_data + quester_find_user_data_offset(ctx, 1)) = (struct timer_task){0, 100};
    ctx->all_nodes[ctx->node_count].node.out_node_count = 1;
    ctx->all_nodes[ctx->node_count].node.out_node_ids[0] = 2;
    ctx->node_count++;

    ctx->all_nodes[ctx->node_count].node = (struct node){ TEST_TASK, 2, "M_02",  "Mission 02" };
    ctx->all_nodes[ctx->node_count].editor_node.bounds.x = 700;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.y = 300;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.w = 150;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.h = 100;
    *(struct test_task*)(ctx->user_data + quester_find_user_data_offset(ctx, 2)) = (struct test_task){"this is mission 02\0"};
    ctx->all_nodes[ctx->node_count].node.out_node_count = 1;
    ctx->all_nodes[ctx->node_count].node.out_node_ids[0] = 3;
    ctx->node_count++;

    ctx->all_nodes[ctx->node_count].node = (struct node){ TIMER_TASK, 3, "M_03",  "Mission 03" };
    ctx->all_nodes[ctx->node_count].editor_node.bounds.x = 900;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.y = 300;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.w = 150;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.h = 100;
    *(struct timer_task*)(ctx->user_data + quester_find_user_data_offset(ctx, 3)) = (struct timer_task){0, 100};
    ctx->all_nodes[ctx->node_count].node.out_node_count = 2;
    ctx->all_nodes[ctx->node_count].node.out_node_ids[0] = 4;
    ctx->all_nodes[ctx->node_count].node.out_node_ids[1] = 5;
    ctx->node_count++;

    ctx->all_nodes[ctx->node_count].node = (struct node){ TIMER_TASK, 4, "SM_00", "Side-Mission 00" };
    ctx->all_nodes[ctx->node_count].editor_node.bounds.x = 1100;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.y = 200;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.w = 150;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.h = 100;
    *(struct timer_task*)(ctx->user_data + quester_find_user_data_offset(ctx, 4)) = (struct timer_task){0, 100};
    ctx->all_nodes[ctx->node_count].node.out_node_count = 0;
    ctx->node_count++;

    ctx->all_nodes[ctx->node_count].node = (struct node){ TEST_TASK, 5, "SM_01", "Side-Mission 01" };
    ctx->all_nodes[ctx->node_count].editor_node.bounds.x = 1100;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.y = 400;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.w = 150;
    ctx->all_nodes[ctx->node_count].editor_node.bounds.h = 100;
    *(struct test_task*)(ctx->user_data + quester_find_user_data_offset(ctx, 5)) = (struct test_task){"this is side-mission 01\0"};
    ctx->node_count++;

    ctx->tracked_node_count = 0;
    ctx->tracked_node_ids[ctx->tracked_node_count++] = 0;
}
