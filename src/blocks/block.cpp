#include "block.h"
using namespace cow_block;

#ifdef __unix__
# undef LITTLE_ENDIAN
# undef BIG_ENDIAN
#endif // __unix__

CRC64::CRC64() {
    init_crc64();
}

void CRC64::update(const uint8_t* data, const size_t length) {
    for (size_t i = 0; i < length; ++i) {
        crc64_value = table[(crc64_value ^ data[i]) & 0xFF] ^ (crc64_value >> 8);
    }
}

[[nodiscard]] uint64_t CRC64::get_checksum(const endian_t endian
    /* CRC64 tools like 7ZIP display in BIG_ENDIAN */) const
{
    // add the final complement that ECMA‑182 requires
    return (endian == BIG_ENDIAN
        ? reverse_bytes(crc64_value ^ 0xFFFFFFFFFFFFFFFFULL)
        : (crc64_value ^ 0xFFFFFFFFFFFFFFFFULL));
}

void CRC64::init_crc64()
{
    crc64_value = 0xFFFFFFFFFFFFFFFF;
    for (uint64_t i = 0; i < 256; ++i) {
        uint64_t crc = i;
        for (uint64_t j = 8; j--; ) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xC96C5795D7870F42;  // Standard CRC-64 polynomial
            else
                crc >>= 1;
        }
        table[i] = crc;
    }
}

uint64_t CRC64::reverse_bytes(uint64_t x)
{
    x = ((x & 0x00000000FFFFFFFFULL) << 32) | ((x & 0xFFFFFFFF00000000ULL) >> 32);
    x = ((x & 0x0000FFFF0000FFFFULL) << 16) | ((x & 0xFFFF0000FFFF0000ULL) >> 16);
    x = ((x & 0x00FF00FF00FF00FFULL) << 8)  | ((x & 0xFF00FF00FF00FF00ULL) >> 8);
    return x;
}

char hex_table [] = {
    '0', 0x00,
    '1', 0x01,
    '2', 0x02,
    '3', 0x03,
    '4', 0x04,
    '5', 0x05,
    '6', 0x06,
    '7', 0x07,
    '8', 0x08,
    '9', 0x09,
    'a', 0x0A,
    'b', 0x0B,
    'c', 0x0C,
    'd', 0x0D,
    'e', 0x0E,
    'f', 0x0F,
};

void c_bin2hex(const char bin, char hex[2])
{
    auto find_in_table = [](const char p_hex) -> char {
        for (size_t i = 0; i < sizeof(hex_table); i += 2) {
            if (hex_table[i + 1] == p_hex) {
                return hex_table[i];
            }
        }

        throw std::invalid_argument("Invalid binary code");
    };

    const char bin_a = static_cast<char>(bin >> 4 & 0x0F);
    const char bin_b = static_cast<char>(bin & 0x0F);

    hex[0] = find_in_table(bin_a);
    hex[1] = find_in_table(bin_b);
}

std::string bin2hex(const std::vector < char > & vec)
{
    std::string result;
    char buffer [3] { };
    for (const auto & bin : vec) {
        c_bin2hex(bin, buffer);
        result += buffer;
    }

    return result;
}
