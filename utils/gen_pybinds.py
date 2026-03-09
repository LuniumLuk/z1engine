import os
import re

# Use absolute path based on the script location
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ENGINE_ROOT = os.path.join(SCRIPT_DIR, "..", "engine", "runtime", "source")
STUBS_DIR = os.path.join(SCRIPT_DIR, "..", "engine", "stubs")
OUTPUT_FILE = os.path.join(ENGINE_ROOT, "python", "py_engine.gen.cpp") # GENERATED FILE
STUB_FILE = os.path.join(STUBS_DIR, "z1.pyi")
MANUAL_BINDINGS_FILE = os.path.join(ENGINE_ROOT, "python", "py_engine.cpp")

# Regex patterns
REFLECTED_STRUCT_RE = re.compile(r'REFLECTED_STRUCT\s*\(\s*(\w+)\s*\)')
REFLECTED_FIELD_RE = re.compile(r'REFLECTED_FIELD\s*\(\s*(\w+)\s*,\s*(\w+)\s*,')
REFLECT_ENUM_RE = re.compile(r'REFLECT_ENUM\s*\(\s*(\w+)\s*,')

# Manual bindings parsing patterns
PY_CLASS_RE = re.compile(r'(?:auto\s+(?P<var>\w+)\s*=\s*)?py::class_<.*?>(?:\s*\(.*?\))?\s*\(\s*\w+\s*,\s*"(?P<name>\w+)"\)')
PY_INIT_RE = re.compile(r'\.def\s*\(\s*py::init<(?P<args>.*?)>\s*\(\)\s*\)')
PY_DEF_RE = re.compile(r'(?:(?P<var>\w+)\.|^[\t ]*\.)def\s*\(\s*"(?P<name>\w+)"')
PY_PROP_RE = re.compile(r'(?:(?P<var>\w+)\.|^[\t ]*\.)def_(?:readwrite|property(?:_readonly)?)\s*\(\s*"(?P<name>\w+)"')


def extract_struct_body(content, struct_name):
	# Find start of struct
	match = re.search(r'REFLECTED_STRUCT\s*\(\s*' + struct_name + r'\s*\)', content)
	if not match:
		return ""

	start_idx = match.end()

	# Find the opening brace
	brace_idx = content.find('{', start_idx)
	if brace_idx == -1:
		return ""

	# Walk to find matching closing brace
	count = 1
	i = brace_idx + 1
	while i < len(content) and count > 0:
		if content[i] == '{':
			count += 1
		elif content[i] == '}':
			count -= 1
		i += 1

	if count == 0:
		return content[brace_idx+1 : i-1]
	return ""

def extract_enum_items(content, enum_name):
	# Find all REFLECT_ENUM(enum_name, Value)
	# Matches: REFLECT_ENUM(LightType, Directional)
	# Using re.escape for enum_name just in case, though it's alphanumeric usually
	pattern = r'REFLECT_ENUM\s*\(\s*' + re.escape(enum_name) + r'\s*,\s*(\w+)\s*\)'
	matches = re.findall(pattern, content)
	# Return list of (name, value_expr). Value expr is unused/implicit.
	return [(m, "") for m in matches]

def get_header_files(root):
	files = []
	for dirpath, dirnames, filenames in os.walk(root):
		for filename in filenames:
			if filename.endswith(".h"):
				files.append(os.path.join(dirpath, filename))
	return files

def parse_files(files):
	structs = set()
	enums = set()
	fields = {}  # struct_name -> list of field_names
	struct_headers = {} # struct_name -> header_path
	enum_headers = {} # enum_name -> header_path

	for filepath in files:
		with open(filepath, 'r', encoding='utf-8') as f:
			content = f.read()

		# Find structs
		found_structs = REFLECTED_STRUCT_RE.findall(content)
		for s in found_structs:
			structs.add(s)
			struct_headers[s] = filepath

		# Find enums
		found_enums = REFLECT_ENUM_RE.findall(content)
		for e in found_enums:
			enums.add(e)
			enum_headers[e] = filepath

		# Find fields
		found_fields = REFLECTED_FIELD_RE.findall(content)
		for s, f in found_fields:
			if s not in fields:
				fields[s] = []
			fields[s].append(f)

	# Filter out false positives from macro definitions
	if "type" in structs:
		structs.remove("type")
	if "type" in enums:
		enums.remove("type")

	return structs, enums, fields, struct_headers, enum_headers

def strip_struct_name(name):
	if name.endswith("Component"):
		return name[:-9]
	return name

def strip_field_name(name):
	if name.startswith("m_"):
		return name[2:]
	return name

def generate_bindings(structs, enums, fields, struct_headers, enum_headers):
	# Sort for deterministic output
	sorted_structs = sorted(list(structs))
	sorted_enums = sorted(list(enums))

	# Check for default constructors and extract bodies
	has_default_ctor = {}
	struct_bodies = {}
	enum_items = {} # enum_name -> list of (name, value_expr)

	for s in sorted_structs:
		path = struct_headers[s]
		with open(path, 'r', encoding='utf-8') as f:
			content = f.read()
			# Simple heuristic: StructName() or StructName() = default
			if re.search(fr'\b{s}\s*\(\s*\)', content):
				has_default_ctor[s] = True
			else:
				has_default_ctor[s] = False

			# Extract body
			# Use balanced brace parser
			struct_bodies[s] = extract_struct_body(content, s)

	for e in sorted_enums:
		path = enum_headers[e]
		with open(path, 'r', encoding='utf-8') as f:
			content = f.read()
			enum_items[e] = extract_enum_items(content, e)

	return generate_cpp_bindings(sorted_structs, sorted_enums, fields, struct_headers, enum_headers, has_default_ctor, enum_items), \
		   generate_python_stubs(sorted_structs, sorted_enums, fields, struct_bodies, has_default_ctor, enum_items)

def generate_cpp_bindings(sorted_structs, sorted_enums, fields, struct_headers, enum_headers, has_default_ctor, enum_items):
	# Collect includes
	includes = set()
	includes.add('#include "pch.h"')
	includes.add('#include "z1engine.h"')

	# Generate relative include paths for each struct
	for s in sorted_structs:
		if s in struct_headers:
			path = struct_headers[s]
			rel_path = os.path.relpath(path, ENGINE_ROOT).replace("\\", "/")
			includes.add(f'#include "{rel_path}"')

	# Generate relative include paths for each enum
	for e in sorted_enums:
		if e in enum_headers:
			path = enum_headers[e]
			rel_path = os.path.relpath(path, ENGINE_ROOT).replace("\\", "/")
			includes.add(f'#include "{rel_path}"')

	# Generate code
	code = []
	code.append("// This file is automatically generated by utils/gen_pybinds.py")
	code.append("// Do not modify this file directly.")
	code.append("")

	# Write includes
	code.append('#include "pch.h"')
	if '#include "pch.h"' in includes:
		includes.remove('#include "pch.h"')
	if '#include "z1engine.h"' in includes:
		code.append('#include "z1engine.h"')
		includes.remove('#include "z1engine.h"')

	sorted_includes = sorted(list(includes))
	for inc in sorted_includes:
		code.append(inc)

	code.append("")
	code.append('#include "pybind11/embed.h"')
	code.append('#include "pybind11/stl.h"')
	code.append("namespace py = pybind11;")
	code.append("using namespace z1;")
	code.append("")

	# Generate binding function
	code.append("void bind_generated(py::module& m, py::class_<Entity, std::shared_ptr<Entity>>& entity_cls) {")

	# Bind Enums
	code.append("\t// Bind Enums")
	for e in sorted_enums:
		code.append(f'\tpy::enum_<{e}>(m, "{e}")')
		for name, val in enum_items[e]:
			code.append(f'\t\t.value("{name}", {e}::{name})')
		code.append(f'\t\t.export_values();')
		code.append("")

	# Generated Bindings
	code.append("\t// Generated Bindings")
	for s in sorted_structs:
		py_name = strip_struct_name(s)
		code.append(f'\tpy::class_<{s}>(m, "{py_name}")')

		if has_default_ctor.get(s, False):
			code.append('\t\t.def(py::init<>())')

		if s in fields:
			for f in fields[s]:
				py_field = strip_field_name(f)
				code.append(f'\t\t.def_readwrite("{py_field}", &{s}::{f})')

		code[-1] += ";"
		code.append("")

	# Auto-generate properties for Entity
	code.append("\t// Entity component properties")
	for s in sorted_structs:
		if s.endswith("Component"):
			py_name = strip_struct_name(s)
			prop_name = re.sub(r'(?<!^)(?=[A-Z])', '_', py_name).lower()

			code.append(f'\tentity_cls.def_property_readonly("{prop_name}", [](Entity& e) -> {s}* {{')
			code.append(f'\t\tif (e.has_component<{s}>()) {{')
			code.append(f'\t\t\treturn &e.get_component<{s}>();')
			code.append(f'\t\t}}')
			code.append(f'\t\treturn nullptr;')
			code.append(f'\t}}, py::return_value_policy::reference);')

	code.append("")

	# Allow access to GlobalSettings via z1.globals
	# This uses the m_global pointer from runtime context
	code.append('\t// Allow access to GlobalSettings via z1.globals or z1.global')
	code.append('\tm.def("__getattr__", [](const std::string &name) -> py::object {')
	code.append('\t\tif (name == "globals" || name == "global") {')
	code.append('\t\t\treturn py::cast(z1::g_runtime_context.m_global.get(), py::return_value_policy::reference);')
	code.append('\t\t}')
	code.append('\t\tif (name == "scene") {')
	code.append('\t\t\treturn py::cast(z1::g_runtime_context.m_scene.get(), py::return_value_policy::reference);')
	code.append('\t\t}')
	code.append('\t\tthrow py::attribute_error("module \'z1\' has no attribute \'" + name + "\'");')
	code.append('\t});')

	code.append("}")
	code.append("")

	return "\n".join(code)

def get_field_type(struct_body, field_name):
	field_escaped = re.escape(field_name)
	# Match: Type field_name [=;{]
	pattern = re.compile(r'(?P<type>[\w:]+(?:<[^;={]+>)?(?:\s*\*|\s*&)?)\s+' + field_escaped + r'\s*(?:[;={])', re.MULTILINE)
	match = pattern.search(struct_body)
	if match:
		return match.group('type').strip()
	return "Any"

def map_cpp_type_to_python(cpp_type, known_enums=None):
	cpp_type = cpp_type.replace("const", "").strip()
	cpp_type = cpp_type.replace("&", "").strip()

	if known_enums and cpp_type in known_enums:
		return cpp_type

	# Containers first!
	if "std::vector" in cpp_type:
		match = re.search(r'std::vector<(.+)>', cpp_type)
		if match:
			inner = map_cpp_type_to_python(match.group(1), known_enums)
			return f"List[{inner}]"
		return "List[Any]"
	if "std::array" in cpp_type:
		match = re.search(r'std::array<([^,]+),.+>', cpp_type)
		if match:
			inner = map_cpp_type_to_python(match.group(1), known_enums)
			return f"List[{inner}]"
		return "List[Any]"
	if "std::shared_ptr" in cpp_type:
		match = re.search(r'std::shared_ptr<(.+)>', cpp_type)
		if match:
			inner = map_cpp_type_to_python(match.group(1), known_enums)
			return f"Optional[{inner}]"
		return "Optional[Any]"

	# Basic types
	if cpp_type in ["int", "uint32_t", "size_t", "long", "short", "int32_t", "int64_t"]:
		return "int"
	if cpp_type in ["float", "double"]:
		return "float"
	if cpp_type == "bool":
		return "bool"
	if cpp_type == "std::string":
		return "str"
	if "glm::vec2" in cpp_type:
		return "Vec2"
	if "glm::vec3" in cpp_type:
		return "Vec3"
	if "glm::vec4" in cpp_type:
		return "Vec4"

	if cpp_type.endswith("Component"):
		return strip_struct_name(cpp_type)

	return "Any"

def parse_manual_bindings(file_path):
	if not os.path.exists(file_path):
		return []

	with open(file_path, 'r', encoding='utf-8') as f:
		content = f.read()

	classes = {} # name -> {'methods': [], 'fields': [], 'inits': []}
	var_to_class = {} # variable_name -> class_name

	# We process line by line to handle chaining somewhat correctly
	# but we also need to handle multiline statements.
	# For simplicity, let's assume standard formatting from the file we just wrote.

	current_class = None

	lines = content.split('\n')
	for line in lines:
		line = line.strip()
		if not line: continue
		if line.startswith('//'): continue

		# Check for class definition
		class_match = PY_CLASS_RE.search(line)
		if class_match:
			var_name = class_match.group('var')
			class_name = class_match.group('name')
			current_class = class_name
			classes[class_name] = {'methods': [], 'fields': [], 'inits': []}
			if var_name:
				var_to_class[var_name] = class_name
			continue

		# Check for init
		init_match = PY_INIT_RE.search(line)
		if init_match:
			# If it's chained (starts with dot), it applies to current_class
			if line.startswith('.'):
				if current_class:
					args_str = init_match.group('args')
					args = [a.strip() for a in args_str.split(',')] if args_str else []
					classes[current_class]['inits'].append(args)
			continue

		# Check for methods
		def_match = PY_DEF_RE.search(line)
		if def_match:
			var_name = def_match.group('var')
			method_name = def_match.group('name')

			target_class = None
			if var_name and var_name in var_to_class:
				target_class = var_to_class[var_name]
			elif line.startswith('.'):
				target_class = current_class

			if target_class:
				classes[target_class]['methods'].append(method_name)
			continue

		# Check for properties/fields
		prop_match = PY_PROP_RE.search(line)
		if prop_match:
			var_name = prop_match.group('var')
			prop_name = prop_match.group('name')

			target_class = None
			if var_name and var_name in var_to_class:
				target_class = var_to_class[var_name]
			elif line.startswith('.'):
				target_class = current_class

			if target_class:
				classes[target_class]['fields'].append(prop_name)
			continue

	return classes

MANUAL_METHOD_OVERRIDES = {
	"Entity": {
		"add_static_mesh": "\tdef add_static_mesh(self, path: str) -> None: ...",
		"add_skeletal_mesh": "\tdef add_skeletal_mesh(self, path: str) -> None: ...",
		"add_camera": "\tdef add_camera(self) -> None: ...",
	},
	"Scene": {
		"create_entity": "\tdef create_entity(self, name: str) -> Entity: ...",
		"destroy_entity": "\tdef destroy_entity(self, entity: Entity) -> None: ...",
	}
}

def generate_python_stubs(sorted_structs, sorted_enums, fields, struct_bodies, has_default_ctor, enum_items):
	# Parse Manual Bindings from C++
	manual_classes = parse_manual_bindings(MANUAL_BINDINGS_FILE)

	code = []
	code.append("# This file is automatically generated by utils/gen_pybinds.py")
	code.append("# Do not modify this file directly.")
	code.append("from typing import Any, overload, ClassVar, List, Optional")
	code.append("from enum import Enum")
	code.append("")

	code.append("def log_info(msg: str) -> None: ...")
	code.append("def log_warn(msg: str) -> None: ...")
	code.append("def log_error(msg: str) -> None: ...")
	code.append("")

	# Generate Manual Classes Stubs
	for cls_name, data in manual_classes.items():
		code.append(f"class {cls_name}:")

		# Fields
		for f in data['fields']:
			# Try to guess type for common fields
			ftype = "Any"
			if cls_name.startswith("Vec") and f in ["x", "y", "z", "w"]:
				ftype = "float"
			code.append(f"\t{f}: {ftype}")

		if not data['fields'] and not data['methods'] and not data['inits']:
			pass

		# Inits
		if data['inits']:
			for args in data['inits']:
				arg_str = "self"
				for i, arg_type in enumerate(args):
					if not arg_type: continue
					py_type = map_cpp_type_to_python(arg_type)
					arg_str += f", arg{i}: {py_type}"
				code.append(f"\t@overload")
				code.append(f"\tdef __init__({arg_str}) -> None: ...")
		elif cls_name == "Script": # Script needs implicit init
			code.append("\tdef __init__(self) -> None: ...")

		# Methods
		for m in data['methods']:
			# Check manual overrides first
			if cls_name in MANUAL_METHOD_OVERRIDES and m in MANUAL_METHOD_OVERRIDES[cls_name]:
				code.append(MANUAL_METHOD_OVERRIDES[cls_name][m])
				continue

			# Special handling for known methods
			if m == "is_valid":
				code.append(f"\tdef {m}(self) -> bool: ...")
			elif m == "on_update":
				code.append(f"\tdef {m}(self, delta: float) -> None: ...")
			elif m.startswith("on_"):
				code.append(f"\tdef {m}(self) -> None: ...")
			elif m == "__repr__":
				code.append(f"\tdef {m}(self) -> str: ...")
			else:
				code.append(f"\tdef {m}(self, *args: Any, **kwargs: Any) -> Any: ...")

		# Inject generated entity body marker if it's Entity class
		if cls_name == "Entity":
			code.append("\t# __GENERATED_ENTITY_BODY__")

		if cls_name == "Script":
			code.append("\t@property")
			code.append("\tdef entity(self) -> Optional[Entity]: ...")

		code.append("")

	code.append("# --- GENERATED CONTENT BELOW ---")


	# Generate Enums
	for e in sorted_enums:
		code.append(f"class {e}(Enum):")
		for i, (name, val) in enumerate(enum_items[e]):
			code.append(f"\t{name} = {i}")
		code.append("")

	# Generate Structs
	for s in sorted_structs:
		py_name = strip_struct_name(s)
		code.append(f"class {py_name}:")

		has_fields = False
		if s in fields and fields[s]:
			has_fields = True
			for f in fields[s]:
				py_field = strip_field_name(f)
				cpp_type = get_field_type(struct_bodies.get(s, ""), f)
				py_type = map_cpp_type_to_python(cpp_type, sorted_enums)
				code.append(f"\t{py_field}: {py_type}")

		if not has_fields and not has_default_ctor.get(s, False):
			code.append("\tpass")

		if has_default_ctor.get(s, False):
			code.append("\tdef __init__(self) -> None: ...")

		code.append("")

	# Generate Entity properties
	entity_props = []
	for s in sorted_structs:
		if s.endswith("Component"):
			py_name = strip_struct_name(s)
			prop_name = re.sub(r'(?<!^)(?=[A-Z])', '_', py_name).lower()
			entity_props.append(f"\t@property")
			entity_props.append(f"\tdef {prop_name}(self) -> Optional[{py_name}]: ...")

	# Inject Entity properties into the Manual Entity class if present
	# We look for "# __GENERATED_ENTITY_BODY__" in the code and replace it
	injected = False
	for i in range(len(code)):
		if "# __GENERATED_ENTITY_BODY__" in code[i]:
			code[i] = "\n".join(entity_props)
			injected = True
			break

	if not injected:
		# Fallback if marker not found
		code.append("# WARNING: Entity properties not injected (marker not found)")

	code.append("globals: GlobalSettings")
	code.append("scene: Scene")
	code.append("")

	return "\n".join(code)

def main():
	print(f"Scanning {ENGINE_ROOT}...")
	files = get_header_files(ENGINE_ROOT)
	structs, enums, fields, struct_headers, enum_headers = parse_files(files)

	if "type" in structs:
		structs.remove("type")

	print(f"Found {len(structs)} structs and {len(enums)} enums.")
	for s in sorted(list(structs)):
		print(f"  - {s}")
	for e in sorted(list(enums)):
		print(f"  - {e}")

	cpp_content, stub_content = generate_bindings(structs, enums, fields, struct_headers, enum_headers)

	print(f"Writing to {OUTPUT_FILE}...")
	with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
		f.write(cpp_content)

	if not os.path.exists(STUBS_DIR):
		os.makedirs(STUBS_DIR)

	print(f"Writing to {STUB_FILE}...")
	with open(STUB_FILE, 'w', encoding='utf-8') as f:
		f.write(stub_content)

	print("Done.")

if __name__ == "__main__":
	main()
