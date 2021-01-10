void quester_reset_dynamic_state(struct quester_context *ctx)
{
    memset(ctx->dynamic_state->finishing_status, -1, sizeof(enum quester_out_connection_type) * ctx->dynamic_state->capacity);

    memset(ctx->dynamic_state->activation_flags, 0, sizeof(enum quester_activation_flags) * 
            ctx->dynamic_state->capacity);

    ctx->dynamic_state->tracked_node_count = ctx->static_state->initially_tracked_node_count;
    memcpy(ctx->dynamic_state->tracked_node_ids, ctx->static_state->initially_tracked_node_ids,
            sizeof(int) * ctx->static_state->initially_tracked_node_count);

    memset(ctx->dynamic_state->tracked_node_data, 0, ctx->dynamic_state->node_count * 
            quester_max_dynamic_data_size());
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
    struct quester_runtime_quest_data *dynamic_state = (*ctx)->dynamic_state;
    memset(dynamic_state, 0, dynamic_state_size);
    dynamic_state->size = dynamic_state_size;

    fread((void*)dynamic_state + sizeof(size_t), dynamic_state_size - sizeof(size_t), 1, f);

    (*ctx)->dynamic_state->finishing_status = (void*)(*ctx)->dynamic_state + 
        sizeof(struct quester_runtime_quest_data);
    (*ctx)->dynamic_state->activation_flags = (void*)(*ctx)->dynamic_state->finishing_status + 
        sizeof(enum quester_out_connection_type) * dynamic_state->capacity;
    (*ctx)->dynamic_state->tracked_node_ids = (void*)(*ctx)->dynamic_state->activation_flags + 
        sizeof(enum quester_activation_flags) * dynamic_state->capacity;
    (*ctx)->dynamic_state->tracked_node_data = (void*)(*ctx)->dynamic_state->tracked_node_ids + 
        sizeof(int) * dynamic_state->capacity;
}
