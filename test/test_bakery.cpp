#include "bakery.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <chrono>

static std::string format_file_size(uintmax_t bytes) {
	constexpr const char* suffixes[] = {"b", "kb", "mb", "gb", "tb"};
	constexpr int num_suffixes = sizeof(suffixes) / sizeof(suffixes[0]);

	if (bytes == 0) return "0 b";

	int suffix_idx = static_cast<int>(log2(bytes) / 10);
	suffix_idx = std::min(suffix_idx, num_suffixes - 1);

	double size = static_cast<double>(bytes) / (1ull << (suffix_idx * 10));

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2) << size << " " << suffixes[suffix_idx];

	std::string result = oss.str();
	size_t dot_pos = result.find(".00");
	if (dot_pos != std::string::npos && dot_pos == result.length() - 3) {
		result.erase(dot_pos, 3);
	}

	return result;
}

static void check_file_size(const std::string& filepath) {
	std::ifstream file(filepath, std::ios::binary | std::ios::ate);
	if (!file) {
		std::cerr << "error: could not open file " << filepath << std::endl;
		return;
	}

	std::streamsize size = file.tellg();
	file.close();

	std::cout << "file: " << filepath << "\n"
			  << "size: " << size << " bytes ("
			  << format_file_size(size) << ")\n";
}

int main() {
	const char* input = "../bakery/asset/wood.png";
	const char* output = "../bakery/asset/wood.bin";

	z1::bakery::compress_image(input, output);

	std::cout << "successfully compressed image: " << input << std::endl;
	std::cout << "output compressed image: " << input << std::endl;

	int w, h;
	const unsigned char* src_data = nullptr;
	const unsigned char* dst_data = nullptr;
	{
		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 10; ++i) {
			src_data = z1::bakery::load_uncompressed_image(input, &w, &h);
			z1::bakery::free_loaded_data(src_data);
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		std::cout << "load_uncompressed_image took " << duration.count() << " us\n";
	}
	{
		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 10; ++i) {
			dst_data = z1::bakery::load_compressed_image(output, &w, &h);
			z1::bakery::free_loaded_data(dst_data);
		}
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		std::cout << "load_compressed_image took " << duration.count() << " us\n";
	}

	src_data = z1::bakery::load_uncompressed_image(input, &w, &h);
	dst_data = z1::bakery::load_compressed_image(output, &w, &h);

	std::cout << "successfully loaded compressed image: " << output << std::endl;

	for (int i = 0; i < w * h * 4; ++i) {
		if (src_data[i] != dst_data[i]) {
			std::cerr << "un-matched data found" << std::endl;
			return 1;
		}
	}

	std::cout << "compressed image passed per-pixel check: " << output << std::endl;

	check_file_size(input);
	check_file_size(output);

	return 0;
}
