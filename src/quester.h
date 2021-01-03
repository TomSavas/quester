#define QUESTER_STATIC_ASSERT(pred, msg) typedef char msg [(pred) ? 1 : -1]

#ifdef QUESTER_IMPLEMENTATION
enum quester_node_type
{
    QUESTER_BUILTIN_CONTAINER_TASK = 0,
    QUESTER_BUILTIN_AND_TASK,
    QUESTER_BUILTIN_OR_TASK,
    QUESTER_BUILTIN_PLACEHOLDER_TASK,

    // Tasks to support container functionality
    QUESTER_BUILTIN_IN_BRIDGE_TASK,
    QUESTER_BUILTIN_OUT_BRIDGE_TASK,

    // user-defined tasks
#ifndef QUESTER_USER_NODE_TYPE_ENUMS
    #error QUESTER_USER_NODE_TYPE_ENUMS must be defined by the user
#endif
    QUESTER_USER_NODE_TYPE_ENUMS,

    QUESTER_NODE_TYPE_COUNT
};
#endif // QUESTER_IMPLEMENTATION

#ifndef QUESTER_H
#define QUESTER_H

#include <stdbool.h>

#include "quester_node_implementation.h"
#include "quester_node.h"
#include "quester_game_definition.h"
#include "quester_dynamic_state.h"
#include "quester_context.h"

int quester_find_index(struct quester_context *ctx, int node_id);
int quester_max_static_data_size();
int quester_max_dynamic_data_size();
int quester_find_static_data_offset(struct quester_context *ctx, int node_id);
int quester_find_dynamic_data_offset(struct quester_context *ctx, int node_id);
void quester_fill_with_test_data(struct quester_context *ctx);

void quester_empty() {}
enum quester_tick_result quester_empty_tick() { return 0; }

void quester_complete_task(struct quester_context *ctx, int id);

#include "quester_builtin_tasks.h"
#endif // QUESTER_H

#ifdef QUESTER_IMPLEMENTATION
QUESTER_IMPLEMENT_BUILTIN_NODES

#ifndef QUESTER_IMPLEMENT_USER_NODES
    #error QUESTER_IMPLEMENT_USER_NODES must be defined
#endif
QUESTER_IMPLEMENT_USER_NODES

const struct quester_node_implementation quester_node_implementations[QUESTER_NODE_TYPE_COUNT] =
{
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_CONTAINER_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_AND_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_OR_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_PLACEHOLDER_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_IN_BRIDGE_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_OUT_BRIDGE_TASK),

#ifndef QUESTER_USER_NODE_IMPLEMENTATIONS
    #error QUESTER_USER_NODE_IMPLEMENTATIONS must be defined by the user
#endif
    QUESTER_USER_NODE_IMPLEMENTATIONS,
};

#include "quester_node.c"
#include "quester_dynamic_state.c"
#include "quester_builtin_tasks.c"
#include "quester_context.c"

//************************************************************************************************/
// Utilities 
//************************************************************************************************/
void quester_complete_task(struct quester_context *ctx, int id)
{
    //struct node *node = &ctx->static_state->all_nodes[quester_find_index(ctx, id)].node;
    //void *static_node_data = ctx->static_state->static_node_data + 
    //quester_find_static_data_offset(ctx, id);
    //void *dynamic_node_data = ctx->dynamic_state->tracked_node_data + 
    //quester_find_dynamic_data_offset(ctx, id);

    //quester_node_implementations[node->type].on_completion(static_node_data, dynamic_node_data);
}

int quester_find_index(struct quester_context *ctx, int node_id)
{
    return node_id;
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
    union quester_node *node = quester_add_node(ctx);
    node->node = (struct node){ QUESTER_BUILTIN_IN_BRIDGE_TASK, 0, "Game entrypoint",  "Game entrypoint" };
    node->editor_node.prop_rect = nk_rect(0, 0, 300, 400);
    node->editor_node.bounds.x = 0;
    node->editor_node.bounds.y = 0;
    node->editor_node.bounds.w = 400;
    node->editor_node.bounds.h = 150;

    node = quester_add_container_node(ctx);
    node->node = (struct node){ QUESTER_BUILTIN_CONTAINER_TASK, 1, "C01", "Container 01" };
    node->editor_node.prop_rect = nk_rect(0, 0, 300, 400);
    node->editor_node.bounds.x = 500;
    node->editor_node.bounds.y = 0;
    node->editor_node.bounds.w = 200;
    node->editor_node.bounds.h = 150;

    ctx->static_state->initially_tracked_node_count = 1;
    ctx->static_state->initially_tracked_node_ids[0] = 0;

    ctx->dynamic_state->node_count = ctx->static_state->node_count;
    quester_reset_dynamic_state(ctx);
}
#endif // QUESTER_IMPLEMENTATION
