#ifdef QUESTER_IMPLEMENTATION
enum quester_node_type
{
    // builtin tasks
    QUESTER_BUILTIN_AND_TASK = 0,
    QUESTER_BUILTIN_OR_TASK,
    QUESTER_BUILTIN_PLACEHOLDER_TASK,
    QUESTER_BUILTIN_ENTRYPOINT_TASK,

    // user-defined tasks
    QUESTER_USER_NODE_TYPE_ENUMS,
    // TODO: static_assert here to assure this is defined

    QUESTER_NODE_TYPE_COUNT
};
#endif

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
//#include "temp_tasks.h"



#endif

#ifdef QUESTER_IMPLEMENTATION
QUESTER_IMPLEMENT_BUILTIN_NODES
QUESTER_IMPLEMENT_USER_NODES
// TODO: static_assert here to assure this is defined

const struct quester_node_implementation quester_node_implementations[QUESTER_NODE_TYPE_COUNT] =
{
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_AND_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_OR_TASK),
    QUESTER_NODE_IMPLEMENTATION(QUESTER_BUILTIN_PLACEHOLDER_TASK),
    {"ENTRYPOINT_TASK", quester_empty_tick, quester_empty, quester_empty, quester_empty, 0, 0},
    QUESTER_USER_NODE_IMPLEMENTATIONS,
    // TODO: static_assert here to assure this is defined
};

#include "quester_context.c"
#include "quester_node.c"
#include "quester_dynamic_state.c"
#include "quester_builtin_tasks.c"

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
    ctx->static_state->all_nodes[ctx->static_state->node_count].node = 
        (struct node){ QUESTER_BUILTIN_ENTRYPOINT_TASK, 0, "Ent",  "Entrypoint" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 100;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 1;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = 
        (struct node){ TIMER_TASK, 1, "M_00",  "Mission 00" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 350;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 1)) = 
        (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 2;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node =
        (struct node){ TIMER_TASK, 2, "M_01",  "Mission 01" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 600;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 2)) = 
        (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 3;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = 
        (struct node){ TEST_TASK, 3, "M_02",  "Mission 02" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 850;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct test_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 3)) = 
        (struct test_task){"this is mission 02\0"};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 1;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 4;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = 
        (struct node){ TIMER_TASK, 4, "M_03",  "Mission 03" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 1100;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 300;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 4)) = 
        (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 2;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[0] = 5;
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_ids[1] = 6;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = 
        (struct node){ TIMER_TASK, 5, "SM_00", "Side-Mission 00" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 1450;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct timer_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 5)) = 
        (struct timer_task){0, 100};
    ctx->static_state->all_nodes[ctx->static_state->node_count].node.out_node_count = 0;
    ctx->static_state->node_count++;

    ctx->static_state->all_nodes[ctx->static_state->node_count].node = 
        (struct node){ TEST_TASK, 6, "SM_01", "Side-Mission 01" };
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.x = 1450;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.y = 400;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.w = 200;
    ctx->static_state->all_nodes[ctx->static_state->node_count].editor_node.bounds.h = 150;
    *(struct test_task*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, 6)) = 
        (struct test_task){"this is side-mission 01\0"};
    ctx->static_state->node_count++;

    ctx->static_state->initially_tracked_node_count = 1;
    ctx->static_state->initially_tracked_node_ids[0] = 0;

    ctx->dynamic_state->node_count = ctx->static_state->node_count;
    quester_reset_dynamic_state(ctx);
}
#endif
