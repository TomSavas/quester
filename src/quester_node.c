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

void quester_remove_node(struct quester_context *ctx, int id)
{
    struct node *node = &ctx->static_state->all_nodes[id].node;

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