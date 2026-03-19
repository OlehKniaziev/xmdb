SQL Syntax Reference
====================

This page describes the XMDB SQL grammar in BNF notation.
Terminals in this grammar are written in ``UPPERCASE`` or quoted,
for the sake of keeping the grammar description simple.
In reality, the lexer is case-insensitive with regards to terminals.


.. contents:: Contents
   :local:
   :depth: 1

----

Grammar
-------

.. code-block:: bnf

   query          ::= statement ( ";" statement )* ";"?

   statement      ::= (insert_stmt
                    | update_stmt
                    | delete_stmt
                    | create_stmt
                    | drop_stmt
                    | use_stmt
                    | alter_stmt) ";"

   select_expr    ::= "SELECT" select_list "FROM" identifier
                      ( "WHERE" expr )?

   select_list    ::= select_expr ( "," select_expr )*

   select_expr    ::= "*"
                    | expr

   insert_stmt    ::= "INSERT" "INTO" identifier
                      "(" identifier ( "," identifier )* ")"
                      "VALUES" value_row ( "," value_row )*

   value_row      ::= "(" expr ( "," expr )* ")"

   update_stmt    ::= "UPDATE" identifier
                      "SET" assignment ( "," assignment )*
                      ( "WHERE" expr )?

   assignment     ::= identifier "=" expr

   delete_stmt    ::= "DELETE" "FROM" identifier
                      ( "WHERE" expr )?

   create_stmt    ::= create_table_stmt
                    | create_database_stmt
                    | create_user_stmt

   create_table_stmt    ::= "CREATE" "TABLE" identifier
                            "(" column_def ( "," column_def )* ")"

   column_def           ::= identifier column_type

   column_type          ::= "INTEGER" | "FLOAT" | "DOUBLE"
                          | "TEXT" | "BOOLEAN" | "PNG"

   create_database_stmt ::= "CREATE" "DATABASE" identifier

   create_user_stmt     ::= "CREATE" "USER" identifier

   drop_stmt      ::= "DROP" ( "TABLE" | "DATABASE" ) identifier

   use_stmt       ::= "USE" identifier

   alter_stmt     ::= "ALTER" "USER" identifier
                      "SET" assignment ( "," assignment )*

   expr           ::= expr ( "=" | ">" | "<" ) expr
                    | call_expr
                    | primary

   call_expr      ::= identifier "(" ( expr ( "," expr )* )? ")"

   primary        ::= identifier
                    | integer_literal
                    | string_literal
                    | "TRUE"
                    | "FALSE"
                    | "NULL"
                    | "(" expr ")"

   identifier         ::= IDENT
   integer_literal    ::= INTEGER
   string_literal     ::= "'" <characters> "'"
                        | "\"" <characters> "\""
