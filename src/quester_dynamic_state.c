void quester_reset_dynamic_state(struct quester_context *ctx)
{
    ctx->dynamic_state->failed_node_count = 0;
    memset(ctx->dynamic_state->failed_node_ids, 0, sizeof(int) * ctx->dynamic_state->capacity);

    ctx->dynamic_state->completed_node_count = 0;
    memset(ctx->dynamic_state->completed_node_ids, 0, sizeof(int) * ctx->dynamic_state->capacity);

    memset(ctx->dynamic_state->ctl_flags, 0, sizeof(enum quester_control_flags) * 
            ctx->dynamic_state->capacity);

    ctx->dynamic_state->tracked_node_count = ctx->static_state->initially_tracked_node_count;
    memcpy(ctx->dynamic_state->tracked_node_ids, ctx->static_state->initially_tracked_node_ids,
            sizeof(int) * ctx->static_state->initially_tracked_node_count);

    memset(ctx->dynamic_state->tracked_node_data, 0, ctx->dynamic_state->node_count * 
            quester_max_dynamic_data_size());
}
