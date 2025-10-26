import re
import argparse
from jinja2 import Template
from pprint import pprint

# Match namespace <name> {
NAMESPACE_REGEX = re.compile(
    r"namespace\s+(\w+)\s*(?:/\*.*?\*/|\s|//[^\n]*\n)*\{", re.DOTALL
)
# Match WCLASS() class <name> {
CLASS_REGEX = re.compile(
    r"WCLASS\(\)\s*class\s+(\w+)\s*(?:\s*:\s*[\w\s,:<>]+)?\s*\{", re.DOTALL
)
# Match WENUM() enum <name> {
ENUM_REGEX = re.compile(
    r"WENUM\(\)\s*enum\s+(\w+)\s*(?:/\*.*?\*/|\s|//[^\n]*\n)*\{", re.DOTALL
)
# Match WPROPERTY(...)
PROPERTY_REGEX = re.compile(r"WPROPERTY\((.*)\)", re.DOTALL)


def match_brace_block(text, start_index):
    """Returns the block from { to matching }, and the end index."""
    brace_depth = 0
    for i in range(start_index, len(text)):
        if text[i] == "{":
            brace_depth += 1
        elif text[i] == "}":
            brace_depth -= 1
            if brace_depth == 0:
                return text[start_index : i + 1], i + 1
    return "", len(text)


def prettify_var_name(name: str) -> str:
    # Remove leading 'e' or 'b' if immediately followed by an uppercase letter
    if len(name) >= 2 and name[0] in ("e", "b") and name[1].isupper():
        name = name[1:]

    # Remove underscores and split camelCase/PascalCase/SCREAMING_SNAKE_CASE
    parts = re.findall(r"[A-Z]?[a-z]+|[A-Z]+(?![a-z])", name.replace("_", " "))

    # Filter empty parts and title-case each word
    pretty = " ".join(word.capitalize() for word in parts if word.strip())

    return pretty


def find_properties(class_body: str) -> []:
    lines = class_body.splitlines()
    props = []

    for i, line in enumerate(lines):
        pr_match = PROPERTY_REGEX.search(line)
        if not pr_match or i + 1 >= len(lines):
            continue

        next_line = lines[i + 1].strip()

        # Remove inline comments
        next_line = next_line.split("//")[0].strip()

        # Remove trailing ';' or '{}' or '= ...'
        clean_line = re.split(r"[;{=]", next_line, maxsplit=1)[0].strip()

        # Split into tokens for name detection (last token is the variable name)
        tokens = clean_line.split()
        if not tokens:
            continue

        name = tokens[-1]
        typ = " ".join(tokens[:-1])

        # Isolate [0] from the name and puts it back to the type
        match = re.match(r"(\w+)(\[\s*\d+\s*\])?", name)
        if match.group(2) is not None:
            name = match.group(1)
            typ += match.group(2)

        prop = {
            "type": typ,
            "name": name,
            "edit": "Edit::eInput",
            "args": ["0.f", "0.f", ".01", '"%.3f"'],
            "prettified": prettify_var_name(name),
        }

        if typ == "int":
            prop["args"] = ["0", "0", "1"]

        # Override edit/args if WPROPERTY() specifies
        args = (
            [a.strip() for a in pr_match.group(1).split(",")]
            if pr_match.group(1)
            else []
        )
        if args and args[0]:
            prop["edit"] = args[0]
            for j in range(len(args[1:])):
                prop["args"][j] = args[1:][j]

        props.append(prop)

    return props


def find_refresh(class_body: str) -> {}:
    lines = class_body.splitlines()

    for i, line in enumerate(lines):
        if "REFRESH()" in line:
            if i + 1 < len(lines):
                next_line = lines[i + 1].strip()
                tokens = next_line.split()
                if len(tokens) >= 2:
                    name = tokens[1].rstrip("();")
                    return {"name": name, "valid": True}

    return {"valid": False}


def find_enums(enum_body: str) -> []:
    enums = []
    lines = enum_body.splitlines()

    for line in lines:
        line = line.split("//")[0]
        line = line.translate(str.maketrans("", "", "{} ,"))

        if len(line) == 0:
            continue

        enums.append({"prettified": prettify_var_name(line), "name": line})

    return enums


def prepend_member_type_variables(frosted_metas: []):
    type_dictionary = []

    # populate dictionary
    for frost in frosted_metas:
        type_dictionary.append({"type": frost["name"], "namespace": frost["namespace"]})

    # find all properties, and compare them to dictionary
    for frost in frosted_metas:
        if frost["type"] == "class":
            true_namespace = frost["namespace"] + frost["name"] + "::"

            # go through dictionary to see if we reference a type from there
            for prop in frost["properties"]:

                type_tokens = re.split(r"(\W+)", prop["type"])  # keep separators

                for i, token in enumerate(type_tokens):
                    if not re.match(r"\w+", token):
                        continue

                    for custom_type in type_dictionary:
                        if (
                            token == custom_type["type"]
                            and true_namespace == custom_type["namespace"]
                        ):
                            # then add that namespace to our type
                            type_tokens[i] = true_namespace + token

                prop["type"] = "".join(type_tokens)


def parse_cpp(text: str, namespace: str, frosted_metas: []):
    cursor = 0

    while cursor < len(text):
        # Find all matches from current position
        ns_match = NAMESPACE_REGEX.search(text, cursor)
        cl_match = CLASS_REGEX.search(text, cursor)
        en_match = ENUM_REGEX.search(text, cursor)

        candidates = []
        if ns_match:
            candidates.append((ns_match.start(), ns_match, "namespace"))
        if cl_match:
            candidates.append((cl_match.start(), cl_match, "class"))
        if en_match:
            candidates.append((en_match.start(), en_match, "enum"))

        if not candidates:
            break

        # Sort to get the first match in the file
        candidates.sort(key=lambda x: x[0])
        _, match, match_type = candidates[0]

        name = match.group(1)
        brace_start = text.find("{", match.end() - 1)
        if brace_start == -1:
            cursor = match.end()
            continue

        block, end_index = match_brace_block(text, brace_start)

        if match_type == "namespace":
            # recurse into namespace body, update namespace prefix
            parse_cpp(block[1:-1], namespace + name + "::", frosted_metas)

        elif match_type == "class":
            frost = {
                "type": match_type,
                "name": name,
                "prettified": prettify_var_name(name),
                "start": match.start(),
                "end": end_index,
                "namespace": namespace,
                "properties": find_properties(block[1:-1]),
                "refresh": find_refresh(block[1:-1]),
            }
            frosted_metas.append(frost)

            parse_cpp(block[1:-1], namespace + name + "::", frosted_metas)

        elif match_type == "enum":
            enums = find_enums(block)
            frost = {
                "type": match_type,
                "name": name,
                "enums": enums,
                "start": match.start(),
                "end": end_index,
                "namespace": namespace,
            }
            frosted_metas.append(frost)

        # advance cursor past this block
        cursor = end_index


def parse(filename: str) -> []:
    frosted_metas = []

    with open(filename) as f:
        content = f.read()
        parse_cpp(content, "", frosted_metas)

    return frosted_metas


def generate_meta(frosted_metas: [], filename: str) -> str:

    template_src = r"""#ifndef WFROST_GENERATED_{{- filename }}
#define WFROST_GENERATED_{{- filename }}

#include <frost.hpp>
#include <wsp_devkit.hpp>

{% for meta in metas %}
{%- if meta.type == 'class' %} 
#define WCLASS_BODY${{- meta.name -}}() friend struct frost::Meta<{{- meta.namespace -}}{{- meta.name }}>; 
{%- endif -%}
{%- endfor %}

#undef WGENERATED_META_DATA
#define WGENERATED_META_DATA() \
using namespace wsp;
{%- for meta in metas -%}
template <> struct frost::Meta<{{- meta.namespace -}}{{- meta.name -}}> { static constexpr char const * name = "{{- meta.prettified -}}"; static constexpr frost::WhispUsage usage = 
{%- if meta.type == 'class' -%}
    frost::WhispUsage::eClass; using Type = {{- meta.namespace -}}{{- meta.name -}}; 
{%- elif meta.type == 'enum' -%}
    frost::WhispUsage::eEnum;
{%- endif -%}
{%- if meta.type == 'class' -%}
    static constexpr auto fields = std::make_tuple(
{%- for prop in meta.properties -%}
    frost::Field<Type, {{- prop.type -}}>{"{{- prop.prettified -}}", &Type::{{- prop.name -}}, {{- prop.edit -}}, {%- for arg in prop.args -%}{{- arg -}}{%- if not loop.last -%}, {%- endif -%}{%- endfor -%} }{%- if not loop.last -%}, {%- endif -%}
{%- endfor -%}
    );
{%- if meta.refresh.valid -%}
    static constexpr bool hasRefresh = true; static constexpr auto refreshFunc = &Type::{{- meta.refresh.name -}};
{%- else -%}
    static constexpr bool hasRefresh = false;
{%- endif -%}
{%- endif -%}
    };
{%- if meta.type == 'enum' -%}
template <> inline const wsp::dictionary<std::string, {{- meta.namespace -}}{{- meta.name -}}>& frost::EnumDictionary() { static const wsp::dictionary<std::string, {{- meta.namespace -}}{{- meta.name -}}> dict = { {
{%- for enum in meta.enums -%}
        { "{{- enum.prettified -}}", {{- meta.namespace -}}{{- meta.name -}}::{{- enum.name -}} }{%- if not loop.last -%}, {%- endif -%}
{%- endfor -%}
    } }; return dict; }
{%- endif -%}
{%- endfor %}

#endif"""

    return Template(template_src).render(
        metas=frosted_metas, filename=filename.split(".")[0].split("/")[-1]
    )


def main():
    parser = argparse.ArgumentParser(
        description="Parse C++ classes with WCLASS and WPROPERTY."
    )
    parser.add_argument("filename", help="C++ source file to process")
    args = parser.parse_args()

    frosted_metas = parse(args.filename)
    prepend_member_type_variables(frosted_metas)

    print(generate_meta(frosted_metas, args.filename))


if __name__ == "__main__":
    main()
