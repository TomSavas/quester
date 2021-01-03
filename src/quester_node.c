union quester_node *quester_add_node(struct quester_context *ctx)
{
    // Pull the id from the pool
    int available_id_index = ctx->static_state->capacity - ctx->static_state->node_count++;
    int id = ctx->static_state->available_ids[available_id_index - 1];

    union quester_node *node = &ctx->static_state->all_nodes[id];
    ctx->dynamic_state->node_count++;
    node->node.id = id;

    return node;
}

union quester_node *quester_add_container_node(struct quester_context *ctx)
{
    union quester_node *container_node = quester_add_node(ctx);

    union quester_node *in_bridge = quester_add_node(ctx);
    in_bridge->node.type = QUESTER_BUILTIN_IN_BRIDGE_TASK;
    enum in_connection_type *in_bridge_type = ctx->static_state->static_node_data + 
        quester_find_static_data_offset(ctx, in_bridge->node.id);
    *in_bridge_type = QUESTER_ACTIVATION_INPUT;
    // TEMP
    in_bridge->editor_node.bounds.x = 100;
    in_bridge->editor_node.bounds.y = 400;
    in_bridge->editor_node.bounds.w = 300;
    in_bridge->editor_node.bounds.h = 150;

    union quester_node *completion_out_bridge = quester_add_node(ctx);
    completion_out_bridge->node.type = QUESTER_BUILTIN_OUT_BRIDGE_TASK;
    enum out_connection_type *completion_bridge_type = ctx->static_state->static_node_data + 
        quester_find_static_data_offset(ctx, completion_out_bridge->node.id);
    *completion_bridge_type = QUESTER_COMPLETION_OUTPUT;
    // TEMP
    completion_out_bridge->editor_node.bounds.x = 1500;
    completion_out_bridge->editor_node.bounds.y = 200;
    completion_out_bridge->editor_node.bounds.w = 300;
    completion_out_bridge->editor_node.bounds.h = 150;

    union quester_node *failure_out_bridge = quester_add_node(ctx);
    failure_out_bridge->node.type = QUESTER_BUILTIN_OUT_BRIDGE_TASK;
    enum out_connection_type *failure_bridge_type = ctx->static_state->static_node_data + 
        quester_find_static_data_offset(ctx, failure_out_bridge->node.id);
    *failure_bridge_type = QUESTER_FAILURE_OUTPUT;
    // TEMP
    failure_out_bridge->editor_node.bounds.x = 1500;
    failure_out_bridge->editor_node.bounds.y = 600;
    failure_out_bridge->editor_node.bounds.w = 300;
    failure_out_bridge->editor_node.bounds.h = 150;

    struct quester_container_task_data data;
    data.bridge_node_count = 3;
    data.bridge_node_ids[0] = in_bridge->node.id;
    data.bridge_node_ids[1] = completion_out_bridge->node.id;
    data.bridge_node_ids[2] = failure_out_bridge->node.id;

    data.contained_node_count = 3;
    data.contained_nodes[0] = in_bridge->node.id;
    data.contained_nodes[1] = completion_out_bridge->node.id;
    data.contained_nodes[2] = failure_out_bridge->node.id;
    *(struct quester_container_task_data*)(ctx->static_state->static_node_data + quester_find_static_data_offset(ctx, container_node->node.id)) = data;

    return container_node;
}

void quester_remove_node(struct quester_context *ctx, int id)
{
    struct node *node = &ctx->static_state->all_nodes[id].node;

    if (node->type == QUESTER_BUILTIN_CONTAINER_TASK)
    {
        // TODO: optimize this to not remove connections within the container
        struct quester_container_task_data *container = ctx->static_state->static_node_data + 
            quester_find_static_data_offset(ctx, id);
        for (int i = 0; i < container->contained_node_count; i++)
            quester_remove_node(ctx, container->contained_nodes[i]);
    }

    for (int i = 0; i < node->in_connection_count; i++)
        quester_remove_out_connections_to(ctx, node->in_connections[i].from_id, id);

    for (int i = 0; i < node->out_connection_count; i++)
        quester_remove_in_connections_from(ctx, node->out_connections[i].to_id, id);
        
    // So that once the new node takes this one's place, it doesn't have any connections
    node->in_connection_count = 0;
    node->out_connection_count = 0;

    // Return id to the pool
    int available_id_index = ctx->static_state->capacity - ctx->static_state->node_count--;
    ctx->static_state->available_ids[available_id_index] = node->id;
}

void quester_add_connection(struct quester_context *ctx, struct out_connection out, struct in_connection in)
{
    int from_node_index = quester_find_index(ctx, in.from_id);
    int to_node_index = quester_find_index(ctx, out.to_id);

    struct node *from_node = &ctx->static_state->all_nodes[from_node_index].node;
    struct node *to_node = &ctx->static_state->all_nodes[to_node_index].node;

    // TODO: would probably make sense to factor this out into separate functions, the calling function
    // will most likely know if either of the connected nodes is a container or not
    if (to_node->type == QUESTER_BUILTIN_CONTAINER_TASK)
    {
        struct quester_container_task_data *container = ctx->static_state->static_node_data + 
            quester_find_static_data_offset(ctx, out.to_id);

        for (int i = 0; i < container->bridge_node_count; i++)
        {
            int bridge_id = container->bridge_node_ids[i];

            struct node *to_bridge = &ctx->static_state->all_nodes[bridge_id].node;
            if (to_bridge->type != QUESTER_BUILTIN_IN_BRIDGE_TASK)
                continue;

            enum in_connection_type *accepted_bridge_type = ctx->static_state->static_node_data + 
                quester_find_static_data_offset(ctx, bridge_id);
            if (in.type != *accepted_bridge_type)
                continue;

            quester_add_connection(ctx, (struct out_connection) { out.type, bridge_id }, in);
        }
    }

    if(from_node->type == QUESTER_BUILTIN_CONTAINER_TASK)
    {
        struct quester_container_task_data *container = ctx->static_state->static_node_data + 
            quester_find_static_data_offset(ctx, in.from_id);

        for (int i = 0; i < container->bridge_node_count; i++)
        {
            int bridge_id = container->bridge_node_ids[i];

            struct node *to_bridge = &ctx->static_state->all_nodes[bridge_id].node;
            if (to_bridge->type != QUESTER_BUILTIN_OUT_BRIDGE_TASK)
                continue;

            enum out_connection_type *bridge_out_type = ctx->static_state->static_node_data + 
                quester_find_static_data_offset(ctx, bridge_id);
            if (out.type != *bridge_out_type)
                continue;

            quester_add_connection(ctx, out, (struct in_connection) { in.type, bridge_id });
        }
    }

    from_node->out_connections[from_node->out_connection_count++] = out;
    to_node->in_connections[to_node->in_connection_count++] = in;
}   

void quester_remove_out_connections_to(struct quester_context *ctx, int connection_owner_id, int to_id)
{
    struct node *node = &ctx->static_state->all_nodes[connection_owner_id].node;

    int initial_out_connection_count = node->out_connection_count;
    for (int i = 0; i < initial_out_connection_count; i++)
        if (node->out_connections[i].to_id == to_id)
        {
            node->out_connections[i] = node->out_connections[node->out_connection_count-- - 1];
            break;
        }
}

void quester_remove_in_connections_from(struct quester_context *ctx, int connection_owner_id, int from_id)
{
    struct node *node = &ctx->static_state->all_nodes[connection_owner_id].node;

    int initial_in_connection_count = node->in_connection_count;
    for (int i = 0; i < initial_in_connection_count; i++)
        if (node->in_connections[i].from_id == from_id)
        {
            node->in_connections[i] = node->in_connections[node->in_connection_count-- - 1];
            break;
        }
}