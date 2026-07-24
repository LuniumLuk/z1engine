import os
import argparse
import sys
import re

# Pattern: line like "} else {" or "} else if (...) {" — split to put else on its own line
_else_pattern = re.compile(r'^(\s*)\}\s+(else\b.*)$')

def is_ascii(s):
	return all(ord(c) < 128 for c in s)

def convert_to_tabs(line):
	# Find the end of leading whitespace
	i = 0
	while i < len(line) and line[i] in ' \t':
		i += 1

	if i == 0:
		return line

	leading_whitespace = line[:i]
	content = line[i:]

	# Expand tabs to spaces (tab width 4)
	expanded = leading_whitespace.expandtabs(4)
	total_spaces = len(expanded)

	# Convert back to tabs
	tabs = total_spaces // 4
	remaining_spaces = total_spaces % 4

	new_leading = '\t' * tabs + ' ' * remaining_spaces
	return new_leading + content

def process_file(filepath, tasks, dry_run):
	try:
		with open(filepath, 'rb') as f:
			content_bytes = f.read()

		# Try to decode as UTF-8 first
		try:
			content = content_bytes.decode('utf-8')
		except UnicodeDecodeError:
			# Try latin-1 as fallback to not crash, but warn about encoding
			try:
				content = content_bytes.decode('latin-1')
				print(f"[WARN] Decoded {filepath} as latin-1 (contains non-UTF-8 bytes)")
			except:
				print(f"[ERROR] Could not decode {filepath}. Skipping.")
				return

		lines = content.splitlines() # splitlines() handles \r, \n, \r\n

		new_lines = []

		# Check for ASCII if requested
		if 'ascii' in tasks:
			for i, line in enumerate(lines):
				if not all(ord(c) < 128 for c in line):
					print(f"[WARN] Non-ASCII characters found in {filepath}:{i+1}")
					# print(f"       Line: {line.strip()}") # Optional: too verbose

		for i, line in enumerate(lines):
			original_line = line
			current_line = line

			# Task 2: Delete trailing whitespaces
			if 'trailing' in tasks:
				current_line = current_line.rstrip()

			# Task 3: Convert indentation spaces to tabs
			if 'tabs' in tasks:
				current_line = convert_to_tabs(current_line)

			# Task 4: Put else on its own line (split "} else" patterns)
			if 'else-nl' in tasks:
				m = _else_pattern.match(current_line)
				if m:
					indent = m.group(1)
					rest = m.group(2)  # "else ..."
					new_lines.append(indent + '}')
					new_lines.append(indent + rest)
					continue

			new_lines.append(current_line)

		# Task 5: CRLF check - We always apply CRLF if 'crlf' is in tasks or if we modified content
		# Actually, if we just reconstruct with \r\n join, we enforce CRLF.

		if not new_lines:
			new_content = ""
		else:
			new_content = "\r\n".join(new_lines) + "\r\n"

		new_content_bytes = new_content.encode('utf-8')

		# Determine if we should write
		should_write = False

		if content_bytes != new_content_bytes:
			should_write = True

		# If 'crlf' is NOT in tasks, we should check if the only difference is line endings.
		# But for simplicity and to respect the general requirement "all line endings should be CRLF",
		# we will enforce CRLF if we touch the file.
		# However, if 'crlf' is NOT in tasks, and there are NO other changes (trailing, tabs),
		# we ideally shouldn't modify the file just for CRLF.

		if should_write and 'crlf' not in tasks:
			# Check if stripped content is identical (ignoring whitespace differences potentially caused by tabs/trailing?)
			# No, we just want to know if logic changes happened.
			# This is hard to detect efficiently without tracking flags.
			pass

		if should_write:
			if dry_run:
				print(f"[DRY-RUN] Would modify: {filepath}")
			else:
				print(f"[FIX] Modifying: {filepath}")
				with open(filepath, 'wb') as f:
					f.write(new_content_bytes)

	except Exception as e:
		print(f"[ERROR] Failed to process {filepath}: {e}")

def main():
	parser = argparse.ArgumentParser(description="Code formatter utility.")
	parser.add_argument('--extensions', type=str, default=".h,.cpp,.glsl,.py", help="Comma-separated list of extensions (default: .h,.cpp)")
	parser.add_argument('--tasks', type=str, default="all", help="Comma-separated list of tasks: trailing, tabs, ascii, crlf, else-nl (or all)")
	parser.add_argument('--dry-run', action='store_true', help="Run without modifying files")

	args = parser.parse_args()

	root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
	folders = [
		os.path.join(root, 'engine/runtime'),
		os.path.join(root, 'engine/content'),
		os.path.join(root, 'engine/editor'),
		os.path.join(root, 'engine/game'),
		os.path.join(root, 'engine/test'),
		os.path.join(root, 'engine/tool'),
	]
	extensions = [e.strip() for e in args.extensions.split(',')]

	available_tasks = {'trailing', 'tabs', 'ascii', 'crlf', 'else-nl'}
	if args.tasks == 'all':
		tasks = available_tasks
	else:
		tasks = set([t.strip() for t in args.tasks.split(',')])

	# Validate tasks
	for t in tasks:
		if t not in available_tasks:
			print(f"Unknown task: {t}")
			return

	print(f"Scanning folders: {folders}")
	print(f"Extensions: {extensions}")
	print(f"Tasks: {tasks}")
	print(f"Dry run: {args.dry_run}")
	print("-" * 40)

	for folder in folders:
		if not os.path.exists(folder):
			print(f"Folder not found: {folder}")
			continue

		for root, _, files in os.walk(folder):
			for file in files:
				if any(file.endswith(ext) for ext in extensions):
					process_file(os.path.join(root, file), tasks, args.dry_run)

if __name__ == "__main__":
	main()
