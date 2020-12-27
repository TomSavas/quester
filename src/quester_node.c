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

