SQL Reference
=============

This page describes the XMDB SQL grammar and general usage of the language.


.. contents:: Contents
   :local:
   :depth: 1

----

Grammar
-------

Terminals in the grammar are written in ``UPPERCASE`` or quoted,
for the sake of keeping the grammar description simple.
In reality, the lexer is case-insensitive with regards to terminals.

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

----

Built-in functions
------------------

``RGB(width, height, hex_data)``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: sql

   RGB(width INTEGER, height INTEGER, hex_data TEXT) -> IMAGE_CHUNK

Constructs an image chunk value from raw RGB pixel data supplied as a
hexadecimal string.

**Parameters**

.. list-table::
   :header-rows: 1

   * - Parameter
     - Type
     - Description
   * - ``width``
     - ``INTEGER``
     - Width of the image in pixels.
   * - ``height``
     - ``INTEGER``
     - Height of the image in pixels.
   * - ``hex_data``
     - ``TEXT``
     - Pixel data encoded as a hexadecimal string. Each pixel occupies 3
       consecutive bytes in ``R``, ``G``, ``B`` order, so the string must
       contain exactly ``width x height x 6`` hex characters.

**Return type:** ``IMAGE_CHUNK``

**Errors**

The function fails at runtime if ``hex_data`` is not a valid hexadecimal
string, or if the ``hex_data`` is of invalid size.

**Example**

.. code-block:: sql

   -- Insert a 1x1 image with a single red pixel
   INSERT INTO images (data) VALUES (RGB(1, 1, 'ff0000'));
