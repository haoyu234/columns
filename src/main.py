#!/usr/bin/env python3

"""
clcli.py
"""

import sys

sys.dont_write_bytecode = True

import ctypes
import getopt
import dataclasses
from io import StringIO
import os
import re
from clang.cindex import conf, register_function
from clang.cindex import (
    Config,
    Index,
    Cursor,
    CursorKind,
    TypeKind,
    TranslationUnit,
    SourceLocation,
)

import plugin_stub


@dataclasses.dataclass
class Context:
    tu: TranslationUnit
    header_code: StringIO = dataclasses.field(default_factory=StringIO)
    source_code: StringIO = dataclasses.field(default_factory=StringIO)
    struct_type_spelling: str = dataclasses.field(default="")
    prev_cursor: Cursor = dataclasses.field(default=None)


def get_compile_args(include_dirs: list[str], standard: str) -> list[str]:
    compile_args = ["-fparse-all-comments"]

    if standard:
        compile_args.append(f"-std={standard}")

    for include_dir in include_dirs:
        compile_args.append(f"-I{include_dir}")

    return compile_args


def create_unit(path: str, include_dirs: list[str], standard: str) -> TranslationUnit:
    idx = Index.create()
    return idx.parse(
        path,
        args=get_compile_args(include_dirs, standard),
        options=TranslationUnit.PARSE_INCOMPLETE
        | TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
        | TranslationUnit.PARSE_INCLUDE_BRIEF_COMMENTS_IN_CODE_COMPLETION,
    )


def in_system_header(location: SourceLocation) -> bool:
    # pylint: disable=no-member
    return conf.lib.clang_Location_isInSystemHeader(location) > 0


def process_array(cursor: Cursor, ctx: Context):
    element_type = cursor.type.get_array_element_type()
    element_type = element_type.get_canonical().get_declaration()

    array_or_flexable_array = {True: "FLEXIBLE_ARRAY", False: "FIXED_ARRAY"}

    is_flexable_array = False
    if ctx.prev_cursor:
        is_flexable_array = plugin_stub.check_is_flexable_array(ctx.prev_cursor, cursor)
    prefix_str = array_or_flexable_array.get(is_flexable_array)

    element_name = plugin_stub.generate_object_name(element_type)

    if element_type.kind != CursorKind.NO_DECL_FOUND:
        ctx.source_code.write(
            f"    FIELD_OBJECT_{prefix_str}({ctx.struct_type_spelling}, {cursor.spelling}, {element_name}),\n"
        )
    else:
        ctx.source_code.write(
            f"    FIELD_{prefix_str}({ctx.struct_type_spelling}, {cursor.spelling}),\n"
        )


def process_field(cursor: Cursor, ctx: Context):
    canonical_type = cursor.type.get_canonical()
    element_type_declaration = canonical_type.get_declaration()

    if canonical_type.kind in (
        TypeKind.BOOL,
        TypeKind.CHAR_U,
        TypeKind.CHAR_S,
        TypeKind.UCHAR,
        TypeKind.CHAR16,
        TypeKind.CHAR32,
        TypeKind.USHORT,
        TypeKind.UINT,
        TypeKind.ULONG,
        TypeKind.ULONGLONG,
        TypeKind.UINT128,
        TypeKind.CHAR_S,
        TypeKind.SCHAR,
        TypeKind.SHORT,
        TypeKind.INT,
        TypeKind.LONG,
        TypeKind.LONGLONG,
        TypeKind.INT128,
        TypeKind.FLOAT,
        TypeKind.DOUBLE,
        TypeKind.LONGDOUBLE,
        TypeKind.ENUM,
    ):
        ctx.prev_cursor = cursor
        ctx.source_code.write(
            f"    FIELD_NUMBER({ctx.struct_type_spelling}, {cursor.spelling}),\n"
        )
        return

    if canonical_type.kind in (
        TypeKind.CONSTANTARRAY,
        TypeKind.VARIABLEARRAY,
    ):
        process_array(cursor, ctx)
        return

    if element_type_declaration.kind in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL):
        struct_or_union = {
            CursorKind.UNION_DECL: "FIELD_UNION",
            CursorKind.STRUCT_DECL: "FIELD_OBJECT",
        }

        prefix_str = struct_or_union.get(element_type_declaration.kind)

        element_name = element_type_declaration.spelling
        ctx.source_code.write(
            f"    {prefix_str}({ctx.struct_type_spelling}, {cursor.spelling}, {element_name}Columns),\n"
        )
        return


def search_union_or_struct(cursor: Cursor, ctx: Context):
    if not cursor.spelling:
        return

    ctx.prev_cursor = None
    ctx.struct_type_spelling = cursor.type.spelling

    location = cursor.location
    name = os.path.basename(location.file.name)
    line = location.line
    column = location.column

    plugin_stub.begin_object(cursor)
    object_name = plugin_stub.generate_object_name(cursor)
    columns_name = plugin_stub.generate_columns_name(cursor)

    ctx.source_code.write(f"// {name}:{line}:{column}\n")

    ctx.source_code.write(f"static const clColumn {columns_name}[] = {{\n")

    for child in cursor.get_children():
        if child.kind != CursorKind.FIELD_DECL:
            continue

        process_field(child, ctx)

    ctx.source_code.write("};\n")

    if cursor.kind != CursorKind.STRUCT_DECL:
        return

    ctx.source_code.write(f"const clColumn {object_name}[] = {{\n")
    ctx.source_code.write(
        f"    DEFINE_OBJECT({ctx.struct_type_spelling}, {columns_name}),\n"
    )

    ctx.source_code.write("};\n\n")
    ctx.header_code.write(f"extern const struct clColumn {object_name}[];\n")

    plugin_stub.end_object(cursor, object_name)


def search_namespace_or_union_or_struct(cursor: Cursor, ctx: Context):
    actions = {
        CursorKind.NAMESPACE: search_namespace_or_union_or_struct,
        CursorKind.STRUCT_DECL: search_union_or_struct,
        CursorKind.UNION_DECL: search_union_or_struct,
    }

    for child in cursor.get_children():
        if child.kind not in (
            CursorKind.STRUCT_DECL,
            CursorKind.NAMESPACE,
            CursorKind.CLASS_DECL,
            CursorKind.UNION_DECL,
        ):
            continue

        if in_system_header(child.location):
            continue

        if not child.is_definition():
            continue

        action = actions.get(child.kind, None)
        if action:
            action(child, ctx)
            continue


SOURCE_CODE_TPL = """
// generated by the columns. DO NOT EDIT!

$includes

#define USE_COLUMN_MACROS
#include <columns.h>

#ifdef __cplusplus
extern "C" {
#endif

$code

#ifdef __cplusplus
}
#endif
"""

HEADER_CODE_TPL = """
#pragma once

// generated by the columns. DO NOT EDIT!
#include <columns.h>
$includes

#ifdef __cplusplus
extern "C" {
#endif
struct clColumn;

$code

#ifdef __cplusplus
}
#endif
"""


def render_tpl(tpl: str, data: dict[str, str]) -> str:
    result = tpl

    for k, v in data.items():
        result = re.sub(f"\\${k}\\r?\\n?", v, result)

    return result.strip() + "\n"


def render_include(path: str) -> str:
    return f'#include "{path}"\n'


def guess_suffix(standard: str) -> str:
    if "+" in standard:
        return "cpp"
    return "c"


def main():
    inputs = []
    includes = []
    work_dir = os.getcwd()
    standard = "c11"
    plugin = ""

    opts, args = getopt.getopt(sys.argv[1:], "C:I:p:", ["std="])
    for opt in opts:
        if opt[0] == "-C":
            work_dir = opt[1]
        elif opt[0] == "-I":
            includes.append(opt[1])
        elif opt[0] == "--std=":
            standard = opt[1]
        elif opt[0] == "-p":
            plugin = opt[1]

    inputs.extend(args)

    os.chdir(work_dir)
    work_dir = os.getcwd()

    outputs = dict()

    if len(plugin) > 0:
        plugin_stub.load_plugin(plugin)

    plugin_stub.init()

    for path in inputs:
        tu = create_unit(path, includes, standard)

        path = tu.spelling
        header_path = os.path.relpath(path, work_dir)
        ctx = Context(tu=tu)

        search_namespace_or_union_or_struct(tu.cursor, ctx)

        stem, _ = os.path.splitext(header_path)

        includes_str = ""
        if len(includes) > 0:
            includes_str = "".join(map(render_include, includes))

        outputs[stem] = (
            render_tpl(
                HEADER_CODE_TPL,
                {
                    "includes": includes_str,
                    "code": ctx.header_code.getvalue().strip(),
                },
            ),
            render_tpl(
                SOURCE_CODE_TPL,
                {
                    "includes": render_include(header_path),
                    "code": ctx.source_code.getvalue().strip(),
                },
            ),
        )

    suffix = guess_suffix(standard)

    for stem, (header, source_code) in outputs.items():
        with open(f"{stem}_def.h", mode="w", encoding="UTF-8") as out:
            out.write(header)

        with open(f"{stem}_def.{suffix}", mode="w", encoding="UTF-8") as out:
            out.write(source_code)

    plugin_stub.complete()


if __name__ == "__main__":
    functions = [
        ("clang_Location_isInSystemHeader", [SourceLocation], ctypes.c_int),
    ]

    for item in functions:
        register_function(conf.lib, item, not Config.compatibility_check)

    main()