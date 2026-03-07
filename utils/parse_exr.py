import os
import sys
os.environ['OPENCV_IO_ENABLE_OPENEXR'] = '1'
import cv2

def parse_exr(exr_path):
	if not os.path.exists(exr_path):
		print(f"File {exr_path} does not exist.")
		sys.exit(1)

	# Read the EXR file using OpenCV
	exr_image = cv2.imread(exr_path, cv2.IMREAD_UNCHANGED)

	if exr_image is None:
		print(f"Failed to read EXR file: {exr_path}")
		sys.exit(1)

	return exr_image

if __name__ == "__main__":
	world_dir = os.path.dirname(os.path.abspath(__file__))
	world_dir = os.path.join(os.path.dirname(world_dir), "asset", "world")

	for file in os.listdir(world_dir):
		if file.endswith(".exr"):
			exr_path = os.path.join(world_dir, file)
			exr_image = parse_exr(exr_path)
			print(f"Parsed EXR file: {exr_path}, shape: {exr_image.shape}")

			cv2.imwrite(exr_path, exr_image)
