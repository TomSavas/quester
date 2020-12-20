static const char quester_editor[] = "Quester mission editor";
int width = 1200, height = 800;
float camera_x = 0, camera_y = 0;

void quester_ed_draw_contextual_menu(struct nk_context *ctx, struct quester_context **q_ctx)
{
    const struct nk_input *in = &ctx->input;

    if (nk_contextual_begin(ctx, 0, nk_vec2(100, 220), nk_window_get_bounds(ctx))) {
        nk_layout_row_dynamic(ctx, 25, 1);
        for (int i = 0; i < QUESTER_NODE_TYPE_COUNT; i++) 
        {
            if (nk_contextual_item_label(ctx, quester_node_implementations[i].name, NK_TEXT_LEFT))
            {
                union quester_node *node = quester_add_node(*q_ctx);
                node->node.type = i;
                strcpy(node->node.mission_id, "MIS_01");
                strcpy(node->node.name, "New ");
                strcat(node->node.name, quester_node_implementations[i].name);

                node->editor_node.bounds.w = 150;
                node->editor_node.bounds.h = 100;

                node->editor_node.bounds.x = in->mouse.pos.x + camera_x;
                node->editor_node.bounds.y = in->mouse.pos.y + camera_y - node->editor_node.bounds.h / 2;
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
        //nk_layout_row_dynamic(ctx, 25, 1);
        //if (nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT))
        //{
        //    struct quester_context *shrunken_ctx = quester_shrink(*q_ctx, false);
        //    quester_dump_bin(shrunken_ctx, "./quester_state.bin");
        //    quester_free(shrunken_ctx);
        //}
        //if (nk_menu_item_label(ctx, "Load", NK_TEXT_LEFT))
        //{
        //    quester_free(*q_ctx);
        //    *q_ctx = quester_load_bin("./quester_state.bin");
        //}

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

void quester_draw_editor(struct nk_context *ctx, struct quester_context **q_ctx)
{
    struct nk_rect total_space;
    struct nk_command_buffer *canvas;
    const struct nk_input *in = &ctx->input;

    if (nk_begin(ctx, quester_editor, nk_rect(0, 0, width, height), NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_TITLE))
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

                nk_layout_space_push(ctx, nk_rect(x, y, w, h));
                if (nk_group_begin(ctx, q_node->mission_id, NK_WINDOW_MOVABLE |
                            NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_SCALABLE))
                {
                    panel = nk_window_get_panel(ctx);

                    nk_layout_row_dynamic(ctx, 25, 1);
                    if (is_tracked)
                        nk_label_colored(ctx, q_node->name, NK_TEXT_LEFT, nk_rgba(255, 0, 0, 255));
                    else
                        nk_label(ctx, q_node->name, NK_TEXT_LEFT);

                    nk_group_end(ctx);
                }

                struct nk_rect bounds = nk_layout_space_rect_to_local(ctx, panel->bounds);
                bounds.x += camera_x;
                bounds.y += camera_y;
                // Why the fuck do I need this?
                bounds.h += 10;
                qq_node->editor_node.bounds = bounds;
            }

            quester_ed_draw_contextual_menu(ctx, q_ctx);
        }
        nk_layout_space_end(ctx);

        // panning
        if (nk_input_is_mouse_hovering_rect(in, nk_window_get_bounds(ctx)) &&
            nk_input_is_mouse_down(in, NK_BUTTON_RIGHT)) {
            camera_x -= in->mouse.delta.x;
            camera_y -= in->mouse.delta.y;
        }
    }
    nk_end(ctx);
}
