struct quester_context
{
    size_t size;
    bool execution_paused;

    struct quester_quest_definitions *static_state;
    struct quester_runtime_quest_data *dynamic_state;
};

struct quester_context *quester_init(int capacity);
void quester_free(struct quester_context *ctx);

void quester_run(struct quester_context *ctx);