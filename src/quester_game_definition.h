struct quester_game_definition
{
    size_t size;
    int capacity;

    int initially_tracked_node_count;
    int *initially_tracked_node_ids;

    int node_count;
    union quester_node *all_nodes;
    void *static_node_data;
};
