enum quester_control_flags
{
    QUESTER_CTL_DISABLE_ACTIVATION = 1
};

// TODO: needs a better name
// Contains tracking data for nodes. This would be saved along with 
// the player savefile. 
//
// Potentially could save all of the completed task state as well for 
// debugging purposes, maybe controlled with something like
// QUESTER_DEBUG_KEEP_COMPLETED_DYNAMIC_STATE ?
struct quester_dynamic_state
{
    size_t size;
    int capacity;

    int tracked_node_count;
    int *tracked_node_ids;

    // debug/editor only
    enum out_connection_type *finishing_status;
    // debug/editor only

    int node_count;
    enum quester_control_flags *ctl_flags;
    void *tracked_node_data;
};

void quester_reset_dynamic_state(struct quester_context *ctx);
