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

// TEMP:
int quester_find_tracked_owning_container(struct quester_context *ctx, int node_id)
{
    struct quester_game_definition *static_state = ctx->static_state;
    struct quester_dynamic_state *dynamic_state = ctx->dynamic_state;

    for (int i = 0; i < dynamic_state->tracked_node_count; i++)
    {
        int container_id = dynamic_state->tracked_node_ids[i];
        struct node *container = &static_state->all_nodes[container_id].node;

        if (container->type != QUESTER_BUILTIN_CONTAINER_TASK)
            continue;

        struct quester_container_task_data *container_data = static_state->static_node_data + 
            quester_find_static_data_offset(ctx, container_id);

        for (int j = 0; j < container_data->contained_node_count; j++)
            if (container_data->contained_nodes[j] == node_id)
                return container_id;
    }

    return -1;
}

struct quester_activation_result quester_activate_node(struct quester_context *ctx,
    struct in_connection in, int id)
{
    struct quester_game_definition *static_state = ctx->static_state;
    struct quester_dynamic_state *dynamic_state = ctx->dynamic_state;

    struct node *node = &static_state->all_nodes[id].node;

    // No repeated starts
    for (int i = 0; i < dynamic_state->tracked_node_count; i++)
        if (dynamic_state->tracked_node_ids[i] == id)
            return (struct quester_activation_result) { 0 };

    // Need a better solution, or tidy up dynamic flags
    enum quester_control_flags ctl_flags = dynamic_state->ctl_flags[id];
    if (ctl_flags & QUESTER_CTL_DISABLE_ACTIVATION)
        return (struct quester_activation_result) { 0 };

    void *static_data = static_state->static_node_data + quester_find_static_data_offset(ctx, id);
    void *dynamic_data = dynamic_state->tracked_node_data + 
        quester_find_dynamic_data_offset(ctx, id);
    struct quester_activation_result activator_result = 
        quester_node_implementations[node->type].activator(ctx, id, static_data, dynamic_data, &in);

    return activator_result;
}

void quester_activate_node_recurse(struct quester_context *ctx, struct in_connection in, int id,
    int *nodes_to_add_count, int *nodes_to_add)
{
    struct quester_activation_result res = quester_activate_node(ctx, in, id);

    if (res.flags & QUESTER_ACTIVATE)
        nodes_to_add[(*nodes_to_add_count)++] = id;

    if (res.flags & QUESTER_FORWARD_CONNECTIONS_TO_IDS)
        for (int i = 0; i < res.id_count; i++)
            quester_activate_node_recurse(ctx, in, res.ids[i], nodes_to_add_count, nodes_to_add);
}

void quester_run(struct quester_context *ctx)
{
    if (ctx->execution_paused)
        return;

    struct quester_game_definition *static_state = ctx->static_state;
    struct quester_dynamic_state *dynamic_state = ctx->dynamic_state;

    int nodes_to_remove_count = 0;
    int nodes_to_remove[1024];
    int nodes_to_add_count = 0;
    int nodes_to_add[1024];

    for (int i = 0; i < dynamic_state->tracked_node_count; i++)
    {
        int node_id = dynamic_state->tracked_node_ids[i];
        struct quester_tick_result tick_result;

        bool processing_container = false;

        do 
        {
            struct node *node = &static_state->all_nodes[node_id].node;

            void *static_node_data = static_state->static_node_data + 
                quester_find_static_data_offset(ctx, node_id);
            void *dynamic_node_data = dynamic_state->tracked_node_data + 
                quester_find_dynamic_data_offset(ctx, node_id);

            //  TODO: clean up this processing_container mess
            if (!processing_container)
                tick_result = quester_node_implementations[node->type].tick(ctx,
                    node_id, static_node_data, dynamic_node_data);

            if (!processing_container && tick_result.flags & QUESTER_STILL_RUNNING)
                continue;

            // NOTE: for now assume that it's completed and not failed
            dynamic_state->completed_node_ids[dynamic_state->completed_node_count++] = node_id;
            nodes_to_remove[nodes_to_remove_count++] = node_id;

            for (int j = 0; j < node->out_connection_count; j++)
            {
                if (node->out_connections[j].type != tick_result.out_connection_to_trigger)
                    continue;

                // TEMP
                struct in_connection in = { QUESTER_ACTIVATION_INPUT, node_id };
                quester_activate_node_recurse(ctx, in, node->out_connections[j].to_id,
                    &nodes_to_add_count, nodes_to_add);
            }

            node_id = quester_find_tracked_owning_container(ctx, node_id);
            processing_container = true;
        } while (tick_result.flags & QUESTER_FORWARD_TO_CONTAINER_OUTPUT && node_id != -1);
    }

    for (int i = 0; i < nodes_to_remove_count; i++)
    {
        for (int j = 0; j < dynamic_state->tracked_node_count; j++)
        {
            if (nodes_to_remove[i] == dynamic_state->tracked_node_ids[j])
            {
                dynamic_state->tracked_node_ids[j] = dynamic_state->tracked_node_ids[--dynamic_state->tracked_node_count];
                break;
            }
        }
    }

    for (int i = 0; i < nodes_to_add_count; i++)
        dynamic_state->tracked_node_ids[dynamic_state->tracked_node_count++] = nodes_to_add[i];
}

#if 0
void quester_run(struct quester_context *ctx)
{
    if (ctx->execution_paused)
        return;

    struct quester_game_definition *static_state = ctx->static_state;
    struct quester_dynamic_state *dynamic_state = ctx->dynamic_state;

    static int task_to_remove_hack = -1;

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

        struct quester_tick_result tick_result = quester_node_implementations[node->type].tick(ctx,
            node_id, static_node_data, dynamic_node_data);

        if (task_to_remove_hack == node_id)
        {
            // NOTE: for now assume that it's completed and not failed
            dynamic_state->completed_node_ids[dynamic_state->completed_node_count++] = node_id;

            dynamic_state->tracked_node_ids[i] = 
                dynamic_state->tracked_node_ids[--dynamic_state->tracked_node_count];
            task_to_remove_hack = -1;
            continue;
        }

        if (tick_result.flags & QUESTER_STILL_RUNNING)
            continue;

        // NOTE: for now assume that it's completed and not failed
        dynamic_state->completed_node_ids[dynamic_state->completed_node_count++] = node_id;

        dynamic_state->tracked_node_ids[i] = 
            dynamic_state->tracked_node_ids[--dynamic_state->tracked_node_count];

        // Again, stupidest shit in the world, just to test out if it works
        if (tick_result.flags & QUESTER_FORWARD_TO_CONTAINER_OUTPUT)
        {
            int containing_node_id = -1;
            for (int j = 0; j < dynamic_state->tracked_node_count; j++)
            {
                int container_id = dynamic_state->tracked_node_ids[j];
                struct node *container = &static_state->all_nodes[container_id].node;

                if (container->type != QUESTER_BUILTIN_CONTAINER_TASK)
                    continue;

                struct quester_container_task_data *container_data = static_state->static_node_data + 
                    quester_find_static_data_offset(ctx, container_id);

                for (int k = 0; k < container_data->contained_node_count; k++)
                    if (static_state->all_nodes[container_data->contained_nodes[k]].node.id == node_id)
                    {
                        containing_node_id = container_id;
                        break;
                    }

                if (containing_node_id != -1)
                {
                    node = container;
                    task_to_remove_hack = container_id;
                    break;
                }
            }
        }

        for (int j = 0; j < node->out_connection_count; j++)
        {
            if (node->out_connections[j].type != tick_result.out_connection_to_trigger)
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

            if (dup || ctl_flags & QUESTER_CTL_DISABLE_ACTIVATION)
                continue;

            struct in_connection *in_conn = NULL;
            for (int k = 0; k < newly_tracked_node->in_connection_count && in_conn == NULL; k++)
                if (newly_tracked_node->in_connections[k].from_id == node_id)
                    in_conn = &newly_tracked_node->in_connections[k];

            // NOTE: not true anymore now that we are redirecting connections from/to containers
            // If there is an out connection, there should also be an in connection
            //if (!in_conn)
            //    assert(false);
            struct in_connection connection;
            if (!in_conn)
            {
                connection.type = QUESTER_ACTIVATION_INPUT;
                connection.from_id = node_id;
                in_conn = &connection;
            }

            void *newly_tracked_static_node_data = static_state->static_node_data + 
                quester_find_static_data_offset(ctx, newly_tracked_node_id);
            void *newly_tracked_dynamic_node_data = dynamic_state->tracked_node_data + 
                quester_find_dynamic_data_offset(ctx, newly_tracked_node_id);
            struct quester_activation_result activator_result = 
                quester_node_implementations[newly_tracked_node->type].activator(ctx,
                    newly_tracked_node_id, newly_tracked_static_node_data,
                    newly_tracked_dynamic_node_data, in_conn);

            if (activator_result.flags & QUESTER_ACTIVATE)
                dynamic_state->tracked_node_ids[dynamic_state->tracked_node_count++] = 
                    node->out_connections[j].to_id;

            // TODO: fix this bullshit, should recursively run activators on those tasks
            if (activator_result.flags & QUESTER_FORWARD_CONNECTIONS_TO_IDS)
            {
                int dup = 0;
                for (int k = 0; k < activator_result.id_count; k++)
                {
                    for (int l = 0; l < dynamic_state->tracked_node_count; l++)
                        if (dynamic_state->tracked_node_ids[l] == activator_result.ids[k])
                        {
                            dup = 1;
                            break;
                        }

                    if (dup)
                        continue;

                    dynamic_state->tracked_node_ids[dynamic_state->tracked_node_count++] = 
                        activator_result.ids[k];

                    // TEMP: just for testing
                    struct in_connection fictional_connection;
                    void *newly_tracked_static_node_data = static_state->static_node_data + 
                        quester_find_static_data_offset(ctx, activator_result.ids[k]);
                    void *newly_tracked_dynamic_node_data = dynamic_state->tracked_node_data + 
                        quester_find_dynamic_data_offset(ctx, activator_result.ids[k]);
                    struct quester_activation_result activator_result = 
                        quester_node_implementations[newly_tracked_node->type].activator(ctx,
                            activator_result.ids[k], newly_tracked_static_node_data,
                            newly_tracked_dynamic_node_data, &fictional_connection);
                }
            }
        }
    }
}
#endif

#endif