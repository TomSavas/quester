/* 
 * Contains definitions of all the nodes.
 *
 * If you are _not_ doing dynamic quest generation you should store this as a separate file because
 * the quest definitions will be the same for all save files in your game. 
 * However, if you are using quest template api to dynamically create quests you should consider
 * saving this along you player's save file.
*/
struct quester_quest_definitions
{
    size_t size;
    int capacity;

    // size = capacity - node_count
    int *available_ids;

    int initially_tracked_node_count;
    int *initially_tracked_node_ids;

    int node_count;
    union quester_node *all_nodes;
    void *static_node_data;
};

void quester_dump_static_bin(struct quester_context *ctx, const char *dir_path,
        const char *filename);
// Resets dynamic state
void quester_load_static_bin(struct quester_context **ctx, const char *dir_path,
        const char *filename);