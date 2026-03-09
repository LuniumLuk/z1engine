import os
import re

# Use absolute path based on the script location
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ENGINE_ROOT = os.path.join(SCRIPT_DIR, "..", "engine", "runtime", "source")
STUBS_DIR = os.path.join(SCRIPT_DIR, "..", "engine", "stubs")
OUTPUT_FILE = os.path.join(ENGINE_ROOT, "python", "py_engine.cpp")
STUB_FILE = os.path.join(STUBS_DIR, "z1.pyi")

# Regex patterns
REFLECTED_STRUCT_RE = re.compile(r'REFLECTED_STRUCT\s*\(\s*(\w+)\s*\)')
REFLECTED_FIELD_RE = re.compile(r'REFLECTED_FIELD\s*\(\s*(\w+)\s*,\s*(\w+)\s*,')
REFLECT_ENUM_RE = re.compile(r'REFLECT_ENUM\s*\(\s*(\w+)\s*,')

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
	# Find start
	pattern = r'REFLECT_ENUM\s*\(\s*' + enum_name + r'\s*,'
	match = re.search(pattern, content)
	if not match: return []

	start_idx = match.end()
	# Read until closing parenthesis of REFLECT_ENUM
	# Need to balance parenthesis
	count = 1
	i = start_idx
	while i < len(content) and count > 0:
		if content[i] == '(': count += 1
		elif content[i] == ')': count -= 1
		i += 1

	if count == 0:
		items_str = content[start_idx : i-1]
		# Parse items: { "Name", Value }, ...
		# Simple regex: \{ "(\w+)", ([^}]+) \}
		item_re = re.compile(r'\{\s*"(\w+)"\s*,\s*([^}]+)\s*\}')
		return item_re.findall(items_str)
	return []

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
	# pch.h must be first
	code.append('#include "pch.h"')
	if '#include "pch.h"' in includes:
		includes.remove('#include "pch.h"')

	# z1engine.h next
	if '#include "z1engine.h"' in includes:
		code.append('#include "z1engine.h"')
		includes.remove('#include "z1engine.h"')

	# Sort remaining includes
	sorted_includes = sorted(list(includes))
	for inc in sorted_includes:
		code.append(inc)

	code.append("")
	code.append('#include "pybind11/embed.h"')
	code.append('#include "pybind11/stl.h"')
	code.append("namespace py = pybind11;")
	code.append("")
	code.append("using namespace z1;")
	code.append("")
	code.append("void ForceLinkPythonEngine() {}")
	code.append("")
	code.append("static void log_info_py(std::string const& msg) { CLIENT_INFO(msg); }")
	code.append("static void log_warn_py(std::string const& msg) { CLIENT_WARN(msg); }")
	code.append("static void log_error_py(std::string const& msg) { CLIENT_ERROR(msg); }")
	code.append("")

	code.append("// This macro \"creates\" the 'z1' module inside the Python VM")
	code.append("PYBIND11_EMBEDDED_MODULE(z1, m) {")
	code.append('\tCORE_INFO("Initializing z1 Python module");')
	code.append('\tm.doc() = "z1 Engine API";')
	code.append('\tm.def("log_info", &log_info_py);')
	code.append('\tm.def("log_warn", &log_warn_py);')
	code.append('\tm.def("log_error", &log_error_py);')
	code.append("")

	# Bind GLM
	code.append("\t// Bind GLM")
	code.append('\tpy::class_<glm::vec2>(m, "Vec2")')
	code.append('\t\t.def(py::init<float, float>())')
	code.append('\t\t.def(py::init<>())')
	code.append('\t\t.def_readwrite("x", &glm::vec2::x)')
	code.append('\t\t.def_readwrite("y", &glm::vec2::y)')
	code.append('\t\t.def("__repr__", [](const glm::vec2& v) {')
	code.append('\t\t\treturn "Vec2(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ")";')
	code.append('\t\t});')
	code.append("")

	code.append('\tpy::class_<glm::vec3>(m, "Vec3")')
	code.append('\t\t.def(py::init<float, float, float>())')
	code.append('\t\t.def(py::init<>())')
	code.append('\t\t.def_readwrite("x", &glm::vec3::x)')
	code.append('\t\t.def_readwrite("y", &glm::vec3::y)')
	code.append('\t\t.def_readwrite("z", &glm::vec3::z)')
	code.append('\t\t.def("__repr__", [](const glm::vec3& v) {')
	code.append('\t\t\treturn "Vec3(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";')
	code.append('\t\t});')
	code.append("")

	code.append('\tpy::class_<glm::vec4>(m, "Vec4")')
	code.append('\t\t.def(py::init<float, float, float, float>())')
	code.append('\t\t.def(py::init<>())')
	code.append('\t\t.def_readwrite("x", &glm::vec4::x)')
	code.append('\t\t.def_readwrite("y", &glm::vec4::y)')
	code.append('\t\t.def_readwrite("z", &glm::vec4::z)')
	code.append('\t\t.def_readwrite("w", &glm::vec4::w)')
	code.append('\t\t.def("__repr__", [](const glm::vec4& v) {')
	code.append('\t\t\treturn "Vec4(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ", " + std::to_string(v.w) + ")";')
	code.append('\t\t});')
	code.append("")

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

	# Bind Entity (Manual)
	code.append("\t// Bind Entity")
	code.append('\tpy::class_<Entity, std::shared_ptr<Entity>>(m, "Entity")')

	# Auto-generate properties for all components
	for s in sorted_structs:
		if s.endswith("Component"):
			py_name = strip_struct_name(s)
			prop_name = re.sub(r'(?<!^)(?=[A-Z])', '_', py_name).lower()

			# Special case for TransformComponent to match previous "transform" property which was lowercase
			# Actually snake case of "Transform" is "transform"
			# Snake case of "SkeletalMesh" is "skeletal_mesh"

			code.append(f'\t\t.def_property_readonly("{prop_name}", [](Entity& e) -> {s}* {{')
			code.append(f'\t\t\tif (e.has_component<{s}>()) {{')
			code.append(f'\t\t\t\treturn &e.get_component<{s}>();')
			code.append(f'\t\t\t}}')
			code.append(f'\t\t\treturn nullptr;')
			code.append(f'\t\t}}, py::return_value_policy::reference)')

	code.append('\t\t.def("is_valid", &Entity::is_valid);')
	code.append("")

	# Bind Script (Manual)
	code.append("\t// Helper class for Python to inherit from (mocking ScriptBase)")
	code.append("\tstruct PyScript {")
	code.append("\t\tvirtual void on_attach() {}")
	code.append("\t\tvirtual void on_update(float) {}")
	code.append("\t\tvirtual void on_detach() {}")
	code.append("\t\tvirtual ~PyScript() = default;")
	code.append("\t};")
	code.append("")
	code.append("\t// Bind Script Base Class for Python to inherit")
	code.append('\tpy::class_<PyScript>(m, "Script")')
	code.append('\t\t.def(py::init<>())')
	code.append('\t\t.def("on_attach", &PyScript::on_attach)')
	code.append('\t\t.def("on_update", &PyScript::on_update)')
	code.append('\t\t.def("on_detach", &PyScript::on_detach);')

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
			return inner
		return "Any"

	if cpp_type.endswith("Component"):
		return strip_struct_name(cpp_type)

	return "Any"

def generate_python_stubs(sorted_structs, sorted_enums, fields, struct_bodies, has_default_ctor, enum_items):
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

	for e in sorted_enums:
		code.append(f"class {e}(Enum):")
		for i, (name, val) in enumerate(enum_items[e]):
			code.append(f"\t{name} = {i}")
		code.append("")

	code.append("class Vec2:")
	code.append("\tx: float")
	code.append("\ty: float")
	code.append("\t@overload")
	code.append("\tdef __init__(self) -> None: ...")
	code.append("\t@overload")
	code.append("\tdef __init__(self, x: float, y: float) -> None: ...")
	code.append("\tdef __repr__(self) -> str: ...")
	code.append("")

	code.append("class Vec3:")
	code.append("\tx: float")
	code.append("\ty: float")
	code.append("\tz: float")
	code.append("\t@overload")
	code.append("\tdef __init__(self) -> None: ...")
	code.append("\t@overload")
	code.append("\tdef __init__(self, x: float, y: float, z: float) -> None: ...")
	code.append("\tdef __repr__(self) -> str: ...")
	code.append("")

	code.append("class Vec4:")
	code.append("\tx: float")
	code.append("\ty: float")
	code.append("\tz: float")
	code.append("\tw: float")
	code.append("\t@overload")
	code.append("\tdef __init__(self) -> None: ...")
	code.append("\t@overload")
	code.append("\tdef __init__(self, x: float, y: float, z: float, w: float) -> None: ...")
	code.append("\tdef __repr__(self) -> str: ...")
	code.append("")

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

	code.append("class Entity:")
	for s in sorted_structs:
		if s.endswith("Component"):
			py_name = strip_struct_name(s)
			prop_name = re.sub(r'(?<!^)(?=[A-Z])', '_', py_name).lower()
			code.append("\t@property")
			code.append(f"\tdef {prop_name}(self) -> Optional[{py_name}]: ...")

	code.append("\tdef is_valid(self) -> bool: ...")
	code.append("")

	code.append("class Script:")
	code.append("\tdef __init__(self) -> None: ...")
	code.append("\tdef on_attach(self) -> None: ...")
	code.append("\tdef on_update(self, delta_time: float) -> None: ...")
	code.append("\tdef on_detach(self) -> None: ...")
	code.append("\t@property")
	code.append("\tdef entity(self) -> Optional[Entity]: ...")
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
