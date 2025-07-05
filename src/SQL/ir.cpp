#include <Core/util.hpp>

#include "ir.hpp"

using namespace ok::literals;

namespace xmdb::SQL {
template<typename T>
struct IRContractAdder {};

template<>
struct IRContractAdder<StringView> {
    static U32 add(IREmitter* emitter, StringView sv) {
        return emitter->add_string(sv);
    }
};

template<>
struct IRContractAdder<S64> {
    static U32 add(IREmitter* emitter, S64 x) {
        return emitter->add_int(x);
    }
};

template<>
struct IRContractAdder<TableSchema*> {
    static U32 add(IREmitter* emitter, TableSchema* schema) {
        return emitter->add_schema(schema);
    }
};

template<>
struct IRContractAdder<U32> {
    static U32 add(IREmitter* emitter, U32 ip) {
        OK_UNUSED(emitter);
        return ip;
    }
};

#define INSTR_VAR_0(name)                                                                                              \
    static U32 emit_##name(IREmitter* emitter, Token token) {                                                                 \
        emitter->tokens.push(token);                                                                                   \
        U32 var_name = emitter->gen_temp();                                                                            \
        IRInstruction instr;                                                                                           \
        instr.op = IRInstructionOperator_##name;                                                                       \
        instr.operand1 = var_name;                                                                                     \
        return emitter->add_instruction(instr);                                                                        \
    }

#define INSTR_VAR_1(name, t1)                                                                                          \
    static U32 emit_##name(IREmitter* emitter, Token token, t1 o1) {                                                          \
        emitter->tokens.push(token);                                                                                   \
        U32 var_name = emitter->gen_temp();                                                                            \
        IRInstruction instr;                                                                                           \
        instr.op = IRInstructionOperator_##name;                                                                       \
        instr.operand1 = var_name;                                                                                     \
        instr.operand2 = IRContractAdder<t1>::add(emitter, o1);                                                         \
        return emitter->add_instruction(instr);                                                                        \
    }

#define INSTR_VAR_2(name, t1, t2)                                                                                      \
    static U32 emit_##name(IREmitter* emitter, Token token, t1 o1, t2 o2) {                                                   \
        emitter->tokens.push(token);                                                                                   \
        U32 var_name = emitter->gen_temp();                                                                            \
        IRInstruction instr;                                                                                           \
        instr.op = IRInstructionOperator_##name;                                                                       \
        instr.operand1 = var_name;                                                                                     \
        instr.operand2 = IRContractAdder<t1>::add(emitter, o1);                                                         \
        instr.operand3 = IRContractAdder<t2>::add(emitter, o2);                                                         \
        return emitter->add_instruction(instr);                                                                        \
    }

#define INSTR_0(name) \
    static U32 emit_##name(IREmitter* emitter, Token token) {                                                          \
        emitter->tokens.push(token);                                                                                   \
        IRInstruction instr;                                                                                           \
        instr.op = IRInstructionOperator_##name;                                                                       \
        return emitter->add_instruction(instr);                                                                        \
    }

#define INSTR_1(name, t1)                                                                                              \
    static U32 emit_##name(IREmitter* emitter, Token token, t1 o1) {                                                          \
        emitter->tokens.push(token);                                                                                   \
        IRInstruction instr;                                                                                           \
        instr.op = IRInstructionOperator_##name;                                                                       \
        instr.operand1 = IRContractAdder<t1>::add(emitter, o1);                                                         \
        return emitter->add_instruction(instr);                                                                        \
    }

#define INSTR_2(name, t1, t2)                                                                                          \
    static U32 emit_##name(IREmitter* emitter, Token token, t1 o1, t2 o2) {                                                   \
        emitter->tokens.push(token);                                                                                   \
        IRInstruction instr;                                                                                           \
        instr.op = IRInstructionOperator_##name;                                                                       \
        instr.operand1 = IRContractAdder<t1>::add(emitter, o1);                                                         \
        instr.operand2 = IRContractAdder<t2>::add(emitter, o2);                                                         \
        return emitter->add_instruction(instr);                                                                        \
    }

#define INSTR_3(name, t1, t2, t3)                                                                                          \
    static U32 emit_##name(IREmitter* emitter, Token token, t1 o1, t2 o2, t3 o3) {                                                   \
        emitter->tokens.push(token);                                                                                   \
        IRInstruction instr;                                                                                           \
        instr.op = IRInstructionOperator_##name;                                                                       \
        instr.operand1 = IRContractAdder<t1>::add(emitter, o1);                                                         \
        instr.operand2 = IRContractAdder<t2>::add(emitter, o2);                                                         \
        instr.operand3 = IRContractAdder<t3>::add(emitter, o3);                                                         \
        return emitter->add_instruction(instr);                                                                        \
    }

ENUM_IR_CONTRACTS

#undef INSTR_0
#undef INSTR_1
#undef INSTR_2
#undef INSTR_3
#undef INSTR_VAR_0
#undef INSTR_VAR_1
#undef INSTR_VAR_2

static inline bool compile_use_stmt(UseStmt* stmt, IrContext* ctx) {
    auto db_name = stmt->database.view();

    for (UZ i = 0; i < ctx->database_schemas.count; ++i) {
        if (ctx->database_schemas[i].name == db_name) {
            ctx->active_db_id = i;
            emit_UseDatabase(&ctx->ir_emitter, stmt->token, db_name);
            return true;
        }
    }

    String error_message = String::format(ctx->allocator, "cannot find database '" OK_SV_FMT "'", OK_SV_ARG(db_name));
    ctx->error_on(stmt->token, error_message);
    return false;
}

static Optional<U32> compile_expr(Expr*, IrContext*);

static inline Optional<U32> compile_binop(BinaryOpExpr* binop, IrContext* ctx) {
    auto lhs = compile_expr(binop->lhs, ctx);
    TRY(lhs);
    auto rhs = compile_expr(binop->rhs, ctx);
    TRY(rhs);

    IREmitter* e = &ctx->ir_emitter;

    switch (binop->kind) {
    case BinaryOpExpr::Kind::EQ: return emit_Eq(e, binop->token, lhs.value, rhs.value);
    case BinaryOpExpr::Kind::GT: return emit_Gt(e, binop->token, lhs.value, rhs.value);
    case BinaryOpExpr::Kind::LT: return emit_Lt(e, binop->token, lhs.value, rhs.value);
    default:                     OK_UNREACHABLE();
    }
}

static Optional<U32> compile_ident(IdentifierExpr* ident, IrContext* ctx) {
    switch (ctx->current_namespace()) {
    case IrContext::NS_TABLE: {
        U32 table_location = ctx->current_table_id();
        StringView column_name = ident->value.view();

        Optional<TableSchema*> table_schema = ctx->get_table_schema_by_id(table_location);
        OK_ASSERT(table_schema.has_value());

        if (!table_schema.value->find_column(column_name)) {
            String error_message = String::format(ctx->allocator, "cannot find column '%s'", ident->value.cstr());
            ctx->error_on(ident->token, error_message);
            return {};
        }

            return emit_FetchColumn(&ctx->ir_emitter, ident->token, table_location, column_name);
    }
    case IrContext::NS_GLOBAL: {
        StringView table_name = ident->value.view();
        Optional<TableSchema*> table_schema = ctx->get_table_schema(ctx->active_db_id, table_name);

        if (!table_schema) {
            String error_message = String::format(ctx->allocator, "cannot find table '%s'", ident->value.cstr());
            ctx->error_on(ident->token, error_message);
            return {};
        }

        return emit_FetchTable(&ctx->ir_emitter, ident->token, table_name, table_schema.value);
    }
    default: OK_UNREACHABLE();
    }
}

static Optional<U32> compile_expr(Expr* expr, IrContext* ctx) {
    auto* e = &ctx->ir_emitter;

    switch (expr->type) {
    case Expr::INTEGER_LIT: {
        auto* integer_expr = static_cast<IntegerExpr*>(expr);
        return emit_ConstInt(e, expr->token, integer_expr->value);
    }
    case Expr::STRING_LIT: {
        auto* string_expr = static_cast<StringExpr*>(expr);
        return emit_ConstString(e, expr->token, string_expr->value.view());
    }
    case Expr::TRUE_LIT:  return emit_ConstTrue(e, expr->token);
    case Expr::FALSE_LIT: return emit_ConstFalse(e, expr->token);
    case Expr::NULL_LIT:  return emit_ConstNull(e, expr->token);
    case Expr::IDENT:     {
        auto* ident = static_cast<IdentifierExpr*>(expr);
        return compile_ident(ident, ctx);
    }
    case Expr::BINARY_OP:
    case Expr::SELECT:    OK_UNREACHABLE();
    default:              OK_UNREACHABLE();
    }
}

static inline bool parse_type(StringView input, Token where, IrContext* ctx, ColumnType* out) {
    if (input == "int"_sv) {
        *out = COLUMN_INTEGER;
        return true;
    }

    if (input == "text"_sv) {
        *out = COLUMN_TEXT;
        return true;
    }

    String error_message =
            String::format(ctx->allocator, "'" OK_SV_FMT "' is not a valid column type", OK_SV_ARG(input));
    ctx->error_on(where, error_message);
    return false;
}

static inline bool compile_create_stmt(CreateStmt* stmt, IrContext* ctx) {
    if (stmt->target == CreateStmt::Target::DATABASE) {
        for (UZ i = 0; i < ctx->database_schemas.count; ++i) {
            if (ctx->database_schemas[i].name == stmt->name) {
                String error_message =
                        String::format(ctx->allocator, "database '%s' already exists", stmt->name.cstr());
                ctx->error_on(stmt->token, error_message);
                return false;
            }
        }

        DBSchema new_db_schema = DBSchema::alloc(ctx->allocator, stmt->name.view());
        ctx->database_schemas.push(new_db_schema);

        emit_CreateDatabase(&ctx->ir_emitter, stmt->token, stmt->name.view());
        return true;
    }

    OK_ASSERT(stmt->target == CreateStmt::Target::TABLE);

    if (ctx->get_table_schema(ctx->active_db_id, stmt->name.view())) {
        String error_message = String::format(ctx->allocator, "table '%s' already exists", stmt->name.cstr());
        ctx->error_on(stmt->token, error_message);
        return false;
    }

    auto* create_table_stmt = static_cast<CreateTableStmt*>(stmt);

    TableSchema* table_schema = ctx->alloc_table_schema(ctx->active_db_id, stmt->name, true);
    OK_ASSERT(table_schema->columns_types.has_value());

    for (UZ i = 0; i < create_table_stmt->column_names.count; ++i) {
        String column_type_string = create_table_stmt->column_types[i];
        ColumnType column_type;
        TRY(parse_type(column_type_string.view(), stmt->token, ctx, &column_type));

        String column_name = create_table_stmt->column_names[i];
        table_schema->columns_names.push(column_name.copy(ctx->allocator));
        table_schema->columns_types.value.push(column_type);
    }

    emit_CreateTable(&ctx->ir_emitter, stmt->token, create_table_stmt->name.view(), table_schema);

    return true;
}

static inline bool compile_drop_stmt(DropStmt* stmt, IrContext* ctx) {
    auto name = stmt->name.view();

    switch (stmt->target) {
    case DropStmt::Target::DATABASE: {
        for (UZ i = 0; i < ctx->database_schemas.count; ++i) {
            if (ctx->database_schemas[i].name == name) {
                emit_DropDatabase(&ctx->ir_emitter, stmt->token, name);
                return true;
            }
        }

        String error_message = String::format(ctx->allocator, "database '%s' does not exist", stmt->name.cstr());
        ctx->error_on(stmt->token, error_message);
        return false;
    }
    case DropStmt::Target::TABLE: {
        if (!ctx->get_table_schema(ctx->active_db_id, name)) {
            String error_message = String::format(ctx->allocator, "table '%s' does not exist", stmt->name.cstr());
            ctx->error_on(stmt->token, error_message);
            return false;
        }

        emit_DropTable(&ctx->ir_emitter, stmt->token, name);
        return true;
    }
    default: OK_UNREACHABLE();
    }
}

static inline bool compile_unoptimizable_stmt(Stmt* stmt, IrContext* ctx) {
    switch (stmt->type) {
    case Stmt::USE: {
        auto* use = static_cast<UseStmt*>(stmt);
        return compile_use_stmt(use, ctx);
    }
    case Stmt::CREATE: {
        auto* create = static_cast<CreateStmt*>(stmt);
        return compile_create_stmt(create, ctx);
    }
    case Stmt::DROP: {
        auto* drop = static_cast<DropStmt*>(stmt);
        return compile_drop_stmt(drop, ctx);
    }
    default: OK_UNREACHABLE();
    }
}

struct StmtGraphNode {
    enum Type : U8 {
        EQ,
        LT,
        GT,
        SELECT,

        UPDATE,
        INSERT,
        DELETE,

        LEAF,

        MAX,
    };

    static StmtGraphNode expr(Type type, Expr *value, Slice<U32> edges) {
        StmtGraphNode node;
        node.flags = type;
        node.up.expr = value;
        node.edges = edges;
        return node;
    }

    static StmtGraphNode stmt(Type type, Stmt *stmt, Slice<U32> edges) {
        StmtGraphNode node;
        node.flags = type;
        node.up.stmt = stmt;
        node.edges = edges;
        return node;
    }


    inline Type type() const {
        return (Type) (flags & 0xFF);
    }

    inline bool is_visited() const {
        return flags & (1 << (sizeof(flags) * 8 - 1));
    }

    inline void make_visited() {
        flags |= (1 << (sizeof(flags) * 8 - 1));
    }

    U32 ir_id = 0;
    U32 flags;
    // Check the node type to find out which one is active.
    union {
        Expr *expr;
        Stmt *stmt;
    } up;
    Slice<U32> edges;
};

static const char* stmt_graph_node_types_pretty[StmtGraphNode::Type::MAX] = {
        [StmtGraphNode::EQ] = "=",          [StmtGraphNode::LT] = "<",          [StmtGraphNode::GT] = ">",
        [StmtGraphNode::SELECT] = "SELECT", [StmtGraphNode::UPDATE] = "UPDATE", [StmtGraphNode::INSERT] = "INSERT",
        [StmtGraphNode::DELETE] = "DELETE", [StmtGraphNode::LEAF] = "<leaf>",
};

struct StmtGraph {
    explicit StmtGraph(Allocator* allocator) : allocator{allocator} {
        node_indices = Table<ok::HashPtr<Expr>, U32>::alloc(allocator);
        nodes = List<StmtGraphNode>::alloc(allocator);
    }

    inline U32 add_expr_node(Expr* expr, StmtGraphNode node) {
        nodes.push(node);

        U32 id = nodes.count - 1;
        node_indices.put(expr, id);
        return id;
    }

    inline U32 add_stmt_node(StmtGraphNode node) {
        nodes.push(node);
        return nodes.count - 1;
    }

    Allocator* allocator;
    Table<ok::HashPtr<Expr>, U32> node_indices;
    List<StmtGraphNode> nodes;
    U32 root_node_index{};
};

static U32 expr_to_node(StmtGraph* g, Expr* expr) {
    auto existing_node = g->node_indices.get(ok::HashPtr{expr});
    if (existing_node.has_value()) return existing_node.value;

    Slice<U32> edges{nullptr, 0};
    StmtGraphNode::Type node_type{};

    switch (expr->type) {
    case Expr::INTEGER_LIT:
    case Expr::STRING_LIT:
    case Expr::TRUE_LIT:
    case Expr::FALSE_LIT:
    case Expr::NULL_LIT:
    case Expr::IDENT:       {
        node_type = StmtGraphNode::LEAF;
        break;
    }
    case Expr::BINARY_OP: {
        auto* binop = static_cast<BinaryOpExpr*>(expr);

        switch (binop->kind) {
        case BinaryOpExpr::Kind::EQ: {
            node_type = StmtGraphNode::EQ;
            break;
        }
        case BinaryOpExpr::Kind::LT: {
            node_type = StmtGraphNode::LT;
            break;
        }
        case BinaryOpExpr::Kind::GT: {
            node_type = StmtGraphNode::GT;
            break;
        }
        }

        auto lhs_node = expr_to_node(g, binop->lhs);
        auto rhs_node = expr_to_node(g, binop->rhs);

        auto* edges_ptr = g->allocator->alloc<U32>(2);
        edges_ptr[0] = lhs_node;
        edges_ptr[1] = rhs_node;

        edges = Slice<U32>{edges_ptr, 2};

        break;
    }
    case Expr::SELECT: {
        node_type = StmtGraphNode::SELECT;

        auto* select = static_cast<SelectExpr*>(expr);

        UZ edges_count = select->exprs.count + 1;

        auto* edges_ptr = g->allocator->alloc<U32>(select->exprs.count + 1);

        UZ i;
        for (i = 0; i < select->exprs.count; ++i) {
            auto node = expr_to_node(g, select->exprs[i]);
            edges_ptr[i] = node;
        }

        auto table_node = expr_to_node(g, select->table);
        edges_ptr[i] = table_node;

        edges = Slice<U32>{edges_ptr, edges_count};

        break;
    }
    default: OK_UNREACHABLE();
    }

    StmtGraphNode graph_node = StmtGraphNode::expr(node_type, expr, edges);

    return g->add_expr_node(expr, graph_node);
}

static StmtGraph build_expr_stmt_graph(Allocator* allocator, ExprStmt* stmt) {
    StmtGraph graph{allocator};
    graph.root_node_index = expr_to_node(&graph, stmt->expr);
    return graph;
}

static StmtGraph build_insert_stmt_graph(Allocator *allocator, InsertStmt *stmt) {
    StmtGraph graph{allocator};

    UZ edges_count = stmt->values.count + 1;
    U32 *edges_ptr = allocator->alloc<U32>(edges_count);

    Slice<U32> edges{};
    edges.items = edges_ptr;
    edges.count = edges_count;

    edges[0] = expr_to_node(&graph, stmt->table);

    for (UZ i = 0; i < stmt->values.count; ++i) {
        Expr *value = stmt->values[i];
        U32 value_id = expr_to_node(&graph, value);
        edges[i + 1] = value_id;
    }

    StmtGraphNode root_node = StmtGraphNode::stmt(StmtGraphNode::INSERT, stmt, edges);
    graph.root_node_index = graph.add_stmt_node(root_node);
    return graph;
}

StmtGraph build_update_stmt_graph(Allocator *allocator, UpdateStmt *stmt) {
    StmtGraph graph{allocator};

    if (stmt->filter.has_value()) OK_TODO();

    UZ edges_count = stmt->values.count + 1;

    Slice<U32> edges;
    edges.items = allocator->alloc<U32>(edges_count);
    edges.count = edges_count;

    edges[0] = expr_to_node(&graph, stmt->table);

    for (UZ i = 0; i < stmt->values.count; ++i) {
        edges[i + 1] = expr_to_node(&graph, stmt->values[i]);
    }

    StmtGraphNode root_node = StmtGraphNode::stmt(StmtGraphNode::UPDATE, stmt, edges);
    graph.root_node_index = graph.add_stmt_node(root_node);
    return graph;
}

static StmtGraph build_delete_stmt_graph(Allocator *allocator, DeleteStmt *stmt) {
    StmtGraph graph{allocator};

    if (stmt->filter.has_value()) OK_TODO();

    Slice<U32> edges;
    edges.items = allocator->alloc<U32>(1);
    edges.count = 1;

    edges[0] = expr_to_node(&graph, stmt->table);

    StmtGraphNode root_node = StmtGraphNode::stmt(StmtGraphNode::DELETE, stmt, edges);
    graph.root_node_index = graph.add_stmt_node(root_node);
    return graph;
}

static StmtGraph build_statement_graph(Allocator* allocator, Stmt* stmt) {
    switch (stmt->type) {
    case Stmt::INSERT: {
        auto *insert = static_cast<InsertStmt*>(stmt);
        return build_insert_stmt_graph(allocator, insert);
    }
    case Stmt::UPDATE: {
        auto *update = static_cast<UpdateStmt*>(stmt);
        return build_update_stmt_graph(allocator, update);
    }
    case Stmt::DELETE: {
        auto *del = static_cast<DeleteStmt*>(stmt);
        return build_delete_stmt_graph(allocator, del);
    }
    case Stmt::EXPR: {
        auto* expr = static_cast<ExprStmt*>(stmt);
        return build_expr_stmt_graph(allocator, expr);
    }

    default: OK_UNREACHABLE();
    }
}

static inline bool is_stmt_graph_optimizable(Stmt* stmt) {
    switch (stmt->type) {
    case Stmt::USE:
    case Stmt::DROP:
    case Stmt::CREATE: return false;
    case Stmt::EXPR:
    case Stmt::UPDATE:
    case Stmt::DELETE:
    case Stmt::INSERT: return true;
    default:           OK_UNREACHABLE();
    }
}

static void to_string(ok::String* string, StmtGraph* g, U32 node_idx, bool type_only = false) {
    auto* node = &g->nodes[node_idx];

    if (node->type() == StmtGraphNode::LEAF) {
        auto expr_string = node->up.expr->to_string(ok::temp_allocator);
        string->format_append("\"%s\";", expr_string.cstr());
        return;
    }

    const char* node_type_str = stmt_graph_node_types_pretty[node->type()];

    string->format_append("\"%s\"", node_type_str);

    if (type_only) {
        string->push(';');
        return;
    }

    string->append(" -> "_sv);
    to_string(string, g, node->edges[0], true);

    for (UZ i = 1; i < node->edges.count; ++i) {
        string->format_append("\"%s\" -> ", node_type_str);
        to_string(string, g, node->edges[i], true);
    }
}

Optional<U32> compile_graph_node(StmtGraph* g, U32 node_id, IrContext* ctx) {
    auto* node = &g->nodes[node_id];
    if (node->is_visited()) return node->ir_id;

    node->make_visited();

    IREmitter* e = &ctx->ir_emitter;

    switch (node->type()) {
    case StmtGraphNode::LEAF: {
        auto expr_id = compile_expr(node->up.expr, ctx);
        TRY(expr_id);
        node->ir_id = expr_id.value;
        return node->ir_id;
    }
    case StmtGraphNode::EQ: {
        auto lhs_node = node->edges[0];
        auto rhs_node = node->edges[1];

        auto lhs = compile_graph_node(g, lhs_node, ctx);
        TRY(lhs);
        auto rhs = compile_graph_node(g, rhs_node, ctx);
        TRY(rhs);

        node->ir_id = emit_Eq(e, node->up.expr->token, lhs.value, rhs.value);
        return node->ir_id;
    }
    case StmtGraphNode::LT: {
        auto lhs_node = node->edges[0];
        auto rhs_node = node->edges[1];

        auto lhs = compile_graph_node(g, lhs_node, ctx);
        TRY(lhs);
        auto rhs = compile_graph_node(g, rhs_node, ctx);
        TRY(rhs);

        node->ir_id = emit_Lt(e, node->up.expr->token, lhs.value, rhs.value);
        return node->ir_id;
    }
    case StmtGraphNode::GT: {
        auto lhs_node = node->edges[0];
        auto rhs_node = node->edges[1];

        auto lhs = compile_graph_node(g, lhs_node, ctx);
        TRY(lhs);
        auto rhs = compile_graph_node(g, rhs_node, ctx);
        TRY(rhs);

        node->ir_id = emit_Gt(e, node->up.expr->token, lhs.value, rhs.value);
        return node->ir_id;
    }
    case StmtGraphNode::SELECT: {
        OK_ASSERT(node->edges.count != 0);
        U32 table_node_edge = node->edges[node->edges.count - 1];
        StmtGraphNode table_node = g->nodes[table_node_edge];

        Optional<U32> table_node_id = compile_graph_node(g, table_node_edge, ctx);
        TRY(table_node_id);

        Optional<TableSchema*> table_schema = ctx->get_table_schema_by_id(table_node_id.value);

        if (!table_schema) {
            String error_message = String::alloc(ctx->allocator, "expression is not a table");
            ctx->error_on(table_node.up.expr->token, error_message);
            return {};
        }

        UZ columns_count = node->edges.count - 1;

        ctx->push_table(table_node_id.value);
        {
            for (UZ i = 0; i < node->edges.count - 1; ++i) {
                U32 edge_id = node->edges[i];

                StmtGraphNode* expr_node = &g->nodes[edge_id];
                Optional<U32> expr_node_id = compile_graph_node(g, edge_id, ctx);
                TRY(expr_node_id);

                StringView column_name = ""_sv;
                if (expr_node->up.expr->type == Expr::IDENT) {
                    IdentifierExpr* ident = static_cast<IdentifierExpr*>(expr_node->up.expr);
                    column_name = ident->value.view();
                }

                emit_EmitColumn(&ctx->ir_emitter, expr_node->up.expr->token, expr_node_id.value, column_name);
            }
        }
        ctx->pop_namespace();

        node->ir_id = emit_EmitQuery(&ctx->ir_emitter, node->up.expr->token, columns_count);
        return node->ir_id;
    }
    case StmtGraphNode::INSERT: {
        OK_ASSERT(node->edges.count != 0);

        U32 table_node_edge = node->edges[0];
        StmtGraphNode table_node = g->nodes[table_node_edge];

        Optional<U32> table_node_id = compile_graph_node(g, table_node_edge, ctx);
        TRY(table_node_id);

        Optional<TableSchema*> table_schema = ctx->get_table_schema_by_id(table_node_id.value);

        if (!table_schema) {
            String error_message = String::alloc(ctx->allocator, "expression is not a table");
            ctx->error_on(table_node.up.expr->token, error_message);
            return {};
        }

        OK_ASSERT(node->up.stmt->type == Stmt::INSERT);

        auto *insert_stmt = static_cast<InsertStmt*>(node->up.stmt);

        ctx->push_table(table_node_id.value);
        {
            UZ values_start = 1;

            for (UZ i = 0; i < insert_stmt->values_counts.count; ++i) {
                U32 values_count = insert_stmt->values_counts[i];

                U32 first_expr_node_id = node->edges[values_start];
                StmtGraphNode *first_expr_node = &g->nodes[first_expr_node_id];

                if (values_count != insert_stmt->columns.count) {
                    String error_message = String::format(ctx->allocator,
                                                         "expected %zu values, but got %u instead",
                                                         insert_stmt->columns.count,
                                                         values_count);
                    ctx->error_on(first_expr_node->up.expr->token, error_message);
                    return {};
                }

                for (UZ j = 0; j < values_count; ++j) {
                    U32 value_edge = node->edges[values_start + j];
                    StmtGraphNode *value_edge_node = &g->nodes[value_edge];

                    Optional<U32> value_id = compile_graph_node(g, value_edge, ctx);
                    TRY(value_id);

                    StringView column_name = insert_stmt->columns[j].view();

                    emit_InsertColumn(&ctx->ir_emitter,
                                      value_edge_node->up.expr->token,
                                      table_node_id.value,
                                      value_id.value,
                                      column_name);
                }

                emit_InsertRow(&ctx->ir_emitter, first_expr_node->up.expr->token, table_node_id.value);

                values_start += values_count;
            }
        }
        ctx->pop_namespace();

        node->ir_id = emit_CommitInsert(&ctx->ir_emitter, insert_stmt->token);
        return node->ir_id;
    }
    case StmtGraphNode::UPDATE: {
        OK_ASSERT(node->edges.count != 0);

        U32 table_node_edge = node->edges[0];
        StmtGraphNode table_node = g->nodes[table_node_edge];

        Optional<U32> table_node_id = compile_graph_node(g, table_node_edge, ctx);
        TRY(table_node_id);

        Optional<TableSchema*> table_schema = ctx->get_table_schema_by_id(table_node_id.value);

        if (!table_schema) {
            String error_message = String::alloc(ctx->allocator, "expression is not a table");
            ctx->error_on(table_node.up.expr->token, error_message);
            return {};
        }

        OK_ASSERT(node->up.stmt->type == Stmt::UPDATE);

        auto *update_stmt = static_cast<UpdateStmt*>(node->up.stmt);

        ctx->push_table(table_node_id.value);
        {
            for (UZ i = 1; i < node->edges.count; ++i) {
                StringView column_name = update_stmt->columns[i - 1].view();

                if (!table_schema.value->find_column(column_name)) {
                    String error_message = String::format(ctx->allocator,
                                                          "column '" OK_SV_FMT "' not found",
                                                          OK_SV_ARG(column_name));
                    ctx->error_on(update_stmt->token, error_message);
                    return {};
                }

                U32 value_edge = node->edges[i];
                StmtGraphNode *value_edge_node = &g->nodes[value_edge];

                Optional<U32> value_edge_id = compile_graph_node(g, value_edge, ctx);
                TRY(value_edge_id);

                emit_UpdateColumn(&ctx->ir_emitter,
                                  value_edge_node->up.expr->token,
                                  table_node_id.value,
                                  value_edge_id.value,
                                  column_name);
            }
        }
        ctx->pop_namespace();

        node->ir_id = emit_CommitUpdate(&ctx->ir_emitter, update_stmt->token);
        return node->ir_id;
    }
    case StmtGraphNode::DELETE: {
        OK_ASSERT(node->edges.count == 1);

        U32 table_node_edge = node->edges[0];
        StmtGraphNode table_node = g->nodes[table_node_edge];

        Optional<U32> table_node_id = compile_graph_node(g, table_node_edge, ctx);
        TRY(table_node_id);

        Optional<TableSchema*> table_schema = ctx->get_table_schema_by_id(table_node_id.value);

        if (!table_schema) {
            String error_message = String::alloc(ctx->allocator, "expression is not a table");
            ctx->error_on(table_node.up.expr->token, error_message);
            return {};
        }

        OK_ASSERT(node->up.stmt->type == Stmt::DELETE);

        ctx->push_table(table_node_id.value);
        {
            // TODO: apply filter
        }
        ctx->pop_namespace();

        node->ir_id = emit_DeleteTable(&ctx->ir_emitter, node->up.stmt->token, table_node_id.value);
        return node->ir_id;
    }
    default: OK_TODO();
    }
}

static inline bool compile_graph(StmtGraph* graph, IrContext* ctx) {
#if 0
    auto dot_src = ok::String::alloc(ctx->allocator, "digraph { graph [dpi=200]; ");

    for (UZ i = 0; i < graph->nodes.count; ++i) {
        to_string(&dot_src, graph, i);
    }

    dot_src.push('}');

    auto dot = ok::Command::alloc(ctx->allocator, "dot");

    dot.arg("-Tpng").arg("-ostmt.png").set_stdin(dot_src.view());
    auto err = dot.exec();

    OK_ASSERT(!err.has_value());
#endif
    return (bool) compile_graph_node(graph, graph->root_node_index, ctx);
}

const char *stringify_op(StringView op) {
    char *data = ok::temp_allocator->alloc<char>(op.count + 1);
    memcpy(data, op.data, op.count);
    data[op.count] = '\0';
    return data;
}

const char *stringify_op(S64 op) {
    String s = ok::to_string(ok::temp_allocator, op);
    return s.cstr();
}

const char *stringify_op(TableSchema *op) {
    String s = String::format(ok::temp_allocator, "<table schema with %zu columns>", op->columns_names.count);
    return s.cstr();
}

String stringify_ir(Allocator* allocator, CompiledQuery* emitter) {
    String buffer = String::alloc(allocator);

    Slice<IRInstruction> instructions = emitter->instructions.slice();

    UZ ip = instructions.count;
    U32 ip_padding = 1;
    while (ip /= 10) ++ip_padding;

    for (UZ i = 0; i < instructions.count; ++i) {
        IRInstruction instr = instructions[i];

        buffer.format_append("[%-*zu] ", ip_padding, i);

#define INSTR_VAR_2(name, t1, t2) \
        case IRInstructionOperator_##name: { \
        Triple<String, t1, t2> operands = operands_of_##name(emitter, i); \
        const char *op1 = stringify_op(operands.op2); \
        const char *op2 = stringify_op(operands.op3); \
        buffer.format_append("%s := %s %s %s", operands.op1.cstr(), #name,  op1, op2); \
        break;\
        }

#define INSTR_VAR_1(name, t1) \
        case IRInstructionOperator_##name: { \
        Tuple<String, t1> operands = operands_of_##name(emitter, i); \
        const char *op1 = stringify_op(operands.op2); \
        buffer.format_append("%s := %s %s", operands.op1.cstr(), #name, op1); \
        break;\
        }

#define INSTR_VAR_0(name) \
        case IRInstructionOperator_##name: { \
        String var_name = operands_of_##name(emitter, i); \
        buffer.format_append("%s := %s", var_name.cstr(), #name); \
        break;\
        }

#define INSTR_0(name) \
        case IRInstructionOperator_##name: { \
        buffer.format_append("%s", #name); \
        break;\
        }

#define INSTR_1(name, t1) \
        case IRInstructionOperator_##name: { \
        t1 op = operands_of_##name(emitter, i); \
        const char *op_str = stringify_op(op); \
        buffer.format_append("%s %s", #name, op_str); \
        break;\
        }

#define INSTR_2(name, t1, t2) \
        case IRInstructionOperator_##name: { \
        Tuple<t1, t2> operands = operands_of_##name(emitter, i); \
        const char *op1 = stringify_op(operands.op1); \
        const char *op2 = stringify_op(operands.op2); \
        buffer.format_append("%s %s %s", #name, op1, op2); \
        break;\
        }

#define INSTR_3(name, t1, t2, t3) \
        case IRInstructionOperator_##name: { \
        Triple<t1, t2, t3> operands = operands_of_##name(emitter, i); \
        const char *op1 = stringify_op(operands.op1); \
        const char *op2 = stringify_op(operands.op2); \
        const char *op3 = stringify_op(operands.op3); \
        buffer.format_append("%s %s %s %s", #name, op1, op2, op3); \
        break;\
        }

switch (instr.op) {
    ENUM_IR_CONTRACTS
#undef INSTR_0
#undef INSTR_1
#undef INSTR_2
#undef INSTR_3
#undef INSTR_VAR_0
#undef INSTR_VAR_1
#undef INSTR_VAR_2
        default: OK_UNREACHABLE();
        }

        buffer.push('\n');
    }

    return buffer;
}

bool ir_compile_query(Query *q, IrContext *ctx, CompiledQuery *out_query) {
    for (UZ i = 0; i < q->stmts.count; ++i) {
        auto* stmt = q->stmts[i];

        if (is_stmt_graph_optimizable(stmt)) {
            StmtGraph g = build_statement_graph(ctx->allocator, stmt);
            TRY(compile_graph(&g, ctx));
        } else {
            TRY(compile_unoptimizable_stmt(stmt, ctx));
        }
    }

    out_query->instructions = ctx->ir_emitter.instructions.slice();
    out_query->tokens = ctx->ir_emitter.tokens.slice();
    out_query->strings = ctx->ir_emitter.strings.slice();
    out_query->integers = ctx->ir_emitter.integers.slice();
    out_query->schemas = ctx->ir_emitter.schemas.slice();

    return true;
}
}; // namespace xmdb::SQL
