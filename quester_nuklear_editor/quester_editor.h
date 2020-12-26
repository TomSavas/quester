static const char quester_editor[] = "Quester mission editor";
int width = 1200, height = 800;
float camera_x = 0, camera_y = 0;

bool quester_ed_draw_task_prop_editor(struct nk_context *ctx, struct quester_context **q_ctx, int node_id)
{
    struct node *q_node = &(*q_ctx)->static_state->all_nodes[node_id];
    union quester_node *qq_node = &(*q_ctx)->static_state->all_nodes[node_id];

    char title[256];
    strcpy(title, q_node->name);
    strcat(title, " property editor");
    bool opened = false;
    if (nk_begin(ctx, title, nk_rect(0, 0, 300, 400), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE | NK_WINDOW_MOVABLE |
                            NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_SCALABLE | NK_WINDOW_CLOSABLE))
    {
        nk_layout_row_dynamic(ctx, 25, 2);
        nk_label(ctx, "Task Id:", NK_TEXT_LEFT);
        nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, q_node->mission_id, sizeof(q_node->mission_id), nk_filter_default);

        void *static_node_data = (*q_ctx)->static_state->static_node_data + 
            quester_find_static_data_offset(*q_ctx, q_node->id);
        void *dynamic_node_data = (*q_ctx)->dynamic_state->tracked_node_data + 
            quester_find_dynamic_data_offset(*q_ctx, q_node->id);
        quester_node_implementations[q_node->type].nk_prop_edit_display(ctx, *q_ctx, q_node->id, static_node_data, dynamic_node_data);
        quester_node_implementations[q_node->type].nk_display(ctx, *q_ctx, q_node->id, static_node_data, dynamic_node_data);
        opened = true;
    }
    nk_end(ctx);

    return opened;
}

void quester_ed_draw_contextual_menu(struct nk_context *ctx, struct quester_context **q_ctx)
{
    const struct nk_input *in = &ctx->input;

    if (nk_contextual_begin(ctx, 0, nk_vec2(270, 220), nk_window_get_bounds(ctx))) {
        nk_layout_row_dynamic(ctx, 25, 1);
        for (int i = 0; i < QUESTER_NODE_TYPE_COUNT; i++) 
        {
            if (nk_contextual_item_label(ctx, quester_node_implementations[i].name, NK_TEXT_LEFT))
            {
                union quester_node *node = quester_add_node(*q_ctx);
                node->node.type = i;
                strcpy(node->node.mission_id, node->node.type == QUESTER_BUILTIN_OR_TASK ? "OR" : "MIS_01");
                //strcpy(node->node.mission_id, "MIS_01");
                strcpy(node->node.name, "New ");
                strcat(node->node.name, quester_node_implementations[i].name);

                node->editor_node.bounds.w = 300;
                node->editor_node.bounds.h = 220;

                node->editor_node.bounds.x = in->mouse.pos.x + camera_x;
                node->editor_node.bounds.y = in->mouse.pos.y + camera_y - node->editor_node.bounds.h / 2;

                // TEMP: just for testing
                void *static_node_data = (*q_ctx)->static_state->static_node_data + quester_find_static_data_offset(ctx, node->node.id);
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
        nk_contextual_end(ctx);
    }
}

void quester_ed_draw_menu(struct nk_context *ctx, struct quester_context **q_ctx)
{
    nk_menubar_begin(ctx);
    nk_layout_row_begin(ctx, NK_STATIC, 25, 1);
    nk_layout_row_push(ctx, 120);
    if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(160, 200)))
    {
        static enum nk_collapse_states static_menu_state = NK_MAXIMIZED;
        if (nk_tree_state_push(ctx, NK_TREE_TAB, "Static State", &static_menu_state))
        {
            if (nk_menu_item_label(ctx, "Save", NK_TEXT_CENTERED))
            {
                quester_dump_static_bin(*q_ctx, "./", "save");
            }
            if (nk_menu_item_label(ctx, "Load", NK_TEXT_CENTERED))
            {
                quester_load_static_bin(q_ctx, "./", "save");
            }

            nk_tree_pop(ctx);
        }

        static enum nk_collapse_states dynamic_menu_state = NK_MAXIMIZED;
        if (nk_tree_state_push(ctx, NK_TREE_TAB, "Dynamic State", &dynamic_menu_state))
        {
            if (nk_menu_item_label(ctx, "Save", NK_TEXT_CENTERED))
            {
                quester_dump_dynamic_bin(*q_ctx, "./", "save");
            }
            if (nk_menu_item_label(ctx, "Load", NK_TEXT_CENTERED))
            {
                quester_load_dynamic_bin(q_ctx, "./", "save");
            }

            if (nk_menu_item_label(ctx, "Reset", NK_TEXT_CENTERED))
            {
                quester_reset_dynamic_state(*q_ctx);
            }

            nk_tree_pop(ctx);
        }

        if (nk_menu_item_label(ctx, (*q_ctx)->execution_paused ? "Unpause" : "Pause", NK_TEXT_LEFT))
        {
            (*q_ctx)->execution_paused = !(*q_ctx)->execution_paused;
        }

        nk_menu_end(ctx);
    }
}

void quester_ed_draw_grid(struct nk_command_buffer *canvas, struct nk_rect *size)
{
    static const float grid_size = 32.0f;
    const struct nk_color grid_color = nk_rgb(50, 50, 50);

    for (float x = (float)fmod(size->x - camera_x, grid_size); x < size->w; x += grid_size)
        nk_stroke_line(canvas, x+size->x, size->y, x+size->x, size->y+size->h, 1.0f, grid_color);

    for (float y = (float)fmod(size->y - camera_y, grid_size); y < size->h; y += grid_size)
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

void quester_draw_editor(struct nk_context *ctx, struct quester_context **q_ctx)
{
    struct nk_rect total_space;
    struct nk_command_buffer *canvas;
    const struct nk_input *in = &ctx->input;

    int mouse_over_node_id = -1;
    static int prop_editor_open_for[100];
    static int prop_editor_open_count = 0;
    if (nk_begin(ctx, quester_editor, nk_rect(0, 0, width, height), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE | NK_WINDOW_BACKGROUND))
    {
        quester_ed_draw_menu(ctx, q_ctx);

        struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
        struct nk_rect total_space = nk_window_get_content_region(ctx);

        nk_layout_space_begin(ctx, NK_STATIC, total_space.h, INT_MAX);
        {
            struct nk_rect size = nk_layout_space_bounds(ctx);

            quester_ed_draw_grid(canvas, &size);

            for (int i = 0; i < (*q_ctx)->static_state->node_count; i++)
            {
                struct node *q_node = &(*q_ctx)->static_state->all_nodes[i];
                union quester_node *qq_node = &(*q_ctx)->static_state->all_nodes[i];

                float x = qq_node->editor_node.bounds.x - camera_x;
                float y = qq_node->editor_node.bounds.y - camera_y;
                float w = qq_node->editor_node.bounds.w;
                float h = qq_node->editor_node.bounds.h;

                if (x+w < 0 || y+h < 0 || x > size.w || y > size.h)
                    continue;

                bool is_tracked = false;
                for (int j = 0; j < (*q_ctx)->dynamic_state->tracked_node_count; j++)
                    if ((*q_ctx)->dynamic_state->tracked_node_ids[j] == q_node->id)
                    {
                        is_tracked = true;
                        break;
                    }
                struct nk_panel *panel;

                if (is_tracked)
                    quester_ed_set_active_style(ctx);
                else
                    nk_style_default(ctx);

                struct nk_rect node_rect = nk_rect(x, y, w, h);
                nk_layout_space_push(ctx, node_rect);

                char title[1024];
                strcpy(title, q_node->mission_id);
                strcat(title, " (");
                strcat(title, quester_node_implementations[q_node->type].name);
                strcat(title, ")");

                if (nk_group_begin(ctx, title, NK_WINDOW_MOVABLE |
                            NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_SCALABLE | NK_WINDOW_SCROLL_AUTO_HIDE))
                {
                    panel = nk_window_get_panel(ctx);

                    void *static_node_data = (*q_ctx)->static_state->static_node_data + 
                        quester_find_static_data_offset(*q_ctx, q_node->id);
                    void *dynamic_node_data = (*q_ctx)->dynamic_state->tracked_node_data + 
                        quester_find_dynamic_data_offset(*q_ctx, q_node->id);
                    quester_node_implementations[q_node->type].nk_display(ctx, *q_ctx, q_node->id, static_node_data, dynamic_node_data);
                    
                    nk_group_end(ctx);
                }

                // input circle
                struct nk_rect circle = nk_rect(x, y + h * 0.8, 10, 10);
                nk_fill_circle(canvas, circle, nk_rgb(100, 100, 100));

                // output on failure circle
                circle = nk_rect(x + w - 1, y + h, 10, 10);
                nk_fill_circle(canvas, circle, nk_rgb(255, 100, 100));

                // output on completion circle
                circle = nk_rect(x + w - 1, y + h * 0.8, 10, 10);
                nk_fill_circle(canvas, circle, nk_rgb(100, 255, 100));

                if (mouse_over_node_id == -1 && nk_input_is_mouse_hovering_rect(in, nk_layout_space_rect_to_screen(ctx, node_rect)))
                    mouse_over_node_id = q_node->id;

                static bool is_dragging_connection = false;
                static float drag_start_x;
                static float drag_start_y;
                static int dragged_from_id;
                // connecting
                if (!is_dragging_connection)
                {
                    if (nk_input_is_mouse_hovering_rect(in, circle) && nk_input_is_mouse_down(in, NK_BUTTON_LEFT)) {
                        is_dragging_connection = true;
                        dragged_from_id = q_node->id;

                        drag_start_x = circle.x + circle.w/2;
                        drag_start_y = circle.y + circle.h/2;
                    }
                }
                else 
                {
                    nk_stroke_curve(canvas, drag_start_x, drag_start_y, drag_start_x, drag_start_y,
                            in->mouse.pos.x, in->mouse.pos.y, in->mouse.pos.x, in->mouse.pos.y, 1.f, nk_rgb(100, 100, 100));

                    if (!nk_input_is_mouse_down(in, NK_BUTTON_LEFT))
                    {
                        is_dragging_connection = false;

                        for (int j = 0; j < (*q_ctx)->static_state->node_count; j++) 
                        {
                            struct editor_node *n = &(*q_ctx)->static_state->all_nodes[j].editor_node;
                            struct nk_rect would_be_input_circle = nk_rect(n->bounds.x - camera_x, n->bounds.y - camera_y + n->bounds.h  * 0.8, 10, 10);
                            if (nk_input_is_mouse_hovering_rect(in, would_be_input_circle))
                            {
                                printf("connecting %d with %d\n", dragged_from_id, (*q_ctx)->static_state->all_nodes[j].node.id);
                                quester_add_connection(*q_ctx, dragged_from_id, (*q_ctx)->static_state->all_nodes[j].node.id);
                                break;
                            }
                        }
                    }
                }

                // Draw connections
                for (int j = 0; j < q_node->out_node_count; j++)
                {
                    union quester_node *target_node = &(*q_ctx)->static_state->all_nodes[q_node->out_node_ids[j]];
                    float output_node_x = x + w + 4;
                    float output_node_y = y + h * 0.8 + 4;
                    float target_node_x = target_node->editor_node.bounds.x - camera_x + 4;
                    float target_node_y = target_node->editor_node.bounds.y - camera_y + target_node->editor_node.bounds.h * 0.8 + 4;
                    nk_stroke_curve(canvas,
                            output_node_x, output_node_y, output_node_x, output_node_y,
                            target_node_x, target_node_y, target_node_x, target_node_y, 1.f, nk_rgb(100, 100, 100));
                }

                struct nk_rect bounds = nk_layout_space_rect_to_local(ctx, panel->bounds);
                bounds.x += camera_x;
                bounds.y += camera_y;
                qq_node->editor_node.bounds = bounds;
            }
            nk_style_default(ctx);

            if (mouse_over_node_id == -1)
                quester_ed_draw_contextual_menu(ctx, q_ctx);
            else
            {
                if (nk_contextual_begin(ctx, 0, nk_vec2(200, 220), nk_window_get_bounds(ctx))) {
                    nk_layout_row_dynamic(ctx, 25, 1);

                    if (nk_contextual_item_label(ctx, "Properties", NK_TEXT_LEFT))
                    {
                        bool exists = false;
                        for (int i = 0; i < prop_editor_open_count && !exists; i++)
                            exists |= prop_editor_open_for[i] == mouse_over_node_id;

                        if (!exists)
                            prop_editor_open_for[prop_editor_open_count++] = mouse_over_node_id;
                    }

                    if (nk_contextual_item_label(ctx, "Delete", NK_TEXT_LEFT))
                    {
                    }

                    nk_contextual_end(ctx);
                }
            }
        }
        nk_layout_space_end(ctx);

        // panning
        if (mouse_over_node_id == -1 && nk_input_is_mouse_hovering_rect(in, nk_window_get_bounds(ctx)) &&
            nk_input_is_mouse_down(in, NK_BUTTON_RIGHT)) {
            camera_x -= in->mouse.delta.x;
            camera_y -= in->mouse.delta.y;
        }
    }
    nk_end(ctx);

    {
        int retained_prop_editors[100];
        int retained_prop_count = 0;
        for (int i = 0; i < prop_editor_open_count; i++)
        {
            if (quester_ed_draw_task_prop_editor(ctx, q_ctx, prop_editor_open_for[i]))
                retained_prop_editors[retained_prop_count++] = prop_editor_open_for[i];
        }
        memcpy(prop_editor_open_for, retained_prop_editors, sizeof(int) * 100);
        prop_editor_open_count = retained_prop_count;
    }
}
