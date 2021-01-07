static const char quester_editor[] = "Quester mission editor";
int width = 1200, height = 800;

struct quester_editor_context
{
    struct nk_context *nk_ctx;
    struct quester_context *q_ctx;

    // -1 represents that all nodes are displayed
    int currently_displayed_container_node_id;
    bool only_display_containers;

    int prop_editor_count;
    int prop_editor_open_for_ids[32];

    int width;
    int height;

    float camera_x;
    float camera_y;
};

bool quester_ed_draw_task_prop_editor(struct quester_editor_context *ctx, int node_id)
{
    union quester_node *qq_node = &ctx->q_ctx->static_state->all_nodes[node_id];
    struct node *q_node = &qq_node->node;
    struct editor_node *e_node = &qq_node->editor_node;

    char title[256];
    sprintf(title, "%s property editor (id=%d)", q_node->name, q_node->id);
    bool opened = false;
    struct nk_panel *panel;

    if (nk_begin(ctx->nk_ctx, title, e_node->prop_rect, NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE |
                NK_WINDOW_MOVABLE | NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_SCALABLE |
                NK_WINDOW_CLOSABLE))
    {
        panel = nk_window_get_panel(ctx->nk_ctx);

        const char *task_types[QUESTER_NODE_TYPE_COUNT];
        for (int i = 0; i < QUESTER_NODE_TYPE_COUNT; i++)
            task_types[i] = quester_node_implementations[i].name;
    
        // BUG: segfaults when switching from placeholder to or 
        // possibly interprets part of the text as a pointer which causes and invalid read
        // Could possibly clear the node definition once I do tihs, but I'd like to retain
        // the data in case of a failed switch. Can do that once undo/redo is implemented
        nk_layout_row_dynamic(ctx->nk_ctx, 25, 2);
        nk_label(ctx->nk_ctx, "Node type: ", NK_TEXT_LEFT);
        q_node->type = nk_combo(ctx->nk_ctx, task_types, QUESTER_NODE_TYPE_COUNT, q_node->type, 25, nk_vec2(200, 200));

        nk_label(ctx->nk_ctx, "Task Id:", NK_TEXT_LEFT);
        nk_edit_string_zero_terminated(ctx->nk_ctx, NK_EDIT_FIELD, q_node->mission_id,
                sizeof(q_node->mission_id), nk_filter_default);

        void *static_node_data = ctx->q_ctx->static_state->static_node_data + 
            quester_find_static_data_offset(ctx->q_ctx, q_node->id);
        void *dynamic_node_data = ctx->q_ctx->dynamic_state->tracked_node_data + 
            quester_find_dynamic_data_offset(ctx->q_ctx, q_node->id);
        quester_node_implementations[q_node->type].nk_prop_edit_display(ctx->nk_ctx, ctx->q_ctx, q_node->id,
                static_node_data, dynamic_node_data);
        quester_node_implementations[q_node->type].nk_display(ctx->nk_ctx, ctx->q_ctx, q_node->id,
                static_node_data, dynamic_node_data);
        opened = true;
    }
    nk_end(ctx->nk_ctx);

    if (panel)
        e_node->prop_rect = panel->bounds;

    return opened;
}

void quester_ed_draw_contextual_menu(struct quester_editor_context *ctx)
{
    const struct nk_input *in = &ctx->nk_ctx->input;

    if (nk_contextual_begin(ctx->nk_ctx, 0, nk_vec2(270, 300), nk_window_get_bounds(ctx->nk_ctx))) {
        nk_layout_row_dynamic(ctx->nk_ctx, 25, 1);
        for (int i = 0; i < QUESTER_NODE_TYPE_COUNT; i++) 
        {
            if (nk_contextual_item_label(ctx->nk_ctx, quester_node_implementations[i].name, NK_TEXT_LEFT))
            {
                int x = in->mouse.pos.x + ctx->camera_x;
                int y = in->mouse.pos.y + ctx->camera_y;

                union quester_node *node;
                if (i == QUESTER_BUILTIN_CONTAINER_TASK)
                    node = quester_add_container_node(ctx->q_ctx, x, y);
                else
                {
                    node = quester_add_node(ctx->q_ctx);
                    if (ctx->currently_displayed_container_node_id != -1)
                    {
                        struct quester_container_task_data *container = ctx->q_ctx->static_state->static_node_data + 
                            quester_find_static_data_offset(ctx->q_ctx, ctx->currently_displayed_container_node_id);
                        container->contained_nodes[container->contained_node_count++] = node->node.id;
                    }
                }

                node->node.type = i;
                strcpy(node->node.mission_id,
                        node->node.type == QUESTER_BUILTIN_OR_TASK ? "OR" : "MIS_01");
                //strcpy(node->node.mission_id, "MIS_01");
                strcpy(node->node.name, "New ");
                strcat(node->node.name, quester_node_implementations[i].name);

                node->editor_node.bounds.w = 300;
                node->editor_node.bounds.h = 220;

                node->editor_node.bounds.x = x;
                node->editor_node.bounds.y = y;

                node->editor_node.prop_rect = nk_rect(0, 0, 300, 400);

                // TEMP: just for testing
                void *static_node_data = ctx->q_ctx->static_state->static_node_data + 
                    quester_find_static_data_offset(ctx->q_ctx, node->node.id);
                if (node->node.type == TIMER_TASK)
                {
                    struct timer_task *t = static_node_data;
                    t->start_value = 0;
                    t->end_value = 100;
                }
                else if (node->node.type == QUESTER_BUILTIN_OR_TASK)
                {
                    struct quester_or_task_data *t = static_node_data;
                    t->only_once = 0;
                }
            }
        }

        if (ctx->currently_displayed_container_node_id != -1 && nk_contextual_item_label(ctx->nk_ctx, "Exit container node", NK_TEXT_LEFT))
            ctx->currently_displayed_container_node_id = -1;

        nk_contextual_end(ctx->nk_ctx);
    }
}

void quester_ed_draw_menu(struct quester_editor_context *ctx)
{
    nk_menubar_begin(ctx->nk_ctx);
    nk_layout_row_begin(ctx->nk_ctx, NK_STATIC, 25, 2);
    nk_layout_row_push(ctx->nk_ctx, 70);
    if (nk_menu_begin_label(ctx->nk_ctx, "File", NK_TEXT_LEFT, nk_vec2(160, 200)))
    {
        static enum nk_collapse_states static_menu_state = NK_MAXIMIZED;
        if (nk_tree_state_push(ctx->nk_ctx, NK_TREE_TAB, "Static State", &static_menu_state))
        {
            if (nk_menu_item_label(ctx->nk_ctx, "Save", NK_TEXT_CENTERED))
            {
                quester_dump_static_bin(ctx->q_ctx, "./", "save");
            }
            if (nk_menu_item_label(ctx->nk_ctx, "Load", NK_TEXT_CENTERED))
            {
                quester_load_static_bin(&ctx->q_ctx, "./", "save");
            }

            nk_tree_pop(ctx->nk_ctx);
        }

        static enum nk_collapse_states dynamic_menu_state = NK_MAXIMIZED;
        if (nk_tree_state_push(ctx->nk_ctx, NK_TREE_TAB, "Dynamic State", &dynamic_menu_state))
        {
            if (nk_menu_item_label(ctx->nk_ctx, "Save", NK_TEXT_CENTERED))
            {
                quester_dump_dynamic_bin(ctx->q_ctx, "./", "save");
            }
            if (nk_menu_item_label(ctx->nk_ctx, "Load", NK_TEXT_CENTERED))
            {
                quester_load_dynamic_bin(&ctx->q_ctx, "./", "save");
            }

            if (nk_menu_item_label(ctx->nk_ctx, "Reset", NK_TEXT_CENTERED))
            {
                quester_reset_dynamic_state(ctx->q_ctx);
            }

            nk_tree_pop(ctx->nk_ctx);
        }

        if (nk_menu_item_label(ctx->nk_ctx, ctx->q_ctx->execution_paused ? "Unpause" : "Pause", NK_TEXT_LEFT))
        {
            ctx->q_ctx->execution_paused = !ctx->q_ctx->execution_paused;
        }

        nk_menu_end(ctx->nk_ctx);
    }

    if (nk_menu_begin_label(ctx->nk_ctx, "Settings", NK_TEXT_LEFT, nk_vec2(250, 100)))
    {
        nk_layout_row_dynamic(ctx->nk_ctx, 25, 1);
        if (nk_menu_item_symbol_label(ctx->nk_ctx, ctx->only_display_containers ? NK_SYMBOL_RECT_SOLID : NK_SYMBOL_RECT_OUTLINE, "Hide non-quest nodes", NK_TEXT_LEFT))
        {
            ctx->only_display_containers = !ctx->only_display_containers;
        }

        nk_menu_end(ctx->nk_ctx);
    }

    nk_menubar_end(ctx->nk_ctx);
}

void quester_ed_draw_grid(struct nk_command_buffer *canvas, struct nk_rect *size, struct quester_editor_context *ctx)
{
    static const float grid_size = 32.0f;
    const struct nk_color grid_color = nk_rgb(50, 50, 50);

    for (float x = (float)fmod(size->x - ctx->camera_x, grid_size); x < size->w; x += grid_size)
        nk_stroke_line(canvas, x+size->x, size->y, x+size->x, size->y+size->h, 1.0f, grid_color);

    for (float y = (float)fmod(size->y - ctx->camera_y, grid_size); y < size->h; y += grid_size)
        nk_stroke_line(canvas, size->x, y+size->y, size->x+size->w, y+size->y, 1.0f, grid_color);
}

void quester_ed_set_active_style(struct nk_context *ctx)
{
    struct nk_color table[NK_COLOR_COUNT];
    
    table[NK_COLOR_TEXT] = nk_rgba(70, 70, 70, 255);
    table[NK_COLOR_WINDOW] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_HEADER] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_BORDER] = nk_rgba(0, 0, 0, 255);
    table[NK_COLOR_BUTTON] = nk_rgba(185, 185, 185, 255);
    table[NK_COLOR_BUTTON_HOVER] = nk_rgba(170, 170, 170, 255);
    table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
    table[NK_COLOR_TOGGLE] = nk_rgba(150, 150, 150, 255);
    table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(120, 120, 120, 255);
    table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_SELECT] = nk_rgba(190, 190, 190, 255);
    table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_SLIDER] = nk_rgba(190, 190, 190, 255);
    table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(80, 80, 80, 255);
    table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
    table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(60, 60, 60, 255);
    table[NK_COLOR_PROPERTY] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_EDIT] = nk_rgba(150, 150, 150, 255);
    table[NK_COLOR_EDIT_CURSOR] = nk_rgba(0, 0, 0, 255);
    table[NK_COLOR_COMBO] = nk_rgba(175, 175, 175, 255);
    table[NK_COLOR_CHART] = nk_rgba(160, 160, 160, 255);
    table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
    table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba( 255, 0, 0, 255);
    table[NK_COLOR_SCROLLBAR] = nk_rgba(180, 180, 180, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(140, 140, 140, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(150, 150, 150, 255);
    table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
    table[NK_COLOR_TAB_HEADER] = nk_rgba(180, 180, 180, 255);
    nk_style_from_table(ctx, table);
}

void quester_draw_editor(struct quester_editor_context *ctx)
{
    struct nk_rect total_space;
    struct nk_command_buffer *canvas;
    const struct nk_input *in = &ctx->nk_ctx->input;

    bool targeted_for_deletion = false;
    int mouse_over_node_id = -1;
    if (nk_begin(ctx->nk_ctx, quester_editor, nk_rect(0, 0, width, height), NK_WINDOW_NO_SCROLLBAR |
                NK_WINDOW_TITLE | NK_WINDOW_BACKGROUND | NK_WINDOW_CLOSABLE))
    {
        struct nk_command_buffer *canvas = nk_window_get_canvas(ctx->nk_ctx);
        struct nk_rect total_space = nk_window_get_content_region(ctx->nk_ctx);

        quester_ed_draw_menu(ctx);

        nk_layout_space_begin(ctx->nk_ctx, NK_STATIC, total_space.h, INT_MAX);
        {
            struct nk_rect size = nk_layout_space_bounds(ctx->nk_ctx);

            quester_ed_draw_grid(canvas, &size, ctx);

            struct node *container_node;
            struct quester_container_task_data *container_data;
            int node_count;

            if (ctx->currently_displayed_container_node_id == -1)
            {
                node_count = ctx->q_ctx->static_state->node_count;
            }
            else
            {
                container_node = &ctx->q_ctx->static_state->all_nodes[ctx->currently_displayed_container_node_id].node;
                container_data = ctx->q_ctx->static_state->static_node_data + 
                    quester_find_static_data_offset(ctx->q_ctx, ctx->currently_displayed_container_node_id);

                node_count = container_data->contained_node_count;
            }


            for (int i = 0; i < node_count; i++)
            {
                int id;
                if (ctx->currently_displayed_container_node_id == -1)
                {
                    bool is_removed_node = false;
                    for (int j = 0; j < ctx->q_ctx->static_state->capacity - ctx->q_ctx->static_state->node_count; j++)
                        if (i == ctx->q_ctx->static_state->available_ids[j])
                        {
                            is_removed_node = true;
                            node_count++;
                            break;
                        }

                    id = i;

                    bool is_initially_tracked_node = false;
                    for (int j = 0; j < ctx->q_ctx->static_state->initially_tracked_node_count && !is_initially_tracked_node; j++)
                        is_initially_tracked_node = id == ctx->q_ctx->static_state->initially_tracked_node_ids[j];

                    if (is_removed_node ||
                        (ctx->q_ctx->static_state->all_nodes[id].node.type != QUESTER_BUILTIN_CONTAINER_TASK && ctx->only_display_containers && !is_initially_tracked_node))
                        continue;

                }
                else
                {
                    id = container_data->contained_nodes[i];
                }

                union quester_node *qq_node = &ctx->q_ctx->static_state->all_nodes[id];
                struct node *q_node = &qq_node->node;

                float x = qq_node->editor_node.bounds.x - ctx->camera_x;
                float y = qq_node->editor_node.bounds.y - ctx->camera_y;
                float w = qq_node->editor_node.bounds.w;
                float h = qq_node->editor_node.bounds.h;

                if (x+w < 0 || y+h < 0 || x > size.w || y > size.h)
                    continue;

                bool is_tracked = false;
                for (int j = 0; j < ctx->q_ctx->dynamic_state->tracked_node_count; j++)
                    if (ctx->q_ctx->dynamic_state->tracked_node_ids[j] == q_node->id)
                    {
                        is_tracked = true;
                        break;
                    }
                struct nk_panel *panel;

                if (is_tracked)
                    quester_ed_set_active_style(ctx->nk_ctx);
                else
                    nk_style_default(ctx->nk_ctx);

                struct nk_rect node_rect = nk_rect(x, y, w, h);
                nk_layout_space_push(ctx->nk_ctx, node_rect);

                char title[1024];
                sprintf(title, "%s %s (id=%d)", q_node->mission_id,
                    quester_node_implementations[q_node->type].name, q_node->id);

                if (nk_group_begin(ctx->nk_ctx, title, NK_WINDOW_MOVABLE | NK_WINDOW_BORDER | 
                            NK_WINDOW_TITLE | NK_WINDOW_SCALABLE | NK_WINDOW_SCROLL_AUTO_HIDE))
                {
                    panel = nk_window_get_panel(ctx->nk_ctx);

                    void *static_node_data = ctx->q_ctx->static_state->static_node_data + 
                        quester_find_static_data_offset(ctx->q_ctx, q_node->id);
                    void *dynamic_node_data = ctx->q_ctx->dynamic_state->tracked_node_data + 
                        quester_find_dynamic_data_offset(ctx->q_ctx, q_node->id);
                    quester_node_implementations[q_node->type].nk_display(ctx->nk_ctx, ctx->q_ctx, q_node->id,
                            static_node_data, dynamic_node_data);
                    
                    nk_group_end(ctx->nk_ctx);
                }

                // input circle
                struct nk_rect circle = nk_rect(x, y + h * 0.8, 10, 10);
                nk_fill_circle(canvas, circle, nk_rgb(100, 100, 100));

                // output on failure circle
                struct nk_rect failure_circle = nk_rect(x + w - 1, y + h, 10, 10);
                nk_fill_circle(canvas, failure_circle, nk_rgb(255, 100, 100));

                // output on completion circle
                struct nk_rect completion_circle = nk_rect(x + w - 1, y + h * 0.8, 10, 10);
                nk_fill_circle(canvas, completion_circle, nk_rgb(100, 255, 100));

                if (mouse_over_node_id == -1 && nk_input_is_mouse_hovering_rect(in,
                            nk_layout_space_rect_to_screen(ctx->nk_ctx, node_rect)))
                    mouse_over_node_id = q_node->id;

                static enum out_connection_type out_type = -1;
                static bool is_dragging_connection = false;
                static float drag_start_x;
                static float drag_start_y;
                static int dragged_from_id;
                // connecting
                if (!is_dragging_connection)
                {
                    out_type = -1;
                    struct nk_rect circle;
                    if (nk_input_is_mouse_hovering_rect(in, failure_circle))
                    {
                        out_type = QUESTER_FAILURE_OUTPUT;
                        circle = failure_circle;
                    }

                    if (nk_input_is_mouse_hovering_rect(in, completion_circle))
                    {
                        out_type = QUESTER_COMPLETION_OUTPUT;
                        circle = completion_circle;
                    }

                    if (out_type != -1 && nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
                        is_dragging_connection = true;
                        dragged_from_id = q_node->id;

                        drag_start_x = circle.x + circle.w/2;
                        drag_start_y = circle.y + circle.h/2;
                    }
                }
                else 
                {
                    nk_stroke_curve(canvas, drag_start_x, drag_start_y, drag_start_x, drag_start_y,
                            in->mouse.pos.x, in->mouse.pos.y, in->mouse.pos.x, in->mouse.pos.y,
                            1.f, nk_rgb(100, 100, 100));

                    if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
                    {
                        is_dragging_connection = false;

                        for (int j = 0; j < ctx->q_ctx->static_state->node_count; j++) 
                        {
                            struct editor_node *n = 
                                &ctx->q_ctx->static_state->all_nodes[j].editor_node;
                            struct nk_rect would_be_input_circle = nk_rect(n->bounds.x - ctx->camera_x,
                                    n->bounds.y - ctx->camera_y + n->bounds.h  * 0.8, 10, 10);
                            if (nk_input_is_mouse_hovering_rect(in, would_be_input_circle))
                            {
                                printf("connecting %d with %d\n", dragged_from_id,
                                        ctx->q_ctx->static_state->all_nodes[j].node.id);

                                // TODO: fix to work with other types
                                quester_add_connection(ctx->q_ctx, (struct out_connection) { out_type, ctx->q_ctx->static_state->all_nodes[j].node.id},
                                    (struct in_connection) { QUESTER_ACTIVATION_INPUT, dragged_from_id });
                                break;
                            }
                        }
                    }
                }

                // Draw connections
                for (int j = 0; j < q_node->out_connection_count; j++)
                {
                    union quester_node *target_node = 
                        &ctx->q_ctx->static_state->all_nodes[q_node->out_connections[j].to_id];

                    float output_node_x = x;
                    float output_node_y = y;
                    if (q_node->out_connections[j].type == QUESTER_FAILURE_OUTPUT)
                    {
                        output_node_x = x + w + 4;
                        output_node_y = y + h + 4;
                    }
                    else if (q_node->out_connections[j].type == QUESTER_COMPLETION_OUTPUT)
                    {
                        output_node_x = x + w + 4;
                        output_node_y = y + h * 0.8 + 4;
                    }
                    
                    float target_node_x = target_node->editor_node.bounds.x - ctx->camera_x + 4;
                    float target_node_y = target_node->editor_node.bounds.y - ctx->camera_y + 
                        target_node->editor_node.bounds.h * 0.8 + 4;
                    nk_stroke_curve(canvas,
                            output_node_x, output_node_y, output_node_x, output_node_y,
                            target_node_x, target_node_y, target_node_x, target_node_y, 1.f,
                            nk_rgb(100, 100, 100));
                }

                struct nk_rect bounds = nk_layout_space_rect_to_local(ctx->nk_ctx, panel->bounds);
                bounds.x += ctx->camera_x;
                bounds.y += ctx->camera_y;
                qq_node->editor_node.bounds = bounds;
            }
            nk_style_default(ctx->nk_ctx);

            if (mouse_over_node_id == -1)
                quester_ed_draw_contextual_menu(ctx);
            else
            {
                if (nk_contextual_begin(ctx->nk_ctx, 0, nk_vec2(200, 220), nk_window_get_bounds(ctx->nk_ctx))) 
                {
                    nk_layout_row_dynamic(ctx->nk_ctx, 25, 1);

                    if (nk_contextual_item_label(ctx->nk_ctx, "Properties", NK_TEXT_LEFT))
                    {
                        bool exists = false;
                        for (int i = 0; i < ctx->prop_editor_count && !exists; i++)
                            exists |= ctx->prop_editor_open_for_ids[i] == mouse_over_node_id;

                        if (!exists)
                            ctx->prop_editor_open_for_ids[ctx->prop_editor_count++] = mouse_over_node_id;
                    }

                    targeted_for_deletion = nk_contextual_item_label(ctx->nk_ctx, "Delete", NK_TEXT_LEFT);

                    if (ctx->q_ctx->static_state->all_nodes[mouse_over_node_id].node.type == QUESTER_BUILTIN_CONTAINER_TASK)
                    {
                        if (nk_contextual_item_label(ctx->nk_ctx, "Edit container", NK_TEXT_LEFT))
                        {
                            ctx->currently_displayed_container_node_id = mouse_over_node_id;
                        }
                    }

                    nk_contextual_end(ctx->nk_ctx);
                }
            }
        }

        // panning
        if (mouse_over_node_id == -1 && nk_input_is_mouse_hovering_rect(in,
                    nk_window_get_bounds(ctx->nk_ctx)) &&
            nk_input_is_mouse_down(in, NK_BUTTON_RIGHT)) {
            ctx->camera_x -= in->mouse.delta.x;
            ctx->camera_y -= in->mouse.delta.y;
        }

        char *current_quest = ctx->currently_displayed_container_node_id == -1 
            ? "Top-level"
            : ctx->q_ctx->static_state->all_nodes[ctx->currently_displayed_container_node_id].node.mission_id;
        int s = ctx->nk_ctx->style.font->height;
        ((struct nk_user_font*)ctx->nk_ctx->style.font)->height = 72;
        struct nk_rect r = nk_rect(10, 60, 1000, 1000);
        nk_draw_text(canvas, r, current_quest, strlen(current_quest), ctx->nk_ctx->style.font, nk_rgba(255, 255, 255, 100), nk_rgba(255, 255, 255, 100));
        ((struct nk_user_font*)ctx->nk_ctx->style.font)->height = s;

        nk_layout_space_end(ctx->nk_ctx);
    }
    nk_end(ctx->nk_ctx);

    {
        int retained_prop_editors[32];
        int retained_prop_count = 0;
        for (int i = 0; i < ctx->prop_editor_count; i++)
        {
            if (!targeted_for_deletion && quester_ed_draw_task_prop_editor(ctx, ctx->prop_editor_open_for_ids[i]))
                retained_prop_editors[retained_prop_count++] = ctx->prop_editor_open_for_ids[i];
        }
        memcpy(ctx->prop_editor_open_for_ids, retained_prop_editors, sizeof(retained_prop_editors));
        ctx->prop_editor_count = retained_prop_count;
    }

    if (targeted_for_deletion)
        quester_remove_node(ctx->q_ctx, mouse_over_node_id);
}