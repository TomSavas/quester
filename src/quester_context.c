#define QUESTER_USE_LIBC
#ifdef QUESTER_USE_LIBC
#include <stdio.h>
struct quester_context *quester_init(int capacity)
{
    size_t static_state_size = sizeof(struct quester_quest_definitions) + 
        (sizeof(int) + sizeof(union quester_node) + quester_max_static_data_size()) * capacity;
    size_t dynamic_state_size = sizeof(struct quester_runtime_quest_data) +
        (sizeof(int) + sizeof(int) + sizeof(int) + sizeof(enum quester_activation_flags) + 
         quester_max_dynamic_data_size()) * capacity;

    size_t total_ctx_size = sizeof(struct quester_context) + static_state_size + dynamic_state_size;
    struct quester_context *ctx = malloc(total_ctx_size);
    memset(ctx, 0, total_ctx_size);
    ctx->size = total_ctx_size;

    ctx->execution_paused = false;

    ctx->static_state = (void*)ctx + sizeof(struct quester_context);
    ctx->static_state->available_ids = (void*)ctx->static_state + 
        sizeof(struct quester_quest_definitions);
    ctx->static_state->initially_tracked_node_ids = (void*)ctx->static_state->available_ids + 
        sizeof(int) * capacity;
    ctx->static_state->all_nodes = (void*)ctx->static_state->initially_tracked_node_ids + 
        sizeof(int) * capacity;
    ctx->static_state->static_node_data = (void*)ctx->static_state->all_nodes + 
        sizeof(union quester_node) * capacity;
    ctx->static_state->size = static_state_size;
    ctx->static_state->capacity = capacity;
    ctx->static_state->initially_tracked_node_count = 0;
    ctx->static_state->node_count = 0;

    // fill in available_ids
    for (int i = 0; i < capacity; i++)
        ctx->static_state->available_ids[i] = capacity - i - 1;

    ctx->dynamic_state = (void*)ctx->static_state + static_state_size;
    ctx->dynamic_state->finishing_status = (void*)ctx->dynamic_state + 
        sizeof(struct quester_runtime_quest_data);
    ctx->dynamic_state->activation_flags = (void*)ctx->dynamic_state->finishing_status + 
        sizeof(enum quester_out_connection_type) * capacity;
    ctx->dynamic_state->tracked_node_ids = (void*)ctx->dynamic_state->activation_flags + 
        sizeof(enum quester_activation_flags) * capacity;
    ctx->dynamic_state->tracked_node_data = (void*)ctx->dynamic_state->tracked_node_ids + 
        sizeof(int) * capacity;
    ctx->dynamic_state->size = dynamic_state_size;
    ctx->dynamic_state->capacity = capacity;
    ctx->dynamic_state->node_count = 0;
    ctx->dynamic_state->tracked_ticking_node_count = 0;
    ctx->dynamic_state->tracked_non_ticking_node_count = 0;

    quester_fill_with_test_data(ctx);

    return ctx;
}

void quester_free(struct quester_context *ctx)
{
    free(ctx);
}

#endif

struct quester_node_changes
{
    int ticking_nodes_to_add_count;
    int ticking_nodes_to_add[64];
    int non_ticking_nodes_to_add_count;
    int non_ticking_nodes_to_add[64];

    int nodes_to_remove_count;
    // nodes_to_remove_before_adding?
    int nodes_to_remove[128];
};

struct quester_activation_result quester_trigger_activators(struct quester_context *ctx,
    struct quester_connection connection, int id, bool is_non_activating)
{
    struct quester_runtime_quest_data *dynamic_state = ctx->dynamic_state;

    enum quester_activation_flags previous_activation_flags = dynamic_state->activation_flags[id];
    if (previous_activation_flags & QUESTER_ACTIVATE && !(previous_activation_flags & QUESTER_ALLOW_REPEATED_ACTIVATIONS))
    //if (!(previous_activation_flags & QUESTER_ALLOW_REPEATED_ACTIVATIONS))
        return (struct quester_activation_result) { 0 };

    struct quester_quest_definitions *static_state = ctx->static_state;
    struct node *node = &static_state->all_nodes[id].node;

    void *static_data = static_state->static_node_data + quester_find_static_data_offset(ctx, id);
    void *dynamic_data = dynamic_state->tracked_node_data + 
        quester_find_dynamic_data_offset(ctx, id);
    struct quester_activation_result activator_result = is_non_activating
        ? quester_node_implementations[node->type].non_activator(ctx, id, static_data, dynamic_data, &connection)
        : quester_node_implementations[node->type].activator(ctx, id, static_data, dynamic_data, &connection);

    // Editor only, signal that the node hasn't been completed
    dynamic_state->finishing_status[id] = -1;
    // Once a node is activated it stays activated until it outputs something (controlled by tick function)
    dynamic_state->activation_flags[id] = activator_result.flags | (previous_activation_flags & QUESTER_ACTIVATE);

    return activator_result;
}

void quester_trigger_activators_recurse(struct quester_context *ctx,
    struct quester_connection connection, struct quester_node_changes *node_changes,
    bool is_non_activating)
{
    struct quester_activation_result res = quester_trigger_activators(ctx, connection, connection.out.to_id, is_non_activating);

    if (res.flags & QUESTER_ACTIVATE)
    {
        // Removals are done first, so the net effect is that if the node previously was ticking
        // and now is non_ticking, we wll remove it form ticking nodes and add it to the 
        // non_ticking nodes.
        // If the node was ticking and still is, then removal/add will have no net effect
        node_changes->nodes_to_remove[node_changes->nodes_to_remove_count++] = connection.out.to_id;

        if (res.flags & QUESTER_DISABLE_TICKING)
            node_changes->non_ticking_nodes_to_add[node_changes->non_ticking_nodes_to_add_count++] = connection.out.to_id;
        else
            node_changes->ticking_nodes_to_add[node_changes->ticking_nodes_to_add_count++] = connection.out.to_id;
    }

    if (res.flags & QUESTER_FORWARD_CONNECTIONS_TO_IDS)
        for (int i = 0; i < res.id_count; i++)
        {
            connection.out.to_id = res.ids[i];
            quester_trigger_activators_recurse(ctx, connection, node_changes, is_non_activating);
        }
}

void quester_run_node_recurse(struct quester_context *ctx, int id,
    struct quester_tick_result *forwarded_tick_result, struct quester_node_changes *node_changes)
{
    struct quester_quest_definitions *static_state = ctx->static_state;
    struct quester_runtime_quest_data *dynamic_state = ctx->dynamic_state;

    struct node *node = &static_state->all_nodes[id].node;

    struct quester_tick_result res;
    if (forwarded_tick_result == NULL)
    {
        void *static_node_data = static_state->static_node_data + 
            quester_find_static_data_offset(ctx, id);
        void *dynamic_node_data = dynamic_state->tracked_node_data + 
            quester_find_dynamic_data_offset(ctx, id);

        res = quester_node_implementations[node->type].tick(ctx, id, static_node_data,
            dynamic_node_data);
    }
    else
    {            
        res = (struct quester_tick_result) 
        { 
            .flags = forwarded_tick_result->flags_to_forward,
            .out_connection_to_trigger = forwarded_tick_result->out_connections_to_forward_trigger
        };

        // Cannot forward a forwarding flag in order to avoid funky infinite recursion behaviour
        assert(!(res.flags & QUESTER_FORWARD_TICK_RESULT_TO_IDS));
    }

    if (!(res.flags & QUESTER_STILL_RUNNING) || res.out_connection_to_trigger & QUESTER_DEAD_OUTPUT)
    {
        for (int i = 0; i < node->out_connection_count; i++)
        {
            bool is_non_activating = 
                node->out_connections[i].type != res.out_connection_to_trigger;

            struct quester_connection connection = 
            {
                .out = node->out_connections[i],
                .in = { QUESTER_ACTIVATION_INPUT, id }
            };
            quester_trigger_activators_recurse(ctx, connection, node_changes, is_non_activating);
        }

        node_changes->nodes_to_remove[node_changes->nodes_to_remove_count++] = id;
        ctx->dynamic_state->finishing_status[id] = res.out_connection_to_trigger;
        ctx->dynamic_state->activation_flags[id] = 0;
    }

    if (res.flags & QUESTER_FORWARD_TICK_RESULT_TO_IDS)
        for (int i = 0; i < res.id_count; i++)
            quester_run_node_recurse(ctx, res.ids[i], &res, node_changes);
}

void quester_run(struct quester_context *ctx)
{
    if (ctx->execution_paused)
        return;

    struct quester_runtime_quest_data *dynamic_state = ctx->dynamic_state;
    struct quester_node_changes node_changes = 
    {
        .ticking_nodes_to_add_count = 0,
        .non_ticking_nodes_to_add_count = 0,
        .nodes_to_remove_count = 0
    };
    for (int i = 0; i < dynamic_state->tracked_ticking_node_count; i++)
        quester_run_node_recurse(ctx, dynamic_state->tracked_node_ids[i], NULL, &node_changes);

    for (int i = 0; i < node_changes.nodes_to_remove_count; i++)
    {            
        bool removed = false;
        for (int j = 0; j < dynamic_state->tracked_ticking_node_count && !removed; j++)
        {
            if(dynamic_state->tracked_node_ids[j] != node_changes.nodes_to_remove[i])
                continue;

            dynamic_state->tracked_node_ids[j] = 
                dynamic_state->tracked_node_ids[--dynamic_state->tracked_ticking_node_count];
            removed = true;
        }

        for (int j = 0; j < dynamic_state->tracked_non_ticking_node_count && !removed; j++)
        {
            int non_ticking_id = dynamic_state->capacity - 1 - j;
            if(dynamic_state->tracked_node_ids[non_ticking_id] != node_changes.nodes_to_remove[i])
                continue;

            dynamic_state->tracked_node_ids[non_ticking_id] = 
                dynamic_state->tracked_node_ids[--dynamic_state->tracked_non_ticking_node_count];
            removed = true;
        }
    }

    // TODO: maybe just faster to swap the fors? Duplicates are unlikely - only occur when we are
    // switching between ticking to non ticking
    for (int i = 0; i < dynamic_state->tracked_ticking_node_count; i++)
    {            
        // Remove duplicates
        for (int j = 0; j < node_changes.ticking_nodes_to_add_count; j++)
            if (dynamic_state->tracked_node_ids[i] == node_changes.ticking_nodes_to_add[j])               
                node_changes.ticking_nodes_to_add[j--] = node_changes.ticking_nodes_to_add[node_changes.ticking_nodes_to_add_count--];
    }
    for (int i = 0; i < node_changes.ticking_nodes_to_add_count; i++)
        dynamic_state->tracked_node_ids[dynamic_state->tracked_ticking_node_count++] = node_changes.ticking_nodes_to_add[i];

    for (int i = 0; i < dynamic_state->tracked_non_ticking_node_count; i++)
    {                        
        int idx = dynamic_state->capacity - 1 - i;

        // Remove duplicates
        for (int j = 0; j < node_changes.non_ticking_nodes_to_add_count; j++)
            if (dynamic_state->tracked_node_ids[idx] == node_changes.non_ticking_nodes_to_add[j])               
                node_changes.non_ticking_nodes_to_add[j--] = node_changes.non_ticking_nodes_to_add[node_changes.non_ticking_nodes_to_add_count--];
    }
    for (int i = 0; i < node_changes.non_ticking_nodes_to_add_count; i++)
    {
        int idx = dynamic_state->capacity - 1 - dynamic_state->tracked_non_ticking_node_count++;
        dynamic_state->tracked_node_ids[idx] = node_changes.non_ticking_nodes_to_add[i];
    }
}