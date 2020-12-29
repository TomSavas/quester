void quester_or_task_on_start(struct quester_context *ctx, int id,
        struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data,
        int started_from_id)
{
    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    switch(static_node_data->behaviour)
    {
        case COMPLETE_INCOMING_INCOMPLETE_TASKS:
            for (int j = 0; j < node->in_connection_count; j++)
            {
                int in_id = node->in_connections[j].from_id;
                struct node *in_node = 
                    &ctx->static_state->all_nodes[quester_find_index(ctx, in_id)].node;

                // TEMPORARY
                quester_complete_task(ctx, in_id);

                // TODO: factor out tracked node removal
                for (int k = 0; k < ctx->dynamic_state->tracked_node_count; k++)
                    if (ctx->dynamic_state->tracked_node_ids[k] == in_id)
                    {
                        ctx->dynamic_state->tracked_node_ids[k] = 
                            ctx->dynamic_state->tracked_node_ids[--ctx->dynamic_state->tracked_node_count];
                        break;
                    }
            }
            break;
        case FAIL_INCOMING_INCOMPLETE_TASKS:
        //TODO
        case KILL_INCOMING_INCOMPLETE_TASKS:
            for (int j = 0; j < node->in_connection_count; j++)
            {
                // TODO: factor out tracked node removal

                int in_id = node->in_connections[j].from_id;
                for (int k = 0; k < ctx->dynamic_state->tracked_node_count; k++)
                    if (ctx->dynamic_state->tracked_node_ids[k] == in_id)
                    {
                        ctx->dynamic_state->tracked_node_ids[k] = 
                            ctx->dynamic_state->tracked_node_ids[--ctx->dynamic_state->tracked_node_count];
                        break;
                    }
            }
            break;
        case DO_NOTHING:
        default:
            break;
    }

    if (static_node_data->only_once)
        ctx->dynamic_state->ctl_flags[id] |= QUESTER_CTL_DISABLE_ACTIVATION;
}

void quester_or_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data)
{
    const char *actions[] =  { "do nothing", "complete", "fail", "kill" };

    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "Incoming incomplete task action:", NK_TEXT_LEFT);
    static_node_data->behaviour = nk_combo(nk_ctx, actions, sizeof(actions) / sizeof(char*), static_node_data->behaviour, 25,
            nk_vec2(200, 200));

    // To rid of the compile error
    nk_bool only_once = static_node_data->only_once;
    // NOTE: this shit is utterly stupid, the checkbox is inverted
    nk_checkbox_label(nk_ctx, "Activate only once", &only_once);
    static_node_data->only_once = only_once;
}

enum quester_tick_result quester_or_task_tick(struct quester_context *ctx, int id,
        struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data)
{
    return QUESTER_COMPLETED;
}

void quester_or_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, struct quester_or_task_data *static_node_data,
        struct quester_or_task_dynamic_data *data)
{
}

void quester_and_task_on_start(struct quester_context *ctx, int id, void *_,
        struct quester_and_task_dynamic_data *data, int started_from_id)
{
    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    bool duplicate = false;
    for (int i = 0; i < data->completed_dependency_count; i++)
    {
        if (data->completed_dependencies[i] == started_from_id)
            duplicate = true;
    }

    if (!duplicate)
    {
        data->completed_dependencies[data->completed_dependency_count++] = started_from_id;
        printf("completed: %d\n", started_from_id);
    }
    else
    {
        printf("duplicate: %d\n", started_from_id);
    }
    
    data->all_dependencies_completed = data->completed_dependency_count == node->in_connection_count;
}

enum quester_tick_result quester_and_task_tick(struct quester_context *ctx, int id, void *_,
        struct quester_and_task_dynamic_data *data)
{
    return data->all_dependencies_completed;
}

void quester_and_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        void *_, struct quester_and_task_dynamic_data *data)
{
}

void quester_and_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, void *_, struct quester_and_task_dynamic_data *data)
{
}

void quester_placeholder_on_start(struct quester_context *ctx, int id,
        struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *dynamic_data, int started_from_id)
{
    dynamic_data->tick_result = QUESTER_RUNNING;
}

enum quester_tick_result quester_placeholder_tick(struct quester_context *ctx, int id,
        struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *dynamic_data)
{
    return dynamic_data->tick_result;
}

void quester_placeholder_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *dynamic_data)
{
    nk_layout_row_dynamic(nk_ctx, 25, 2);
    if (nk_button_label(nk_ctx, "Manual complete"))
    {
        dynamic_data->tick_result = QUESTER_COMPLETED;
    }

    if (nk_button_label(nk_ctx, "Manual failure"))
    {
        dynamic_data->tick_result = QUESTER_FAILED;
    }

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "DESCRIPTION", NK_TEXT_CENTERED);
    nk_layout_row_dynamic(nk_ctx, 1000, 1);
    // NOTE: instead of using nk_text_wrap we use this, to enable newlines and stuff
    nk_edit_string_zero_terminated(nk_ctx, NK_EDIT_BOX | NK_EDIT_READ_ONLY,
            static_data->description, sizeof(static_data->description), nk_filter_default);
}

void quester_placeholder_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *data)
{
    nk_layout_row_dynamic(nk_ctx, 100, 1);
    nk_edit_string_zero_terminated(nk_ctx, NK_EDIT_BOX, static_data->description,
            sizeof(static_data->description), nk_filter_default);
}
