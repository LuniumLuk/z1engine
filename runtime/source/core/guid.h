#pragma once

#include <array>
#include <random>
#include <sstream>
#include <iomanip>
#include <string>

namespace z1 {

	struct Guid {
		std::string value;

		Guid() : value("") {}

		bool operator==(const Guid& other) const noexcept { return value == other.value; }

		bool is_valid() const { return !value.empty(); }

		explicit operator const std::string& () const noexcept {
			return value;
		}

		static Guid make(std::string v = "") {
			return Guid(std::move(v));
		}

		static Guid generate() {
			static std::random_device rd;
			static std::mt19937_64 gen(rd());
			static std::uniform_int_distribution<uint64_t> dist;

			uint64_t part1 = dist(gen);
			uint64_t part2 = dist(gen);

			// apply GUID v4 variant/version bits
			part1 &= 0xFFFFFFFFFFFF0FFFULL;
			part1 |= 0x0000000000004000ULL; // version 4
			part2 &= 0x3FFFFFFFFFFFFFFFULL;
			part2 |= 0x8000000000000000ULL; // variant

			std::array<unsigned char, 16> bytes;
			for (int i = 0; i < 8; ++i) bytes[i] = static_cast<unsigned char>((part1 >> ((7 - i) * 8)) & 0xFF);
			for (int i = 0; i < 8; ++i) bytes[i + 8] = static_cast<unsigned char>((part2 >> ((7 - i) * 8)) & 0xFF);

			// format as GUID string
			std::ostringstream oss;
			oss << std::hex << std::setfill('0');
			for (int i = 0; i < 16; ++i) {
				oss << std::setw(2) << static_cast<int>(bytes[i]);
				if (i == 3 || i == 5 || i == 7 || i == 9)
					oss << '-';
			}
			return Guid(oss.str());
		}

		friend std::ostream& operator<<(std::ostream& os, Guid const& guid) {
			return os << guid.value;
		}

	private:
		explicit Guid(std::string v) : value(std::move(v)) {}
	};

}

namespace std {
	template<>
	struct hash<z1::Guid> {
		std::size_t operator()(z1::Guid const& guid) const noexcept {
			return std::hash<std::string>()(guid.value);
		}
	};
}
