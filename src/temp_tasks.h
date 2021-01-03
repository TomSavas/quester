#include <stdbool.h>
#include "quester.h"

struct test_task
{
    char str[4096];
};

struct quester_activation_result test_task_activator(struct quester_context *ctx, int id, struct test_task *task, void *_,
        struct in_connection *triggering_connection) 
{
    return (struct quester_activation_result) { QUESTER_ACTIVATE };
}

enum quester_tick_result test_task_tick(struct quester_context *ctx, int id,
        struct test_task *task, void *_)
{
    printf("%s\n", task->str);
    return QUESTER_COMPLETED;
}

void test_task_nk_display(struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        struct test_task *task, void *_)
{
    //nk_layout_row_dynamic(ctx, 25, 1);
    //nk_label(ctx, q_node->name, NK_TEXT_LEFT);
}

void test_task_nk_edit_prop_display(struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        struct test_task *task, void *_)
{
}

struct timer_task
{
    int start_value;
    int end_value;
};

struct tracking_timer_task 
{
    int current_value;
};

struct quester_activation_result timer_task_activator(struct quester_context *ctx, int id, struct timer_task *task,
        struct tracking_timer_task *data, struct in_connection *triggering_connection)
{
    data->current_value = task->start_value;
    return (struct quester_activation_result) { QUESTER_ACTIVATE };
}

// Example of how you could use bool as the return type. Point is that 0 represents running
// and 1 - completed, so use bool if that's all you need
bool timer_task_tick(struct quester_context *ctx, int id, struct timer_task *task,
        struct tracking_timer_task *data)
{
    return ++data->current_value >= task->end_value;
}

void timer_task_nk_display(struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        struct timer_task *task, struct tracking_timer_task *data) 
{
    char current_value[128] = "Current timer value: ";
    char value[32];
    sprintf(value, "%d", data->current_value);
    strcat(current_value, value);

    nk_layout_row_dynamic(nk_ctx, 25, 1);
    nk_label(nk_ctx, current_value, NK_TEXT_LEFT);
}

void timer_task_nk_edit_prop_display(struct nk_context *nk_ctx, struct quester_context *ctx, int id,
        struct timer_task *task, struct tracking_timer_task *data) 
{
    char value[32];
    
    nk_layout_row_dynamic(nk_ctx, 25, 2);

    sprintf(value, "%d", task->start_value);
    nk_label(nk_ctx, "Initial timer value:", NK_TEXT_LEFT);
    nk_edit_string_zero_terminated(nk_ctx, NK_EDIT_FIELD, value, sizeof(value), nk_filter_decimal);
    task->start_value = atoi(value);

    sprintf(value, "%d", task->end_value);
    nk_label(nk_ctx, "End timer value:", NK_TEXT_LEFT);
    nk_edit_string_zero_terminated(nk_ctx, NK_EDIT_FIELD, value, sizeof(value), nk_filter_decimal);
    task->end_value = atoi(value);
}

#define QUESTER_USER_NODE_TYPE_ENUMS                                                               \
    TIMER_TASK,                                                                                    \
    TEST_TASK

#define QUESTER_USER_NODE_IMPLEMENTATIONS                                                          \
    QUESTER_NODE_IMPLEMENTATION(TIMER_TASK),                                                       \
    QUESTER_NODE_IMPLEMENTATION(TEST_TASK)                                                         

#define QUESTER_IMPLEMENT_USER_NODES                                                               \
    QUESTER_IMPLEMENT_NODE(TEST_TASK, struct test_task, void, test_task_activator,                  \
            test_task_tick, test_task_nk_display, test_task_nk_edit_prop_display)                  \
    QUESTER_IMPLEMENT_NODE(TIMER_TASK, struct timer_task, struct tracking_timer_task,              \
            timer_task_activator, timer_task_tick, timer_task_nk_display,                           \
            timer_task_nk_edit_prop_display)                                                       

