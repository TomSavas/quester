struct quester_context;

enum in_connection_type
{
    QUESTER_ACTIVATION_INPUT = 0,
    QUESTER_TEST_INPUT,

    QUESTER_INPUT_TYPE_COUNT
};

struct in_connection
{
    enum in_connection_type type;
    int from_id;
};

enum out_connection_type
{
    QUESTER_COMPLETION_OUTPUT = 0,
    QUESTER_FAILURE_OUTPUT,

    QUESTER_OUTPUT_TYPE_COUNT
};

struct out_connection
{
    enum out_connection_type type;
    int to_id;
};

struct node 
{
    int type;
    int id;

    char mission_id[32];
    char name[128];
    
    int in_connection_count;
    struct in_connection in_connections[100];
    int out_connection_count;
    struct out_connection out_connections[100];
};

struct editor_node
{
    struct node node;
    struct nk_rect bounds;
    struct nk_rect prop_rect;
};

union quester_node
{
    struct editor_node editor_node;  
    struct node node;
};

union quester_node *quester_add_node(struct quester_context *ctx);
//union quester_node *quester_add_container_node(struct quester_context *ctx);
// TEMP
union quester_node *quester_add_container_node(struct quester_context *ctx, int x, int y);
void quester_remove_node(struct quester_context *ctx, int id);

void quester_add_connection(struct quester_context *ctx, struct out_connection out, struct in_connection in);
void quester_remove_out_connections_to(struct quester_context *ctx, int connection_owner_id, int to_id);
void quester_remove_in_connections_from(struct quester_context *ctx, int connection_owner_id, int from_id);