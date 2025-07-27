import re
import argparse
from jinja2 import Template
from pprint import pprint

# Match namespace <name> {
NAMESPACE_REGEX = re.compile(
    r"namespace\s+(\w+)\s*(?:/\*.*?\*/|\s|//[^\n]*\n)*\{", re.DOTALL
)
# Match FROST() class <name> {
CLASS_REGEX = re.compile(
    r"FROST\(\)\s*class\s+(\w+)\s*(?:/\*.*?\*/|\s|//[^\n]*\n)*\{", re.DOTALL
)
# Match PROPERTY(...)
PROPERTY_REGEX = re.compile(r"PROPERTY\((.*)\)", re.DOTALL)


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
        if pr_match:
            if i + 1 < len(lines):
                next_line = lines[i + 1].strip()
                tokens = next_line.split()
                if len(tokens) >= 2:
                    typ, name = tokens[0], tokens[1].rstrip(";")
                    prop = {
                        "type": typ,
                        "name": name,
                        "edit": "eInput",
                        "args": ["0.f", "0.f", ".01", '"%.3f"'],
                        "prettified": prettify_var_name(name),
                    }

                    if typ == "int":
                        prop["args"]: ["0", "0", "1"]

                    args = pr_match.group(1).split(",")
                    if len(args[0]) > 0:
                        prop["edit"] = args[0]

                        for i in range(len(args[1:])):
                            prop["args"][i] = args[1:][i]

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


def parse_cpp(text: str, namespace: str, frosted_classes: []):
    cursor = 0

    while cursor < len(text):
        # Find all matches from current position
        ns_match = NAMESPACE_REGEX.search(text, cursor)
        cl_match = CLASS_REGEX.search(text, cursor)

        candidates = []
        if ns_match:
            candidates.append((ns_match.start(), ns_match, "namespace"))
        if cl_match:
            candidates.append((cl_match.start(), cl_match, "class"))

        if not candidates:
            break

        # Sort to get the first match in the file
        candidates.sort(key=lambda x: x[0])
        _, match, match_type = candidates[0]

        name = match.group(1)
        brace_start = text.find("{", match.end() - 1)
        block, end_index = match_brace_block(text, brace_start)

        if match_type == "namespace":
            parse_cpp(block[1:-1], namespace + name + "::", frosted_classes)
        else:
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
            frosted_classes.append(frost)

        cursor = end_index


def parse(filename: str) -> []:
    frosted_classes = []

    with open(filename) as f:
        content = f.read()
        parse_cpp(content, "", frosted_classes)

    return frosted_classes


def generate_meta(frosted_classes: []) -> str:
    template_src = """#include <tuple>
    #include <frost.hpp>

{% for class in classes %}
#ifndef FROST_META_DATA_{{ class.name }}
#define FROST_META_DATA_{{ class.name }}

#define FROST_BODY${{ class.name }} friend struct frost::Meta<{{ class.name }}>;

#define FROST_DATA${{ class.name }} \\
template <> struct frost::Meta<{{ class.namespace }}{{ class.name }}> \\
{ \\
    static constexpr char const * name = "{{ class.prettified }}"; \\
    using Type = {{ class.namespace }}{{ class.name }}; \\
    static constexpr auto fields = std::make_tuple( \\
    {% for prop in class.properties %} \\
    frost::Field<Type, {{ prop.type }}>{"{{ prop.prettified }}", &Type::{{ prop.name }}, {{ prop.edit }}, {% for arg in prop.args %}{{ arg }}{% if not loop.last %}, {% endif %}{% endfor %} }{% if not loop.last %}, {% endif %} \\
    {% endfor %}); {% if class.refresh.valid %} \\
    static constexpr bool hasRefresh = true; \\
    static constexpr auto refreshFunc = &Type::{{ class.refresh.name }}; \\
    {% else %} static bool valid = false; \\
    {% endif %} \\
}; 
#endif {% endfor %}"""

    return Template(template_src).render(classes=frosted_classes)


def main():
    parser = argparse.ArgumentParser(
        description="Parse C++ classes with FROST and PROPERTY."
    )
    parser.add_argument("filename", help="C++ source file to process")
    args = parser.parse_args()

    print(generate_meta(parse(args.filename)))


if __name__ == "__main__":
    main()
