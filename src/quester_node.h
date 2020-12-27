struct quester_context;

struct node 
{
    int type;
    int id;

    char mission_id[32];
    char name[128];

    int in_node_count;
    int in_node_ids[100];
    int out_node_count;
    int out_node_ids[100];
};

struct editor_node
{
    struct node node;
    struct nk_rect bounds;
};

union quester_node
{
    struct editor_node editor_node;  
    struct node node;
};

union quester_node *quester_add_node(struct quester_context *ctx);
void quester_add_connection(struct quester_context *ctx, int from_node_id, int to_node_id);
