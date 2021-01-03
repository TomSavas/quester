struct quester_activation_result quester_container_task_activator(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_, struct in_connection *triggering_connection)
{
    struct quester_activation_result result = { 0 };

    switch(triggering_connection->type)
    {
        case QUESTER_ACTIVATION_INPUT:
            result.flags = QUESTER_ACTIVATE | QUESTER_DISABLE_TICKING;
            result.id_count = static_node_data->bridge_node_count;
            memcpy(result.ids, static_node_data->bridge_node_ids,
                sizeof(int) * static_node_data->bridge_node_count);

            break;
        // TODO: Add lock/unlock inputs here
        /*
        case QUESTER_LOCK_INPUT:
            break;
        case QUESTER_UNLOCK_INPUT:
            break;
        */
        // To avoid compiler warning
        case QUESTER_INPUT_TYPE_COUNT:
        default:
            break;
    }

    return result;
}

void quester_container_task_display(struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_)
{
    nk_layout_row_dynamic(nk_ctx, 25, 1);

    char quest_title[256];
    strcpy(quest_title, "Quest: ");
    strcat(quest_title, ctx->static_state->all_nodes[id].node.mission_id);
    nk_label(nk_ctx, quest_title, NK_TEXT_LEFT);

    //nk_label(nk_ctx, "Accepted connections types", NK_TEXT_CENTERED);
    //char contained_node_title[128];
    //for (int i = 0; i < static_node_data->bridge_node_count; i++)
    //{
    //    if (static_node_data->bridge)
    //    strcpy(contained_node_title, "-");
    //    strcat(contained_node_title, ctx->static_state->all_nodes[static_node_data->contained_entrypoint_nodes[i]].node.mission_id);
    //    nk_label(nk_ctx, contained_node_title, NK_TEXT_CENTERED);
    //}

    //if (static_node_data->contained_node_count == 0)
    //{
    //    nk_label(nk_ctx, "NO CONTAINED NODES", NK_TEXT_CENTERED);
    //    return;
    //}

    //nk_label(nk_ctx, "Contained nodes:", NK_TEXT_LEFT);
    //for (int i = 0; i < static_node_data->contained_node_count; i++)
    //{
    //    strcpy(contained_node_title, "-");
    //    strcat(contained_node_title, ctx->static_state->all_nodes[static_node_data->contained_nodes[i]].node.mission_id);
    //    nk_label(nk_ctx, contained_node_title, NK_TEXT_CENTERED);
    //}
}

enum quester_tick_result quester_container_task_tick(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_)
{
    return QUESTER_RUNNING;
}

void quester_container_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_container_task_data *static_node_data, void *_)
{
}

struct quester_activation_result quester_or_task_activator(struct quester_context *ctx, int id,
    struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data,
    struct in_connection *triggering_connection)
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

    return (struct quester_activation_result) { QUESTER_ACTIVATE };
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

struct quester_activation_result quester_and_task_activator(struct quester_context *ctx, int id, void *_,
    struct quester_and_task_dynamic_data *data, struct in_connection *triggering_connection)
{
    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    bool duplicate = false;
    for (int i = 0; i < data->completed_dependency_count; i++)
    {
        if (data->completed_dependencies[i] == triggering_connection->from_id)
            duplicate = true;
    }

    if (!duplicate)
    {
        data->completed_dependencies[data->completed_dependency_count++] = 
            triggering_connection->from_id;
        printf("completed: %d\n", triggering_connection->from_id);
    }
    else
    {
        printf("duplicate: %d\n", triggering_connection->from_id);
    }
    
    data->all_dependencies_completed = data->completed_dependency_count == node->in_connection_count;

    return (struct quester_activation_result) { QUESTER_ACTIVATE };
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

struct quester_activation_result quester_placeholder_activator(struct quester_context *ctx, int id,
        struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *dynamic_data, struct in_connection *triggering_connection)
{
    dynamic_data->tick_result = QUESTER_RUNNING;

    return (struct quester_activation_result) { QUESTER_ACTIVATE };
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

struct quester_activation_result quester_entrypoint_activator(struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data, struct in_connection *triggering_connection)
{
    return (struct quester_activation_result) { QUESTER_ACTIVATE };
}

enum quester_tick_result quester_entrypoint_tick(struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data)
{
    return QUESTER_COMPLETED;
}

void quester_entrypoint_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data)
{

}

void quester_entrypoint_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, void *static_data, void *data)
{

}

struct quester_activation_result quester_in_bridge_activator(struct quester_context *ctx, int id,
    enum in_connection_type *static_data, void *dynamic_data, struct in_connection *triggering_connection)
{
    assert(triggering_connection->type == *static_data);

    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_DISABLE_TICKING };
}

enum quester_tick_result quester_in_bridge_tick(struct quester_context *ctx, int id,
    enum in_connection_type *static_data, void *dynamic_data)
{
    return QUESTER_COMPLETED;
}

void quester_in_bridge_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    enum in_connection_type *static_data, void *dynamic_data)
{
    const char *accepted_types[] =  { "QUESTER_ACTIVATION_INPUT" };

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "Accepted in connection type ", NK_TEXT_LEFT);
    *static_data = nk_combo(nk_ctx, accepted_types, sizeof(accepted_types) / sizeof(char*), *static_data, 25,
            nk_vec2(200, 200));
}

void quester_in_bridge_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, enum in_connection_type *static_data, void *data)
{

}

struct quester_activation_result quester_out_bridge_activator(struct quester_context *ctx, int id,
    enum out_connection_type *static_data, void *dynamic_data, struct in_connection *triggering_connection)
{
    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_DISABLE_TICKING };
}

enum quester_tick_result quester_out_bridge_tick(struct quester_context *ctx, int id,
    enum out_connection_type *static_data, void *dynamic_data)
{
    // TODO: INCORRECT - there should be a mapping from out_connection_type to quester_tick_result
    return QUESTER_COMPLETED;
}

void quester_out_bridge_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    enum out_connection_type *static_data, void *dynamic_data)
{
    const char *output_types[] =  { "QUESTER_COMPLETION_OUTPUT", "QUESTER_FAILURE_OUTPUT" };

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "Output type", NK_TEXT_LEFT);
    *static_data = nk_combo(nk_ctx, output_types, sizeof(output_types) / sizeof(char*), *static_data, 25,
            nk_vec2(200, 200));
}

void quester_out_bridge_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, enum out_connection_type *static_data, void *data)
{

}