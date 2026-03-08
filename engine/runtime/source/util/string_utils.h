#include <string>

namespace z1 {

	inline size_t find_paired_brackets(std::string const& code, size_t start) {
		size_t pos = code.find('{', start);
		size_t counter = 1;
		while (counter > 0 && pos != std::string::npos) {
			pos += 1;
			if (code[pos] == '{') {
				counter += 1;
			}
			else if (code[pos] == '}') {
				counter -= 1;
			}
		}
		return pos;
	}

	inline std::string remove_comments(std::string const& code) {
		std::string result;
		result.reserve(code.size()); // reserve space for efficiency

		bool in_single_line_comment = false;
		bool in_multi_line_comment = false;

		for (size_t i = 0; i < code.size(); ++i) {
			if (!in_single_line_comment && !in_multi_line_comment) {
				// check for start of single-line comment
				if (i + 1 < code.size() && code[i] == '/' && code[i + 1] == '/') {
					in_single_line_comment = true;
					i++; // skip the next '/'
					continue;
				}
				// check for start of multi-line comment
				else if (i + 1 < code.size() && code[i] == '/' && code[i + 1] == '*') {
					in_multi_line_comment = true;
					i++; // skip the '*'
					continue;
				}
				// not in a comment - add to result
				result += code[i];
			}
			else if (in_single_line_comment) {
				// check for end of single-line comment (newline)
				if (code[i] == '\n') {
					in_single_line_comment = false;
					result += '\n'; // keep the newline
				}
			}
			else if (in_multi_line_comment) {
				// check for end of multi-line comment
				if (i + 1 < code.size() && code[i] == '*' && code[i + 1] == '/') {
					in_multi_line_comment = false;
					i++; // skip the '/'
				}
			}
		}

		return result;
	}

	inline std::string process_includes(const std::string& input, const std::string& search_dir) {
		std::string result;
		std::istringstream input_stream(input);
		std::string line;
		std::regex include_regex(R"(^\s*#include\s*<([^>]+)>\s*$)");

		while (std::getline(input_stream, line)) {
			std::smatch match;
			if (std::regex_match(line, match, include_regex)) {
				// found an #include directive
				std::string filename = match[1].str();
				std::ifstream file(search_dir + filename);

				if (file.is_open()) {
					// read the entire file content
					std::stringstream file_stream;
					file_stream << file.rdbuf();
					file.close();

					// replace the #include line with file content
					result += file_stream.str() + "\n";
				}
				else {
					CORE_ERROR("failed to open included file: {0}", search_dir + filename);
					// if the file cannot be opened, keep the #include line as-is
					result += line + "\n";
				}
			}
			else {
				// not an #include line, add it as-is
				result += line + "\n";
			}
		}

		return result;
	}

	inline std::vector<std::string> split(const std::string& s, char delimiter) {
		std::vector<std::string> tokens;
		std::string token;
		std::istringstream token_stream(s);
		while (std::getline(token_stream, token, delimiter)) {
			if (!token.empty()) {
				tokens.push_back(token);
			}
		}
		return tokens;
	}

	inline bool is_number(const std::string& s) {
		std::istringstream iss(s);
		float val;
		iss >> val;

		// check if entire string was consumed and no failbit set
		return iss.eof() && !iss.fail();
	}

	inline bool is_blank(const std::string& str) {
		for (char ch : str) {
			if (!std::isspace(static_cast<unsigned char>(ch))) {
				return false;
			}
		}
		return true;
	}

}
