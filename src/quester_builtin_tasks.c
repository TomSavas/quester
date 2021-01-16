void record_dependency(struct quester_context *ctx, int id, struct quester_immediate_dependency_tracker *tracker,
    struct quester_connection *triggering_connection)
{
    int node_index = quester_find_index(ctx, id);
    struct node *node = &ctx->static_state->all_nodes[node_index].node;

    bool duplicate = false;
    for (int i = 0; i < tracker->completed_dependency_count && !duplicate; i++)
        if (tracker->completed_dependencies[i] == triggering_connection->in.from_id)
            duplicate = true;

    if (!duplicate)
        tracker->completed_dependencies[tracker->completed_dependency_count++] = triggering_connection->in.from_id;

    tracker->all_dependencies_completed = tracker->completed_dependency_count == node->in_connection_count;
}

void tracker_completion_settings_display(struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, const char *force_out_connections_checkbox_text)
{
    nk_layout_row_dynamic(nk_ctx, 25, 1);

    nk_checkbox_label(nk_ctx, force_out_connections_checkbox_text,
        (nk_bool*)&tracker_completion_settings->force_out_connections);
    if (tracker_completion_settings->force_out_connections)
        tracker_completion_settings->out_connections_to_force = nk_combo(nk_ctx,
            quester_out_connection_type_names,
            sizeof(quester_out_connection_type_names) / sizeof(char*),
            tracker_completion_settings->out_connections_to_force, 25,
            nk_vec2(200, 200));
}

struct quester_activation_result quester_container_task_activator(struct quester_context *ctx, int id,
    struct quester_container_task_data *static_node_data, void *_,
    struct quester_connection *triggering_connection)
{
    struct quester_activation_result result = { 0 };

    switch(triggering_connection->in.type)
    {
        case QUESTER_ACTIVATION_INPUT:
            result.flags = QUESTER_ACTIVATE | QUESTER_FORWARD_CONNECTIONS_TO_IDS;
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

struct quester_activation_result quester_or_task_activator(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_or_task_dynamic_data *data,
    struct quester_connection *triggering_connection)
{
    if (data->dependencies.all_dependencies_completed)
        return (struct quester_activation_result) { 0 };

    record_dependency(ctx, id, &data->dependencies, triggering_connection);
    data->has_succeeded_dependency = true;
    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_ALLOW_REPEATED_ACTIVATIONS };
}

struct quester_activation_result quester_or_task_non_activator(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_or_task_dynamic_data *data,
    struct quester_connection *triggering_connection)
{
    if (data->dependencies.all_dependencies_completed)
        return (struct quester_activation_result) { 0 };

    record_dependency(ctx, id, &data->dependencies, triggering_connection);
    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_ALLOW_REPEATED_ACTIVATIONS };
}

struct quester_tick_result quester_or_task_tick(struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_or_task_dynamic_data *data)
{
    if (!data->has_succeeded_dependency && !data->dependencies.all_dependencies_completed)
        return (struct quester_tick_result) { QUESTER_STILL_RUNNING };

    struct node *node = &ctx->static_state->all_nodes[id].node;

    struct quester_tick_result res =
        {
            .flags = 0,
            .flags_to_forward = 0,
            .id_count = 0
        };

    if (data->has_succeeded_dependency)
    {
        res.flags = QUESTER_FORWARD_TICK_RESULT_TO_IDS;
        res.out_connection_to_trigger = QUESTER_COMPLETION_OUTPUT;
        res.out_connections_to_forward_trigger = tracker_completion_settings->out_connections_to_force;
    }
    else
    {
        res.out_connection_to_trigger = QUESTER_FAILURE_OUTPUT;
    }

    for (int i = 0; i < node->in_connection_count; i++)
    {
        bool is_completed = false;
        for (int j = 0; j < data->dependencies.completed_dependency_count && !is_completed; j++)
            is_completed = node->in_connections[i].from_id == data->dependencies.completed_dependencies[j];

        if (!is_completed)
            res.ids[res.id_count++] = node->in_connections[i].from_id;
    }

    return res;
}

void quester_or_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_or_task_dynamic_data *data)
{
    tracker_completion_settings_display(nk_ctx, ctx, id, tracker_completion_settings, "Force output on incompleted incoming nodes once this completes");
}

void quester_or_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
        int id, struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings,
        struct quester_or_task_dynamic_data *data)
{
}

struct quester_activation_result quester_and_task_activator(struct quester_context *ctx, int id, struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings,
    struct quester_and_task_dynamic_data *data, struct quester_connection *triggering_connection)
{
    record_dependency(ctx, id, &data->dependencies, triggering_connection);
    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_ALLOW_REPEATED_ACTIVATIONS };
}

struct quester_activation_result quester_and_task_non_activator(struct quester_context *ctx, int id, struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings,
    struct quester_and_task_dynamic_data *data, struct quester_connection *triggering_connection)
{
    record_dependency(ctx, id, &data->dependencies, triggering_connection);
    data->has_failed_dependency = true;
    return (struct quester_activation_result) { QUESTER_ACTIVATE | QUESTER_ALLOW_REPEATED_ACTIVATIONS };
}

struct quester_tick_result quester_and_task_tick(struct quester_context *ctx, int id, struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings,
    struct quester_and_task_dynamic_data *data)
{
    if (!data->has_failed_dependency && !data->dependencies.all_dependencies_completed)
        return (struct quester_tick_result) { QUESTER_STILL_RUNNING };

    struct node *node = &ctx->static_state->all_nodes[id].node;

    struct quester_tick_result res =
        {
            .flags = 0,
            .flags_to_forward = 0,
            .id_count = 0
        };

    if (data->has_failed_dependency)
    {
        res.flags = QUESTER_FORWARD_TICK_RESULT_TO_IDS;
        res.out_connection_to_trigger = QUESTER_FAILURE_OUTPUT;
        res.out_connections_to_forward_trigger = tracker_completion_settings->out_connections_to_force;
    }
    else
    {
        res.out_connection_to_trigger = QUESTER_COMPLETION_OUTPUT;
    }

    for (int i = 0; i < node->in_connection_count; i++)
    {
        bool is_completed = false;
        for (int j = 0; j < data->dependencies.completed_dependency_count && !is_completed; j++)
            is_completed = node->in_connections[i].from_id == data->dependencies.completed_dependencies[j];

        if (!is_completed)
            res.ids[res.id_count++] = node->in_connections[i].from_id;
    }

    return res;
}

void quester_and_task_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_and_task_dynamic_data *data)
{
    tracker_completion_settings_display(nk_ctx, ctx, id, tracker_completion_settings, "Force output on incompleted incoming nodes once this fails");
}

void quester_and_task_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_immediate_dependency_tracker_completion_settings *tracker_completion_settings, struct quester_and_task_dynamic_data *data)
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
        .flags = QUESTER_KILL_CONTAINER_TRACKED_TASKS | QUESTER_FORWARD_TICK_RESULT_TO_IDS,
        .out_connection_to_trigger = static_data->represents_container_output,
        .flags_to_forward = 0,
        .out_connections_to_forward_trigger = static_data->represents_container_output,
        .id_count = 1,
        .ids = { ctx->static_state->all_nodes[id].node.owning_node_id }
    };
}

void quester_out_bridge_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    struct quester_out_bridge_data *static_data, void *dynamic_data)
{
    nk_layout_row_dynamic(nk_ctx, 25, 1);

    static_data->represents_container_output = nk_combo(nk_ctx,
        quester_out_connection_type_names,
        QUESTER_OUTPUT_TYPE_COUNT,
        static_data->represents_container_output, 25,
        nk_vec2(200, 200));
}

void quester_out_bridge_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, struct quester_out_bridge_data *static_data, void *data)
{

}

struct quester_activation_result quester_entrypoint_activator(struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data, struct quester_connection *triggering_connection)
{
    // For now we are adding this as a starting task, so it's already "activated"
    return (struct quester_activation_result) { 0 };
}

struct quester_activation_result quester_entrypoint_non_activator(struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data, struct quester_connection *triggering_connection)
{
    return (struct quester_activation_result) { 0 };
}

struct quester_tick_result quester_entrypoint_tick(struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data)
{
    return (struct quester_tick_result) { 0, QUESTER_COMPLETION_OUTPUT };
}

void quester_entrypoint_display (struct nk_context *nk_ctx, struct quester_context *ctx, int id,
    void *static_data, void *dynamic_data)
{

}

void quester_entrypoint_prop_edit_display (struct nk_context *nk_ctx, struct quester_context *ctx,
    int id, void *static_data, void *data)
{

}