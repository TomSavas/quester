void quester_reset_dynamic_state(struct quester_context *ctx)
{
    memset(ctx->dynamic_state->finishing_status, -1, sizeof(enum out_connection_type) * ctx->dynamic_state->capacity);

    memset(ctx->dynamic_state->ctl_flags, 0, sizeof(enum quester_control_flags) * 
            ctx->dynamic_state->capacity);

    ctx->dynamic_state->tracked_node_count = ctx->static_state->initially_tracked_node_count;
    memcpy(ctx->dynamic_state->tracked_node_ids, ctx->static_state->initially_tracked_node_ids,
            sizeof(int) * ctx->static_state->initially_tracked_node_count);

    memset(ctx->dynamic_state->tracked_node_data, 0, ctx->dynamic_state->node_count * 
            quester_max_dynamic_data_size());
}
