#pragma once

#include <cstdint>
#include <cstdio>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>

namespace z1 {

	// Deterministic 128-bit asset id: word 0 holds the first 8 hex digits.
	// Engine-root ids keep word 0 zero, other roots never do (see from_root_and_path).
	struct Guid {
		uint32_t m_data[4]{ 0, 0, 0, 0 };

		Guid() = default;

		Guid(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3)
			: m_data{ w0, w1, w2, w3 } {
		}

		bool operator==(Guid const& other) const noexcept {
			return m_data[0] == other.m_data[0] && m_data[1] == other.m_data[1]
				&& m_data[2] == other.m_data[2] && m_data[3] == other.m_data[3];
		}

		bool operator!=(Guid const& other) const noexcept { return !(*this == other); }

		bool is_valid() const noexcept {
			return (m_data[0] | m_data[1] | m_data[2] | m_data[3]) != 0;
		}

		std::string to_string() const {
			char buffer[33] = {};
			std::snprintf(buffer, sizeof(buffer), "%08x%08x%08x%08x",
				m_data[0], m_data[1], m_data[2], m_data[3]);
			return buffer;
		}

		// Parses 32 hex digits (dashes tolerated); any other input yields an invalid guid.
		static Guid from_string(std::string_view text) {
			Guid guid;
			int digit = 0;
			for (char c : text) {
				if (c == '-') continue;
				int value = hex_digit_value(c);
				if (value < 0 || digit >= 32) return Guid();
				guid.m_data[digit / 8] = (guid.m_data[digit / 8] << 4) | (uint32_t)value;
				++digit;
			}
			if (digit != 32) return Guid();
			return guid;
		}

		// Engine assets derive from the path alone and keep a zero 32-bit prefix.
		static Guid from_path(std::string const& path) {
			uint64_t high = 0;
			uint64_t low = 0;
			derive_hash(path, high, low);
			return from_hash(high, low, true);
		}

		// Other assets derive from root + path and never carry the engine prefix.
		static Guid from_root_and_path(std::string const& root, std::string const& path) {
			if (root == "engine") {
				return from_path(path);
			}
			uint64_t high = 0;
			uint64_t low = 0;
			derive_hash(root + "/" + path, high, low);
			return from_hash(high, low, false);
		}

		// Applies the engine-prefix / foreign-flip rule to a raw 128-bit hash.
		static Guid from_hash(uint64_t high, uint64_t low, bool engine_asset) {
			Guid guid{ (uint32_t)(high >> 32), (uint32_t)high, (uint32_t)(low >> 32), (uint32_t)low };
			if (engine_asset) {
				guid.m_data[0] = 0;
			}
			else if (guid.m_data[0] == 0) {
				guid.m_data[0] = 0x80000000u;
			}
			return guid;
		}

		friend std::ostream& operator<<(std::ostream& os, Guid const& guid) {
			return os << guid.to_string();
		}

	private:
		static constexpr uint64_t FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL;
		static constexpr uint64_t FNV_PRIME = 0x00000100000001b3ULL;

		static uint64_t fnv1a64(uint64_t hash, std::string_view text) noexcept {
			for (unsigned char c : text) {
				hash ^= (uint64_t)c;
				hash *= FNV_PRIME;
			}
			return hash;
		}

		// Two domain-separated FNV-1a-64 passes form the 128-bit hash.
		static void derive_hash(std::string_view key, uint64_t& high, uint64_t& low) {
			high = fnv1a64(fnv1a64(FNV_OFFSET_BASIS, "z1a:"), key);
			low = fnv1a64(fnv1a64(FNV_OFFSET_BASIS, "z1b:"), key);
		}

		static int hex_digit_value(char c) noexcept {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			return -1;
		}
	};

}

namespace std {
	template<>
	struct hash<z1::Guid> {
		std::size_t operator()(z1::Guid const& guid) const noexcept {
			uint64_t high = ((uint64_t)guid.m_data[0] << 32) | guid.m_data[1];
			uint64_t low = ((uint64_t)guid.m_data[2] << 32) | guid.m_data[3];
			uint64_t mixed = high ^ (low * 0x9e3779b97f4a7c15ULL);
			return (std::size_t)(mixed ^ (mixed >> 32));
		}
	};
}
