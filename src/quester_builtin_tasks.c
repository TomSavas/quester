struct quester_activation_result quester_container_task_activator(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_,
    struct quester_connection *triggering_connection)
{
    struct quester_activation_result result = { 0 };

    switch(triggering_connection->in.type)
    {
        case QUESTER_ACTIVATION_INPUT:
            result.flags = QUESTER_ACTIVATE | QUESTER_DISABLE_TICKING | QUESTER_FORWARD_CONNECTIONS_TO_IDS;
            result.id_count = static_node_data->in_bridge_node_count;
            memcpy(result.ids, static_node_data->in_bridge_node_ids,
                sizeof(int) * static_node_data->in_bridge_node_count);

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

struct quester_activation_result quester_container_task_non_activator(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_,
    struct quester_connection *triggering_connection)
{
    return (struct quester_activation_result) { 0 };
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

struct quester_tick_result quester_container_task_tick(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_)
{
    return (struct quester_tick_result) { QUESTER_STILL_RUNNING, 0 };
}

void quester_container_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_container_task_data *static_node_data, void *_)
{
}

void record_dependency(struct quester_context *ctx, int id, struct quester_node_dependency_tracker *data,
    struct quester_connection *triggering_connection)
{
    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    bool duplicate = false;
    for (int i = 0; i < data->completed_dependency_count && !duplicate; i++)
        if (data->completed_dependencies[i] == triggering_connection->in.from_id)
            duplicate = true;

    if (!duplicate)
        data->completed_dependencies[data->completed_dependency_count++] = triggering_connection->in.from_id;

    data->all_dependencies_completed = data->completed_dependency_count == node->in_connection_count;
}


struct quester_activation_result quester_or_task_activator(struct quester_context *ctx, int id,
    struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data,
    struct quester_connection *triggering_connection)
{
    if (static_node_data->only_once && data->dependencies.all_dependencies_completed)
        return (struct quester_activation_result) { 0 };

    record_dependency(ctx, id, &data->dependencies, triggering_connection);
    data->has_succeeded_dependency = true;
    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_ALLOW_REPEATED_ACTIVATIONS };
}

struct quester_activation_result quester_or_task_non_activator(struct quester_context *ctx, int id,
    struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data,
    struct quester_connection *triggering_connection)
{
    if (static_node_data->only_once && data->dependencies.all_dependencies_completed)
        return (struct quester_activation_result) { 0 };

    record_dependency(ctx, id, &data->dependencies, triggering_connection);
    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_ALLOW_REPEATED_ACTIVATIONS };
}

struct quester_tick_result quester_or_task_tick(struct quester_context *ctx, int id,
    struct quester_or_task_data *static_node_data, struct quester_or_task_dynamic_data *data)
{
    if (!data->has_succeeded_dependency && !data->dependencies.all_dependencies_completed)
        return (struct quester_tick_result) { QUESTER_STILL_RUNNING };

    struct node *node = &ctx->static_state->all_nodes[id].node;
    struct quester_tick_result res =
        {
            .flags = static_node_data->behaviour == DO_NOTHING ? 0 : QUESTER_FORWARD_TICK_RESULT_TO_IDS,
            .out_connection_to_trigger = data->has_succeeded_dependency ? QUESTER_COMPLETION_OUTPUT : QUESTER_FAILURE_OUTPUT,
            .flags_to_forward = 0,
            .out_connections_to_forward_trigger = static_node_data->behaviour - 1,
            .id_count = node->in_connection_count 
        };

    for (int i = 0; i < node->in_connection_count; i++)
    {
        bool is_completed = false;
        for (int j = 0; j < data->dependencies.completed_dependency_count && !is_completed; j++)
            is_completed = node->in_connections[i].from_id == data->dependencies.completed_dependencies[j];

        if (!is_completed)
            res.ids[i] = node->in_connections[i].from_id;
    }

    return res;
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

void quester_or_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, struct quester_or_task_data *static_node_data,
        struct quester_or_task_dynamic_data *data)
{
}

struct quester_activation_result quester_and_task_activator(struct quester_context *ctx, int id, void *_,
    struct quester_and_task_dynamic_data *data, struct quester_connection *triggering_connection)
{
    record_dependency(ctx, id, &data->dependencies, triggering_connection);
    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_ALLOW_REPEATED_ACTIVATIONS };
}

struct quester_activation_result quester_and_task_non_activator(struct quester_context *ctx, int id, void *_,
    struct quester_and_task_dynamic_data *data, struct quester_connection *triggering_connection)
{
    record_dependency(ctx, id, &data->dependencies, triggering_connection);
    data->has_failed_dependency = true;
    return (struct quester_activation_result) { 0 };
}

struct quester_tick_result quester_and_task_tick(struct quester_context *ctx, int id, void *_,
    struct quester_and_task_dynamic_data *data)
{
    return (struct quester_tick_result) { QUESTER_STILL_RUNNING & !data->dependencies.all_dependencies_completed,
        data->has_failed_dependency ? QUESTER_FAILURE_OUTPUT : QUESTER_COMPLETION_OUTPUT };
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
        struct quester_placeholder_dynamic_data *dynamic_data, struct quester_connection *triggering_connection)
{
    dynamic_data->tick_result = (struct quester_tick_result) { QUESTER_STILL_RUNNING, QUESTER_COMPLETION_OUTPUT };

    return (struct quester_activation_result) { QUESTER_ACTIVATE };
}

struct quester_activation_result quester_placeholder_non_activator(struct quester_context *ctx, int id,
        struct quester_placeholder_static_data *static_data,
        struct quester_placeholder_dynamic_data *dynamic_data, struct quester_connection *triggering_connection)
{
    return (struct quester_activation_result) { 0 };
}

struct quester_tick_result quester_placeholder_tick(struct quester_context *ctx, int id,
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
        dynamic_data->tick_result = (struct quester_tick_result){ 0, QUESTER_COMPLETION_OUTPUT };
    }

    if (nk_button_label(nk_ctx, "Manual failure"))
    {
        dynamic_data->tick_result = (struct quester_tick_result){ 0, QUESTER_FAILURE_OUTPUT };
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

struct quester_activation_result quester_in_bridge_activator(struct quester_context *ctx, int id,
    enum quester_in_connection_type *static_data, void *dynamic_data,
    struct quester_connection *triggering_connection)
{
    assert(triggering_connection->in.type == *static_data);

    return (struct quester_activation_result) { QUESTER_ACTIVATE };
}

struct quester_activation_result quester_in_bridge_non_activator(struct quester_context *ctx, int id,
    enum quester_in_connection_type *static_data, void *dynamic_data,
    struct quester_connection *triggering_connection)
{
    return (struct quester_activation_result) { 0 };
}

struct quester_tick_result quester_in_bridge_tick(struct quester_context *ctx, int id,
    enum quester_in_connection_type *static_data, void *dynamic_data)
{
    return (struct quester_tick_result) { 0, QUESTER_COMPLETION_OUTPUT };
}

void quester_in_bridge_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    enum quester_in_connection_type *static_data, void *dynamic_data)
{
    const char *accepted_types[] =  { "QUESTER_ACTIVATION_INPUT" };

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "Accepted in connection type ", NK_TEXT_LEFT);
    *static_data = nk_combo(nk_ctx, accepted_types, sizeof(accepted_types) / sizeof(char*), *static_data, 25,
            nk_vec2(200, 200));
}

void quester_in_bridge_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, enum quester_in_connection_type *static_data, void *data)
{

}

struct quester_activation_result quester_out_bridge_activator(struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data,
    struct quester_connection *triggering_connection)
{
    return (struct quester_activation_result) { QUESTER_ACTIVATE };
}

struct quester_activation_result quester_out_bridge_non_activator(struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data,
    struct quester_connection *triggering_connection)
{
        return (struct quester_activation_result) { 0 };
}

struct quester_tick_result quester_out_bridge_tick(struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data)
{
    return (struct quester_tick_result) 
    { 
        .flags = QUESTER_FORWARD_TICK_RESULT_TO_IDS,
        .out_connection_to_trigger = static_data->type,
        .flags_to_forward = 0,
        .out_connections_to_forward_trigger = static_data->type,
        .id_count = 1,
        .ids = { ctx->static_state->all_nodes[id].node.owning_node_id }
    };
}

void quester_out_bridge_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data)
{
    const char *output_types[] =  { "QUESTER_COMPLETION_OUTPUT", "QUESTER_FAILURE_OUTPUT", "QUESTER_DEAD_OUTPUT" };

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "Output type", NK_TEXT_LEFT);
    static_data->type = nk_combo(nk_ctx, output_types, sizeof(output_types) / sizeof(char*), static_data->type, 25,
            nk_vec2(200, 200));

    const char *actions[] =  { "do nothing", "complete", "fail", "kill" };

    struct node *node = &ctx->static_state->all_nodes[id].node;

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, "Incoming incomplete task action:", NK_TEXT_LEFT);
    static_data->behaviour = nk_combo(nk_ctx, actions, sizeof(actions) / sizeof(char*), static_data->behaviour, 25,
            nk_vec2(200, 200));
}

void quester_out_bridge_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_out_bridge_data *static_data, void *data)
{

}