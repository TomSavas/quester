struct test_task
{
    char str[4096];
};

void test_task_on_start(struct test_task *task)
{
}

bool test_task_is_completed(struct test_task *task)
{
    printf("%s\n", task->str);
    return true;
}

void test_task_nk_display(struct test_task *task)
{
}

QUESTER_IMPLEMENT_NODE(TEST_TASK, struct test_task, test_task_on_start, test_task_is_completed, test_task_nk_display)

struct timer_task
{
    int start_value;
    int end_value;
};

static int timer_value;
void timer_task_on_start(struct timer_task *task)
{
    timer_value = task->start_value;
}

bool timer_task_is_completed(struct timer_task *task)
{
    return timer_value++ >= task->end_value;
}

void timer_task_nk_display(struct timer_task *task)
{
}

QUESTER_IMPLEMENT_NODE(TIMER_TASK, struct timer_task, timer_task_on_start, timer_task_is_completed, timer_task_nk_display)
