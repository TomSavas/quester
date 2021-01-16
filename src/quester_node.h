struct quester_context;

#define QUESTER_IN_CONNECTION_TYPES(F) \
    F(QUESTER_ACTIVATION_INPUT)

enum quester_in_connection_type
{
    QUESTER_IN_CONNECTION_TYPES(QUESTER_IDENTITY),

    QUESTER_INPUT_TYPE_COUNT
};

const char *quester_in_connection_type_names[QUESTER_INPUT_TYPE_COUNT] = 
{
    QUESTER_IN_CONNECTION_TYPES(QUESTER_STRINGIFY)
};

struct quester_in_connection
{
    enum quester_in_connection_type type;
    int from_id;
};

#define QUESTER_OUT_CONNECTION_TYPES(F) \
    F(QUESTER_COMPLETION_OUTPUT),       \
    F(QUESTER_FAILURE_OUTPUT),          \
    F(QUESTER_DEAD_OUTPUT)              

enum quester_out_connection_type
{
    QUESTER_OUT_CONNECTION_TYPES(QUESTER_IDENTITY),

    QUESTER_OUTPUT_TYPE_COUNT
};

const char *quester_out_connection_type_names[QUESTER_OUTPUT_TYPE_COUNT] = 
{
    QUESTER_OUT_CONNECTION_TYPES(QUESTER_STRINGIFY)
};

struct quester_out_connection
{
    enum quester_out_connection_type type;
    int to_id;
};

struct quester_connection
{
    struct quester_out_connection out;
    struct quester_in_connection in;
};

#define QUESTER_ENTRYPOINT_NODE_ID 0

//struct quester_default_node 
struct node
{
    int type;
    int id;

    char mission_id[32];
    char name[128];

    int owning_node_id;
    
    int in_connection_count;
    struct quester_in_connection in_connections[100];
    int out_connection_count;
    struct quester_out_connection out_connections[100];
};

//struct quester_node 
//{
//    #ifdef QUESTER_CUSTOM_NODE
//        QUESTER_CUSTOM_NODE node;
//    #else
//        struct node node;
//    #endif
//};

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

void quester_add_connection(struct quester_context *ctx, struct quester_out_connection out, struct quester_in_connection in);
void quester_remove_out_connections_to(struct quester_context *ctx, int connection_owner_id, int to_id);
void quester_remove_in_connections_from(struct quester_context *ctx, int connection_owner_id, int from_id);