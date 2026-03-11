import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import subprocess
import os
import pathlib
import re

class ToolTip:
	def __init__(self, widget):
		self.widget = widget
		self.tip_window = None

	def show(self, text, x, y):
		if self.tip_window or not text:
			return

		self.tip_window = tw = tk.Toplevel(self.widget)
		tw.wm_overrideredirect(True)
		tw.wm_geometry(f"+{x+15}+{y+10}")

		label = tk.Label(tw, text=text, justify=tk.LEFT,
						 background="#ffffe0", relief=tk.SOLID, borderwidth=1,
						 font=("tahoma", "8", "normal"))
		label.pack(ipadx=1)

	def hide(self):
		if self.tip_window:
			self.tip_window.destroy()
			self.tip_window = None

class ImporterGUI:
	def __init__(self, root):
		self.root = root
		self.root.title("z1engine Importer")
		self.root.geometry("1000x700")

		# Paths
		self.script_dir = pathlib.Path(__file__).parent.absolute()
		self.repo_root = self.script_dir.parents[2]
		self.importer_exe = self.repo_root / "engine" / "bin" / "importer.exe"
		self.content_dir = self.repo_root / "content"
		self.asset_dir = self.repo_root / "asset"

		if not self.importer_exe.exists():
			messagebox.showwarning("Warning", f"Importer executable not found at {self.importer_exe}.\nPlease build the project first.")

		# Mapping
		self.guid_to_path = {}
		self.path_to_item = {}

		# Layout
		main_frame = ttk.Frame(root, padding="10")
		main_frame.pack(fill=tk.BOTH, expand=True)

		# Top bar
		top_frame = ttk.Frame(main_frame)
		top_frame.pack(fill=tk.X, pady=5)

		ttk.Button(top_frame, text="Import New Asset", command=self.open_import_dialog).pack(side=tk.LEFT)
		ttk.Button(top_frame, text="Refresh", command=self.refresh_content).pack(side=tk.LEFT, padx=5)

		# Split view
		paned = ttk.PanedWindow(main_frame, orient=tk.HORIZONTAL)
		paned.pack(fill=tk.BOTH, expand=True)

		# Left: Tree View
		left_frame = ttk.LabelFrame(paned, text="Content Browser", padding=5)
		paned.add(left_frame, weight=1)

		self.tree = ttk.Treeview(left_frame)
		self.tree.pack(fill=tk.BOTH, expand=True)
		self.tree.bind("<<TreeviewSelect>>", self.on_tree_select)

		scrollbar = ttk.Scrollbar(left_frame, orient="vertical", command=self.tree.yview)
		scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
		self.tree.configure(yscrollcommand=scrollbar.set)

		# Right: Details
		right_frame = ttk.LabelFrame(paned, text="Details", padding=5)
		paned.add(right_frame, weight=2)

		self.details_text = tk.Text(right_frame, wrap=tk.NONE)
		self.details_text.pack(fill=tk.BOTH, expand=True)
		self.tooltip = ToolTip(self.details_text)

		# Configure link tag
		self.details_text.tag_config("link", foreground="blue", underline=1)
		self.details_text.tag_bind("link", "<Button-1>", self.on_link_click)
		self.details_text.tag_bind("link", "<Enter>", self.on_link_enter)
		self.details_text.tag_bind("link", "<Leave>", self.on_link_leave)

		self.refresh_content()

	def build_guid_index(self):
		self.guid_to_path = {}
		if not self.content_dir.exists():
			return

		for root, _, files in os.walk(self.content_dir):
			for file in files:
				if file.endswith(".yaml"):
					full_path = pathlib.Path(root) / file
					try:
						with open(full_path, "r") as f:
							content = f.read()
							match = re.search(r"guid:\s*([0-9a-fA-F-]+)", content)
							if match:
								guid = match.group(1)
								rel_path = full_path.relative_to(self.content_dir)
								self.guid_to_path[guid] = rel_path.as_posix()
					except:
						pass

	def refresh_content(self):
		self.build_guid_index()
		self.path_to_item = {}

		for i in self.tree.get_children():
			self.tree.delete(i)

		if not self.content_dir.exists():
			return

		# Walk and collect all paths
		all_paths = []
		for root, dirs, files in os.walk(self.content_dir):
			rel_root = pathlib.Path(root).relative_to(self.content_dir)

			# Add directories
			for d in dirs:
				path = rel_root / d
				if str(rel_root) == ".":
					path = pathlib.Path(d)
				all_paths.append((path.as_posix(), "dir"))

			# Add files
			for f in files:
				if f.endswith(".bin"): continue # Skip binaries
				path = rel_root / f
				if str(rel_root) == ".":
					path = pathlib.Path(f)
				all_paths.append((path.as_posix(), "file"))

		# Sort paths
		all_paths.sort(key=lambda x: x[0])

		for path_str, kind in all_paths:
			parts = pathlib.Path(path_str).parts
			parent_id = ""
			current_id = ""

			for i, part in enumerate(parts):
				current_path = pathlib.Path(*parts[:i+1]).as_posix()

				if current_path in self.path_to_item:
					current_id = self.path_to_item[current_path]
				else:
					if i > 0:
						parent_path = pathlib.Path(*parts[:i]).as_posix()
						parent_id = self.path_to_item.get(parent_path, "")
					else:
						parent_id = ""

					new_id = self.tree.insert(parent_id, "end", text=part, open=False, values=(current_path,))
					self.path_to_item[current_path] = new_id
					current_id = new_id

	def on_tree_select(self, event):
		selected_items = self.tree.selection()
		if not selected_items:
			return

		item_id = selected_items[0]
		# values is a tuple/list, take first element
		values = self.tree.item(item_id, "values")
		if not values:
			return

		path_str = values[0]
		full_path = self.content_dir / path_str

		if full_path.is_file():
			self.show_file_details(full_path)
		else:
			self.details_text.delete("1.0", tk.END)
			self.details_text.insert(tk.END, f"Directory: {path_str}")

	def show_file_details(self, path):
		self.details_text.delete("1.0", tk.END)
		try:
			# Try UTF-8 first
			try:
				with open(path, "r", encoding="utf-8") as f:
					content = f.read()
			except UnicodeDecodeError:
				self.details_text.insert(tk.END, "Binary or non-UTF-8 file content not shown.")
				return

			self.details_text.insert(tk.END, content)
			self.highlight_guids(content)
		except Exception as e:
			self.details_text.insert(tk.END, f"Error reading file: {e}")

	def highlight_guids(self, content):
		uuid_pattern = r"[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"

		start_pos = "1.0"
		while True:
			count_var = tk.IntVar()
			pos = self.details_text.search(uuid_pattern, start_pos, stopindex=tk.END, regexp=True, count=count_var)
			if not pos: break

			end_pos = f"{pos}+{count_var.get()}c"

			guid = self.details_text.get(pos, end_pos)
			if guid in self.guid_to_path:
				self.details_text.tag_add("link", pos, end_pos)

			start_pos = end_pos

	def on_link_click(self, event):
		guid = self.get_guid_at(event.x, event.y)
		if guid:
			self.navigate_to_guid(guid)

	def on_link_enter(self, event):
		self.details_text.config(cursor="hand2")
		guid = self.get_guid_at(event.x, event.y)
		if guid and guid in self.guid_to_path:
			self.tooltip.show(self.guid_to_path[guid], event.x_root, event.y_root)

	def on_link_leave(self, event):
		self.details_text.config(cursor="")
		self.tooltip.hide()

	def get_guid_at(self, x, y):
		try:
			index = self.details_text.index(f"@{x},{y}")
			tags = self.details_text.tag_names(index)
			if "link" in tags:
				ranges = self.details_text.tag_ranges("link")
				for i in range(0, len(ranges), 2):
					start = ranges[i]
					end = ranges[i+1]
					if self.details_text.compare(start, "<=", index) and self.details_text.compare(index, "<", end):
						return self.details_text.get(start, end)
		except tk.TclError:
			pass
		return None

	def navigate_to_guid(self, guid):
		if guid in self.guid_to_path:
			path_str = self.guid_to_path[guid]
			if path_str in self.path_to_item:
				item_id = self.path_to_item[path_str]
				parent = self.tree.parent(item_id)
				while parent:
					self.tree.item(parent, open=True)
					parent = self.tree.parent(parent)

				self.tree.selection_set(item_id)
				self.tree.see(item_id)
				self.on_tree_select(None)
			else:
				messagebox.showinfo("Info", f"Asset found at {path_str} but not in tree?")
		else:
			messagebox.showinfo("Info", "GUID not found in index.")

	def open_import_dialog(self):
		file_path = filedialog.askopenfilename(
			initialdir=self.asset_dir,
			title="Select Asset to Import",
			filetypes=(("All Files", "*.*"), ("Textures", "*.png;*.jpg;*.tga"), ("Models", "*.glb;*.gltf;*.obj"))
		)

		if not file_path:
			return

		self.show_import_settings(file_path)

	def show_import_settings(self, source_file):
		# Dialog for settings
		dialog = tk.Toplevel(self.root)
		dialog.title("Import Settings")
		dialog.geometry("400x300")

		ttk.Label(dialog, text=f"Importing: {pathlib.Path(source_file).name}").pack(pady=10)

		# Output path
		ttk.Label(dialog, text="Output Path (relative to content):").pack(anchor=tk.W, padx=10)
		out_var = tk.StringVar(value=pathlib.Path(source_file).stem)
		ttk.Entry(dialog, textvariable=out_var).pack(fill=tk.X, padx=10)

		# Type override
		ttk.Label(dialog, text="Type (Auto-detect if empty):").pack(anchor=tk.W, padx=10)
		type_var = tk.StringVar()
		type_combo = ttk.Combobox(dialog, textvariable=type_var, values=["", "texture", "gltf", "obj"])
		type_combo.pack(fill=tk.X, padx=10)

		# Texture Settings
		ttk.Label(dialog, text="Sampler (Texture only):").pack(anchor=tk.W, padx=10)
		sampler_var = tk.StringVar(value="linear")
		ttk.Combobox(dialog, textvariable=sampler_var, values=["linear", "nearest"]).pack(fill=tk.X, padx=10)

		ttk.Label(dialog, text="Wrap (Texture only):").pack(anchor=tk.W, padx=10)
		wrap_var = tk.StringVar(value="repeat")
		ttk.Combobox(dialog, textvariable=wrap_var, values=["repeat", "mirror", "clamp", "border"]).pack(fill=tk.X, padx=10)

		def run_import():
			out_path = out_var.get()
			type_val = type_var.get()
			sampler = sampler_var.get()
			wrap = wrap_var.get()

			cmd = [str(self.importer_exe), source_file, out_path]
			if type_val:
				cmd.extend(["--type", type_val])
			if sampler:
				cmd.extend(["--sampler", sampler])
			if wrap:
				cmd.extend(["--wrap", wrap])

			print(f"Running: {' '.join(cmd)}")

			try:
				result = subprocess.run(cmd, capture_output=True, text=True, cwd=self.repo_root)
				if result.returncode == 0:
					messagebox.showinfo("Success", f"Import successful!\n{result.stdout}")
					dialog.destroy()
					self.refresh_content()
				else:
					messagebox.showerror("Error", f"Import failed:\n{result.stderr}\n{result.stdout}")
			except Exception as e:
				messagebox.showerror("Error", f"Failed to run importer: {e}")

		ttk.Button(dialog, text="Import", command=run_import).pack(pady=20)

if __name__ == "__main__":
	root = tk.Tk()
	app = ImporterGUI(root)
	root.mainloop()

