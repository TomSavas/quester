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
        struct quester_runtime_quest_data *new_dynamic_state = (void*)*ctx + 
            sizeof(struct quester_context) + static_state_size;
        memmove(new_dynamic_state, (*ctx)->dynamic_state, (*ctx)->dynamic_state->size);
        (*ctx)->dynamic_state = new_dynamic_state;
    }
    struct quester_quest_definitions *static_state = (*ctx)->static_state;
    memset(static_state, 0, static_state_size);
    static_state->size = static_state_size;

    fread((void*)static_state + sizeof(size_t), static_state_size - sizeof(size_t), 1, f);

    static_state->available_ids = (void*)static_state + 
        sizeof(struct quester_quest_definitions); 
    static_state->initially_tracked_node_ids = (void*)static_state->available_ids + 
        sizeof(int) * static_state->capacity;
    static_state->all_nodes = (void*)static_state->initially_tracked_node_ids + 
        sizeof(int) * static_state->capacity;
    static_state->static_node_data = (void*)static_state->all_nodes + 
        sizeof(union quester_node) * static_state->capacity;

    (*ctx)->dynamic_state->node_count = static_state->node_count;
    quester_reset_dynamic_state(*ctx);
}
