import os

def count_cpp_lines(target_directory, exclude_dirs=None):
	if exclude_dirs is None:
		exclude_dirs = set()
	else:
		exclude_dirs = set(exclude_dirs)

	file_data = [] # List to store (file_path, line_count)
	extensions = ('.cpp', '.h', '.hpp', '.cc', '.cxx', '.glsl')

	for root, dirs, files in os.walk(target_directory):
		dirs[:] = [d for d in dirs if d not in exclude_dirs]

		for file in files:
			if file.lower().endswith(extensions):
				file_path = os.path.join(root, file)
				try:
					with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
						lines = sum(1 for line in f)
						# Store relative path for cleaner output
						rel_path = os.path.relpath(file_path, target_directory)
						file_data.append((rel_path, lines))
				except Exception as e:
					print(f'Could not read {file_path}: {e}')

	# Sort files by line count (index 1) in descending order
	file_data.sort(key=lambda x: x[1], reverse=True)

	# Output Ranked List
	print('\n' + '='*60)
	print(f'{"FILE PATH":<45} | {"LINES":>10}')
	print('-'*60)

	total_lines = 0
	for path, count in file_data:
		print(f'{path[:45]:<45} | {count:>10}')
		total_lines += count

	print('='*60)
	print(f'Total Files Processed: {len(file_data)}')
	print(f'Total Lines of Code:    {total_lines}')
	print('='*60)

if __name__ == '__main__':
	file_dir = os.path.dirname(os.path.abspath(__file__))
	root_dir = os.path.abspath(os.path.join(file_dir, '../engine'))
	count_cpp_lines(root_dir, exclude_dirs=['3rdparty', 'bin', 'intermediate'])