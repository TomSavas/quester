struct quester_context
{
    size_t size;
    bool execution_paused;

    struct quester_game_definition *static_state;
    struct quester_dynamic_state *dynamic_state;
};

struct quester_context *quester_init(int capacity);
void quester_free(struct quester_context *ctx);

void quester_dump_static_bin(struct quester_context *ctx, const char *dir_path,
        const char *filename);
// Resets dynamic state
void quester_load_static_bin(struct quester_context **ctx, const char *dir_path,
        const char *filename);
void quester_dump_dynamic_bin(struct quester_context *ctx, const char *dir_path,
        const char *filename);
void quester_load_dynamic_bin(struct quester_context **ctx, const char *dir_path,
        const char *filename);

void quester_run(struct quester_context *ctx);
