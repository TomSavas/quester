/* 
 * Contains "dynamic" state of all the nodes - state that can change during gameplay.
 * This should be used as (along with) the player's save file to save/load quest state.
*/
// TODO: do some benchmarking, potentially more efficient to store AoS
// instead of the SoA like we're doing now
// TODO: also would be nice to not keep all of this in memory and maybe detect which quests 
// can be unloaded from memory if they can't be reached again
struct quester_runtime_quest_data
{
    size_t size;
    int capacity;

    int tracked_ticking_node_count;
    int tracked_non_ticking_node_count;
    // The array grows from both ends:
    //     The ticking nodes are added from left-to-right starting at the start of array
    //     The non_ticking nodes are added from right-to-left starting at the end of array
    int *tracked_node_ids;

    // debug/editor only
    enum quester_out_connection_type *finishing_status;
    // debug/editor only

    // TODO: split nodes into ticking/non_ticking/inactive(sorted by id for binary search) arrays
    // so that we can iterate through them without any issues
    int node_count;
    enum quester_activation_flags *activation_flags;
    void *tracked_node_data;
};

void quester_dump_dynamic_bin(struct quester_context *ctx, const char *dir_path,
        const char *filename);
void quester_load_dynamic_bin(struct quester_context **ctx, const char *dir_path,
        const char *filename);

void quester_reset_dynamic_state(struct quester_context *ctx);

enum quester_out_connection_type *quester_finishing_status(struct quester_context *ctx, int id);
void *quester_dynamic_data(struct quester_context *ctx, int id);