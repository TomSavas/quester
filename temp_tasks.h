struct test_task
{
    char str[4096];
};

void test_task_on_start(struct quester_context *ctx, int id, struct test_task *task, void *_, int started_from_id) {}

enum quester_tick_result test_task_tick(struct quester_context *ctx, int id, struct test_task *task, void *_)
{
    printf("%s\n", task->str);
    return QUESTER_COMPLETED;
}

void test_task_nk_display(struct nk_context *nk_ctx, struct nk_context *ctx, int id, struct test_task *task, void *_)
{
    //nk_layout_row_dynamic(ctx, 25, 1);
    //nk_label(ctx, q_node->name, NK_TEXT_LEFT);
}

QUESTER_IMPLEMENT_NODE(TEST_TASK, struct test_task, void, test_task_on_start, test_task_tick, test_task_nk_display)

struct timer_task
{
    int start_value;
    int end_value;
};

struct tracking_timer_task 
{
    int current_value;
};

void timer_task_on_start(struct quester_context *ctx, int id, struct timer_task *task, struct tracking_timer_task *data, int started_from_id)
{
    data->current_value = task->start_value;
}

// Example of how you could use bool as the return type. Point is that 0 represents running
// and 1 - completed, so use bool if that's all you need
bool timer_task_tick(struct quester_context *ctx, int id, struct timer_task *task, struct tracking_timer_task *data)
{
    return data->current_value++ >= task->end_value;
}

void timer_task_nk_display(struct nk_context *nk_ctx, struct quester_context *ctx, int id, struct timer_task *task, struct tracking_timer_task *data) 
{
    //nk_layout_row_dynamic(ctx, 25, 1);
    //nk_label(ctx, q_node->name, NK_TEXT_LEFT);

    char current_value[128] = "Current timer value: ";
    char value[32];
    sprintf(value, "%d", data->current_value);
    strcat(current_value, value);

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, current_value, NK_TEXT_LEFT);
}

QUESTER_IMPLEMENT_NODE(TIMER_TASK, struct timer_task, struct tracking_timer_task, timer_task_on_start, timer_task_tick, timer_task_nk_display)
