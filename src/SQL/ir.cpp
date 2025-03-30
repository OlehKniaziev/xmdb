#include <Core/util.hpp>

#include "ir.hpp"

using namespace ok::literals;

namespace xmdb::SQL {
static inline bool compile_use_stmt(UseStmt* stmt, IrContext* ctx) {
    auto db_name = stmt->database.view();

    for (UZ i = 0; i < ctx->database_schemas.count; ++i) {
        if (ctx->database_schemas[i].name == db_name) {
            ctx->active_db_id = i;
            ctx->ir_emitter.use_database(db_name);
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
    case BinaryOpExpr::Kind::EQ: return e->eq(lhs.value, rhs.value);
    case BinaryOpExpr::Kind::GT: return e->gt(lhs.value, rhs.value);
    case BinaryOpExpr::Kind::LT: return e->lt(lhs.value, rhs.value);
    default: OK_UNREACHABLE();
    }
}

static Optional<U32> compile_ident(IdentifierExpr* ident, IrContext* ctx) {
    switch (ctx->current_namespace()) {
    case IrContext::NS_TABLE: {
        U32 table_location = ctx->current_table_id();
        StringView column_name = ident->value.view();

        Optional<TableSchema*> table_schema = ctx->get_table_schema_by_id(table_location);
        OK_ASSERT(table_schema.has_value());

        if (!table_schema.value->find_column(column_name, nullptr)) {
            String error_message = String::format(ctx->allocator, "cannot find column '%s'", ident->value.cstr());
            ctx->error_on(ident->token, error_message);
            return {};
        }

        return ctx->ir_emitter.fetch_column(table_location, column_name);
    }
    case IrContext::NS_GLOBAL: {
        StringView table_name = ident->value.view();

        if (!ctx->get_table_schema(ctx->active_db_id, table_name, nullptr)) {
            String error_message = String::format(ctx->allocator, "cannot find table '%s'", ident->value.cstr());
            ctx->error_on(ident->token, error_message);
            return {};
        }

        return ctx->ir_emitter.fetch_table(table_name);
    }
    default: OK_UNREACHABLE();
    }
}

static Optional<U32> compile_expr(Expr* expr, IrContext* ctx) {
    auto* e = &ctx->ir_emitter;

    switch (expr->type) {
    case Expr::INTEGER_LIT: {
        auto* integer_expr = static_cast<IntegerExpr*>(expr);
        return e->constant(integer_expr->value);
    }
    case Expr::STRING_LIT: {
        auto* string_expr = static_cast<StringExpr*>(expr);
        return e->constant(string_expr->value);
    }
    case Expr::TRUE_LIT: return e->constant_true();
    case Expr::FALSE_LIT: return e->constant_false();
    case Expr::NULL_LIT: return e->constant_null();
    case Expr::IDENT: {
        auto* ident = static_cast<IdentifierExpr*>(expr);
        return compile_ident(ident, ctx);
    }
    case Expr::BINARY_OP:
    case Expr::SELECT: OK_UNREACHABLE();
    default: OK_UNREACHABLE();
    }
}

static inline bool parse_type(StringView input, Token where, IrContext* ctx, ColumnType* out) {
    if (input == "int"_sv) {
        *out = ColumnType::INTEGER;
        return true;
    }

    if (input == "text"_sv) {
        *out = ColumnType::TEXT;
        return true;
    }

    String error_message = String::format(ctx->allocator, "'" OK_SV_FMT "' is not a valid column type", OK_SV_ARG(input));
    ctx->error_on(where, error_message);
    return false;
}

static inline bool compile_create_stmt(CreateStmt* stmt, IrContext* ctx) {
    if (stmt->target == CreateStmt::Target::DATABASE) {
        for (UZ i = 0; i < ctx->database_schemas.count; ++i) {
            if (ctx->database_schemas[i].name == stmt->name) {
                String error_message = String::format(ctx->allocator, "database '%s' already exists", stmt->name.cstr());
                ctx->error_on(stmt->token, error_message);
                return false;
            }
        }

        ctx->ir_emitter.create_database(stmt->name.view());
        return true;
    }

    OK_ASSERT(stmt->target == CreateStmt::Target::TABLE);

    if (ctx->get_table_schema(ctx->active_db_id, stmt->name.view(), nullptr)) {
        String error_message = String::format(ctx->allocator, "table '%s' already exists", stmt->name.cstr());
        ctx->error_on(stmt->token, error_message);
        return false;
    }

    auto* create_table_stmt = static_cast<CreateTableStmt*>(stmt);

    auto* table_schema = ctx->alloc_table_schema(ctx->active_db_id, stmt->name, nullptr);

    for (UZ i = 0; i < create_table_stmt->column_names.count; ++i) {
        String column_type_string = create_table_stmt->column_types[i];
        ColumnType column_type;
        TRY(parse_type(column_type_string.view(), stmt->token, ctx, &column_type));

        String column_name = create_table_stmt->column_names[i];
        table_schema->column_names.push(column_name.copy(ctx->allocator));
        table_schema->column_types.push(column_type);
    }

    ctx->ir_emitter.create_table(create_table_stmt->name.view(), table_schema);

    return true;
}

static inline bool compile_drop_stmt(DropStmt* stmt, IrContext* ctx) {
    auto name = stmt->name.view();

    switch (stmt->target) {
    case DropStmt::Target::DATABASE: {
        for (UZ i = 0; i < ctx->database_schemas.count; ++i) {
            if (ctx->database_schemas[i].name == name) {
                ctx->ir_emitter.drop_database(name);
                return true;
            }
        }

        String error_message = String::format(ctx->allocator, "database '%s' does not exist", stmt->name.cstr());
        ctx->error_on(stmt->token, error_message);
        return false;
    }
    case DropStmt::Target::TABLE: {
        if (!ctx->get_table_schema(ctx->active_db_id, name, nullptr)) {
            String error_message = String::format(ctx->allocator, "table '%s' does not exist", stmt->name.cstr());
            ctx->error_on(stmt->token, error_message);
            return false;
        }

        ctx->ir_emitter.drop_table(name);
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

    StmtGraphNode(Type type, Expr* value, Slice<U32> edges) : flags{type}, value{value}, edges{edges} {
    }

    inline Type type() const {
        return (Type)(flags & 0xFF);
    }

    inline bool is_visited() const {
        return flags & (1 << (sizeof(flags) * 8 - 1));
    }

    inline void make_visited() {
        flags |= (1 << (sizeof(flags) * 8 - 1));
    }

    U32 ir_id = 0;
    U32 flags;
    Expr* value;
    Slice<U32> edges;
};

static const char* stmt_graph_node_types_pretty[StmtGraphNode::Type::MAX] = {
    [StmtGraphNode::EQ] = "=",
    [StmtGraphNode::LT] = "<",
    [StmtGraphNode::GT] = ">",
    [StmtGraphNode::SELECT] = "SELECT",
    [StmtGraphNode::UPDATE] = "UPDATE",
    [StmtGraphNode::INSERT] = "INSERT",
    [StmtGraphNode::DELETE] = "DELETE",
    [StmtGraphNode::LEAF] = "<leaf>",
};

struct StmtGraph {
    explicit StmtGraph(Allocator* allocator) : allocator{allocator} {
        node_indices = Table<ok::HashPtr<Expr>, U32>::alloc(allocator);
        nodes = List<StmtGraphNode>::alloc(allocator);
    }

    inline U32 add_or_get_node(Expr* expr, StmtGraphNode node) {
        auto node_index = node_indices.get(ok::HashPtr{expr});
        if (node_index.has_value()) return node_index.value;

        return add_node(expr, node);
    }

    inline U32 add_node(Expr* expr, StmtGraphNode node) {
        nodes.push(node);

        U32 id = nodes.count - 1;
        node_indices.put(expr, id);
        return id;
    }

    Allocator* allocator;
    Table<ok::HashPtr<Expr>, U32> node_indices;
    List<StmtGraphNode> nodes;
    U32 root_node_index{};
};

static U32 expr_to_node(StmtGraph* g, Expr* expr, bool set_root = false) {
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
    case Expr::IDENT: {
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

    StmtGraphNode graph_node{
        node_type,
        expr,
        edges,
    };

    U32 node_id = g->add_node(expr, graph_node);
    if (set_root) g->root_node_index = node_id;
    return node_id;
}

static StmtGraph build_expr_stmt_graph(Allocator* allocator, ExprStmt* stmt) {
    StmtGraph graph{allocator};
    expr_to_node(&graph, stmt->expr, true);
    return graph;
}

static StmtGraph build_statement_graph(Allocator* allocator, Stmt* stmt) {
    switch (stmt->type) {
    case Stmt::INSERT: {
        OK_TODO();
    }
    case Stmt::UPDATE: {
        OK_TODO();
    }
    case Stmt::DELETE: {
        OK_TODO();
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
    case Stmt::CREATE:
        return false;
    case Stmt::EXPR:
    case Stmt::UPDATE:
    case Stmt::DELETE:
    case Stmt::INSERT:
        return true;
    default: OK_UNREACHABLE();
    }
}

static void to_string(ok::String* string, StmtGraph* g, U32 node_idx, bool type_only = false) {
    auto* node = &g->nodes[node_idx];

    if (node->type() == StmtGraphNode::LEAF) {
        auto expr_string = node->value->to_string(ok::temp_allocator);
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
        auto expr_id = compile_expr(node->value, ctx);
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

        node->ir_id = e->eq(lhs.value, rhs.value);
        return node->ir_id;
    }
    case StmtGraphNode::LT: {
        auto lhs_node = node->edges[0];
        auto rhs_node = node->edges[1];

        auto lhs = compile_graph_node(g, lhs_node, ctx);
        TRY(lhs);
        auto rhs = compile_graph_node(g, rhs_node, ctx);
        TRY(rhs);

        node->ir_id = e->lt(lhs.value, rhs.value);
        return node->ir_id;
    }
    case StmtGraphNode::GT: {
        auto lhs_node = node->edges[0];
        auto rhs_node = node->edges[1];

        auto lhs = compile_graph_node(g, lhs_node, ctx);
        TRY(lhs);
        auto rhs = compile_graph_node(g, rhs_node, ctx);
        TRY(rhs);

        node->ir_id = e->gt(lhs.value, rhs.value);
        return node->ir_id;
    }
    case StmtGraphNode::SELECT: {
        OK_ASSERT(node->edges.count != 0);
        auto table_node_edge = node->edges[node->edges.count - 1];
        StmtGraphNode table_node = g->nodes[table_node_edge];

        auto table_node_id = compile_graph_node(g, table_node_edge, ctx);
        TRY(table_node_id);

        Optional<TableSchema*> table_schema = ctx->get_table_schema_by_id(table_node_id.value);

        if (!table_schema) {
            String error_message = String::alloc(ctx->allocator, "expression is not a table");
            ctx->error_on(table_node.value->token, error_message);
            return {};
        }

        UZ columns_count = node->edges.count - 1;

        ctx->push_table(table_node_id.value);
        {
            for (UZ i = 0; i < node->edges.count - 1; ++i) {
                auto expr_node = compile_graph_node(g, node->edges[i], ctx);
                TRY(expr_node);
                ctx->ir_emitter.emit_column(expr_node.value, "dummy"_sv); // FIXME
            }
        }
        ctx->pop_namespace();

        node->ir_id = ctx->ir_emitter.emit_query(columns_count);
        return node->ir_id;
    }
    default: OK_TODO();
    }
}

static inline bool compile_graph(StmtGraph* graph, IrContext* ctx) {
    auto dot_src = ok::String::alloc(ctx->allocator, "digraph { graph [dpi=200]; ");

    for (UZ i = 0; i < graph->nodes.count; ++i) {
        to_string(&dot_src, graph, i);
    }

    dot_src.push('}');

    auto dot = ok::Command::alloc(ctx->allocator, "dot");

    dot.arg("-Tpng").arg("-ostmt.png").set_stdin(dot_src.view());
    auto err = dot.exec();

    OK_ASSERT(!err.has_value());

    return (bool)compile_graph_node(graph, graph->root_node_index, ctx);
}

static const char* ir_operator_names[IRInstruction::OP_MAX] = {
    [IRInstruction::EQ] = "Equals",
    [IRInstruction::GT] = "Greater",
    [IRInstruction::LT] = "Less",
    [IRInstruction::FETCH_COLUMN] = "FetchColumn",
    [IRInstruction::FETCH_TABLE] = "FetchTable",
    [IRInstruction::EMIT_COLUMN] = "EmitColumn",
    [IRInstruction::EMIT_QUERY] = "EmitQuery",
    [IRInstruction::USE_DATABASE] = "UseDatabase",
    [IRInstruction::CREATE] = "Create",
    [IRInstruction::DROP] = "Drop",
    [IRInstruction::CONST] = "Const",
};

static const char* ir_target_names[IRInstruction::TARGET_MAX] = {
    [IRInstruction::TARGET_TABLE] = "Table",
    [IRInstruction::TARGET_DATABASE] = "Database",
};

String stringify_ir(Allocator* allocator, IREmitter* emitter) {
    String buffer = String::alloc(allocator);

    Slice<IRInstruction> instructions = emitter->instructions.slice();

    UZ ip = instructions.count;
    U32 ip_padding = 1;
    while (ip /= 10) ++ip_padding;

    printf("instruction padding: %u\n", ip_padding);

    for (UZ i = 0; i < instructions.count; ++i) {
        IRInstruction instr = instructions[i];

        const char* operator_name = ir_operator_names[instr.op];

        buffer.format_append("[%-*zu] ", ip_padding, i);

        switch (instr.op) {
        case IRInstruction::CONST: {
            String var_name = emitter->strings[instr.operand1];
            String constant;

            switch (instr.operand2) {
            case IRInstruction::CONST_INT: {
                S64 integer = emitter->integers[instr.operand3];
                constant = ok::to_string(ok::temp_allocator, integer);
                break;
            }
            case IRInstruction::CONST_STRING: {
                String string = emitter->strings[instr.operand3];
                constant = String::format(ok::temp_allocator, "\"%s\"", string.cstr());
                break;
            }
            case IRInstruction::CONST_TRUE: constant = String::alloc(ok::temp_allocator, "TRUE"); break;
            case IRInstruction::CONST_FALSE: constant = String::alloc(ok::temp_allocator, "FALSE"); break;
            case IRInstruction::CONST_NULL: constant = String::alloc(ok::temp_allocator, "NULL"); break;
            default: OK_UNREACHABLE();
            }

            buffer.format_append("%s := %s", var_name.cstr(), constant.cstr());
            break;
        }
        case IRInstruction::FETCH_TABLE: {
            String var_name = emitter->strings[instr.operand1];
            String table_name = emitter->strings[instr.operand2];

            buffer.format_append("%s := %s \"%s\"", var_name.cstr(), operator_name, table_name.cstr());
            break;
        }
        case IRInstruction::FETCH_COLUMN: {
            String var_name = emitter->strings[instr.operand1];
            String column_name = emitter->strings[instr.operand3];

            auto table_instr = emitter->instructions[instr.operand2];
            String table_name = emitter->strings[table_instr.operand1];

            buffer.format_append("%s := %s %s \"%s\"", var_name.cstr(), operator_name, table_name.cstr(), column_name.cstr());
            break;
        }
        case IRInstruction::EMIT_COLUMN: {
            auto column_instr = emitter->instructions[instr.operand1];

            String column_name = emitter->strings[column_instr.operand1];
            String out_name = emitter->strings[instr.operand2];

            buffer.format_append("%s %s \"%s\"", operator_name, column_name.cstr(), out_name.cstr());
            break;
        }
        case IRInstruction::EMIT_QUERY: {
            String var_name = emitter->strings[instr.operand1];
            U32 column_count = instr.operand2;

            buffer.format_append("%s := %s %u", var_name.cstr(), operator_name, column_count);
            break;
        }
        case IRInstruction::LT:
        case IRInstruction::GT:
        case IRInstruction::EQ: {
            String var_name = emitter->strings[instr.operand1];

            auto lhs_instr = emitter->instructions[instr.operand2];
            auto lhs_name = emitter->strings[lhs_instr.operand1];

            auto rhs_instr = emitter->instructions[instr.operand3];
            auto rhs_name = emitter->strings[rhs_instr.operand1];

            buffer.format_append("%s := %s %s %s", var_name.cstr(), operator_name, lhs_name.cstr(), rhs_name.cstr());
            break;
        }
        case IRInstruction::CREATE: {
            U32 target = instr.operand1;
            const char* target_name = ir_target_names[target];
            String name = emitter->strings[instr.operand2];

            if (target == IRInstruction::TARGET_TABLE) {
                auto table_schema = emitter->schemas[instr.operand3];
                buffer.format_append("%s%s %s <schema at %p>", operator_name, target_name, name.cstr(), (void*)table_schema);
            } else if (target == IRInstruction::TARGET_DATABASE) {
                buffer.format_append("%s%s %s", operator_name, target_name, name.cstr());
            } else {
                OK_PANIC_FMT("invalid target '%s' for 'CREATE' statement", target_name);
            }

            break;
        }
        case IRInstruction::DROP: {
            const char* target_name = ir_target_names[instr.operand1];
            String name = emitter->strings[instr.operand2];

            buffer.format_append("%s%s %s", operator_name, target_name, name.cstr());
            break;
        }
        default: OK_PANIC_FMT("don't know how to print the '%s' operator", operator_name);
        }

        buffer.push('\n');
    }

    return buffer;
}

bool ir_compile_query(Query* q, IrContext* ctx) {
    for (UZ i = 0; i < q->stmts.count; ++i) {
        auto* stmt = q->stmts[i];

        if (is_stmt_graph_optimizable(stmt)) {
            StmtGraph g = build_statement_graph(ctx->allocator, stmt);
            TRY(compile_graph(&g, ctx));
        } else {
            TRY(compile_unoptimizable_stmt(stmt, ctx));
        }
    }

    auto ir_string = stringify_ir(ctx->allocator, &ctx->ir_emitter);
    ok::println(ir_string);

    return true;
}
};
