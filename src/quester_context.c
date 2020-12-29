#define QUESTER_USE_LIBC
#ifdef QUESTER_USE_LIBC
#include <stdio.h>
struct quester_context *quester_init(int capacity)
{
    size_t static_state_size = sizeof(struct quester_game_definition) + 
        (sizeof(int) + sizeof(union quester_node) + quester_max_static_data_size()) * capacity;
    size_t dynamic_state_size = sizeof(struct quester_dynamic_state) +
        (sizeof(int) + sizeof(int) + sizeof(int) + sizeof(enum quester_control_flags) + 
         quester_max_dynamic_data_size()) * capacity;

    size_t total_ctx_size = sizeof(struct quester_context) + static_state_size + dynamic_state_size;
    struct quester_context *ctx = malloc(total_ctx_size);
    memset(ctx, 0, total_ctx_size);
    ctx->size = total_ctx_size;

    ctx->execution_paused = false;

    ctx->static_state = (void*)ctx + sizeof(struct quester_context);
    ctx->static_state->available_ids = (void*)ctx->static_state + 
        sizeof(struct quester_game_definition);
    ctx->static_state->initially_tracked_node_ids = (void*)ctx->static_state->available_ids + 
        sizeof(int) * capacity;
    ctx->static_state->all_nodes = (void*)ctx->static_state->initially_tracked_node_ids + 
        sizeof(int) * capacity;
    ctx->static_state->static_node_data = (void*)ctx->static_state->all_nodes + 
        sizeof(union quester_node) * capacity;
    ctx->static_state->size = static_state_size;
    ctx->static_state->capacity = capacity;
    ctx->static_state->initially_tracked_node_count = 0;
    ctx->static_state->node_count = 0;

    // fill in available_ids
    for (int i = 0; i < capacity; i++)
        ctx->static_state->available_ids[i] = capacity - i - 1;

    ctx->dynamic_state = (void*)ctx->static_state + static_state_size;
    ctx->dynamic_state->failed_node_ids = (void*)ctx->dynamic_state + 
        sizeof(struct quester_dynamic_state);
    ctx->dynamic_state->completed_node_ids = (void*)ctx->dynamic_state->failed_node_ids + 
        sizeof(int) * capacity;
    ctx->dynamic_state->ctl_flags = (void*)ctx->dynamic_state->completed_node_ids + 
        sizeof(int) * capacity;
    ctx->dynamic_state->tracked_node_ids = (void*)ctx->dynamic_state->ctl_flags + 
        sizeof(enum quester_control_flags) * capacity;
    ctx->dynamic_state->tracked_node_data = (void*)ctx->dynamic_state->tracked_node_ids + 
        sizeof(int) * capacity;
    ctx->dynamic_state->size = dynamic_state_size;
    ctx->dynamic_state->capacity = capacity;
    ctx->dynamic_state->failed_node_count = 0;
    ctx->dynamic_state->completed_node_count = 0;
    ctx->dynamic_state->node_count = 0;

    quester_fill_with_test_data(ctx);

    return ctx;
}

void quester_free(struct quester_context *ctx)
{
    free(ctx);
}

void quester_dump_static_bin(struct quester_context *ctx, const char *dir_path,
        const char *filename)
{
    char *filepath = malloc(sizeof(char) * (strlen(dir_path) + strlen(filename) + 6));
    filepath[0] = '\0';
    strcat(filepath, dir_path);
    strcat(filepath, filename);
    strcat(filepath, ".s_bin");

    FILE *f = fopen(filepath, "w");
    fwrite(ctx->static_state, ctx->static_state->size, 1, f);
    fclose(f);

    free(filepath);
}

void quester_load_static_bin(struct quester_context **ctx, const char *dir_path,
        const char *filename)
{
    char *filepath = malloc(sizeof(char) * (strlen(dir_path) + strlen(filename) + 6));
    filepath[0] = '\0';
    strcat(filepath, dir_path);
    strcat(filepath, filename);
    strcat(filepath, ".s_bin");

    FILE *f = fopen(filepath, "r");

    size_t static_state_size;
    fread(&static_state_size, sizeof(size_t), 1, f);

    if ((*ctx)->static_state->size < static_state_size)
    {
        size_t new_size = sizeof(struct quester_context) + static_state_size + 
            (*ctx)->dynamic_state->size;
        *ctx = realloc(*ctx, new_size);

        // move dynamic_state to give more space to static_state
        struct quester_dynamic_state *new_dynamic_state = (void*)*ctx + 
            sizeof(struct quester_context) + static_state_size;
        memmove(new_dynamic_state, (*ctx)->dynamic_state, (*ctx)->dynamic_state->size);
        (*ctx)->dynamic_state = new_dynamic_state;
    }
    struct quester_game_definition *static_state = (*ctx)->static_state;
    memset(static_state, 0, static_state_size);
    static_state->size = static_state_size;

    fread((void*)static_state + sizeof(size_t), static_state_size - sizeof(size_t), 1, f);

    static_state->available_ids = (void*)static_state + 
        sizeof(struct quester_game_definition); 
    static_state->initially_tracked_node_ids = (void*)static_state->available_ids + 
        sizeof(int) * static_state->capacity;
    static_state->all_nodes = (void*)static_state->initially_tracked_node_ids + 
        sizeof(int) * static_state->capacity;
    static_state->static_node_data = (void*)static_state->all_nodes + 
        sizeof(union quester_node) * static_state->capacity;

    quester_reset_dynamic_state(*ctx);
}

void quester_dump_dynamic_bin(struct quester_context *ctx, const char *dir_path,
        const char *filename)
{
    char *filepath = malloc(sizeof(char) * (strlen(dir_path) + strlen(filename) + 6));
    filepath[0] = '\0';
    strcat(filepath, dir_path);
    strcat(filepath, filename);
    strcat(filepath, ".d_bin");

    FILE *f = fopen(filepath, "w");
    fwrite(ctx->dynamic_state, ctx->dynamic_state->size, 1, f);
    fclose(f);

    free(filepath);
}

void quester_load_dynamic_bin(struct quester_context **ctx, const char *dir_path,
        const char *filename)
{
    char *filepath = malloc(sizeof(char) * (strlen(dir_path) + strlen(filename) + 6));
    filepath[0] = '\0';
    strcat(filepath, dir_path);
    strcat(filepath, filename);
    strcat(filepath, ".d_bin");

    FILE *f = fopen(filepath, "r");

    size_t dynamic_state_size;
    fread(&dynamic_state_size, sizeof(size_t), 1, f);

    if ((*ctx)->dynamic_state->size < dynamic_state_size)
    {
        size_t new_size = sizeof(struct quester_context) + (*ctx)->static_state->size + 
            dynamic_state_size;
        *ctx = realloc(*ctx, new_size);

        (*ctx)->dynamic_state = (void*)*ctx + sizeof(struct quester_context) + 
            (*ctx)->static_state->size;
    }
    struct quester_dynamic_state *dynamic_state = (*ctx)->dynamic_state;
    memset(dynamic_state, 0, dynamic_state_size);
    dynamic_state->size = dynamic_state_size;

    fread((void*)dynamic_state + sizeof(size_t), dynamic_state_size - sizeof(size_t), 1, f);

    (*ctx)->dynamic_state->failed_node_ids = (void*)(*ctx)->dynamic_state + 
        sizeof(struct quester_dynamic_state);
    (*ctx)->dynamic_state->completed_node_ids = (void*)(*ctx)->dynamic_state->failed_node_ids + 
        sizeof(int) * dynamic_state->capacity;
    (*ctx)->dynamic_state->ctl_flags = (void*)(*ctx)->dynamic_state->completed_node_ids + 
        sizeof(int) * dynamic_state->capacity;
    (*ctx)->dynamic_state->tracked_node_ids = (void*)(*ctx)->dynamic_state->ctl_flags + 
        sizeof(enum quester_control_flags) * dynamic_state->capacity;
    (*ctx)->dynamic_state->tracked_node_data = (void*)(*ctx)->dynamic_state->tracked_node_ids + 
        sizeof(int) * dynamic_state->capacity;
}

void quester_run(struct quester_context *ctx)
{
    if (ctx->execution_paused)
        return;

    struct quester_game_definition *static_state = ctx->static_state;
    struct quester_dynamic_state *dynamic_state = ctx->dynamic_state;

    const int starting_tracked_node_count = dynamic_state->tracked_node_count;
    for (int i = 0; i < starting_tracked_node_count; i++)
    {
        int node_id = dynamic_state->tracked_node_ids[i];
        int node_index = quester_find_index(ctx, node_id);
        if (node_index == -1)
            continue;
        struct node *node = &static_state->all_nodes[node_index].node;

        void *static_node_data = static_state->static_node_data + 
            quester_find_static_data_offset(ctx, node_id);
        void *dynamic_node_data = dynamic_state->tracked_node_data + 
            quester_find_dynamic_data_offset(ctx, node_id);

        enum quester_tick_result tick_result = quester_node_implementations[node->type].tick(ctx,
            node_id, static_node_data, dynamic_node_data);

        // TODO: finish builtin entrypoint implementation
        if (node->type == QUESTER_BUILTIN_ENTRYPOINT_TASK)
            tick_result = QUESTER_COMPLETED;

        if (tick_result == QUESTER_RUNNING)
            continue;

        // NOTE: for now assume that it's completed and not failed
        dynamic_state->completed_node_ids[dynamic_state->completed_node_count++] = node_id;

        dynamic_state->tracked_node_ids[i] = 
            dynamic_state->tracked_node_ids[--dynamic_state->tracked_node_count];
        for (int j = 0; j < node->out_connection_count; j++)
        {
            if (node->out_connections[j].type != quester_tick_type_to_out_connection_type[tick_result])
                continue;

            int dup = 0;
            for (int k = 0; k < dynamic_state->tracked_node_count; k++)
            {
                if (dynamic_state->tracked_node_ids[k] == node->out_connections[j].to_id)
                {
                    dup = 1;
                    break;
                }
            }

            int newly_tracked_node_id = quester_find_index(ctx, node->out_connections[j].to_id);
            struct node *newly_tracked_node = &static_state->all_nodes[newly_tracked_node_id].node;
            // err wrong?
            enum quester_control_flags ctl_flags = dynamic_state->ctl_flags[newly_tracked_node_id];

            if (ctl_flags & QUESTER_CTL_DISABLE_ACTIVATION)
                continue;

            if (!dup)
                dynamic_state->tracked_node_ids[dynamic_state->tracked_node_count++] = 
                    node->out_connections[j].to_id;

            // NOTE: for the time being, call on_start every time, even if it's a dup
            void *newly_tracked_static_node_data = static_state->static_node_data + 
                quester_find_static_data_offset(ctx, newly_tracked_node_id);
            void *newly_tracked_dynamic_node_data = dynamic_state->tracked_node_data + 
                quester_find_dynamic_data_offset(ctx, newly_tracked_node_id);
            quester_node_implementations[newly_tracked_node->type].on_start(ctx,
                    newly_tracked_node_id, newly_tracked_static_node_data,
                    newly_tracked_dynamic_node_data, node_id);
        }
    }
}

#endif
