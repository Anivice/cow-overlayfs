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

std::string cow_block::bin2hex(const std::vector < char > & vec)
{
    std::string result;
    char buffer [3] { };
    for (const auto & bin : vec) {
        c_bin2hex(bin, buffer);
        result += buffer;
    }

    return result;
}

block_manager::block_manager(std::string data_dir, const uint64_t blk_sz)
    : data_dir(std::move(data_dir)), block_size(blk_sz)
{
    const std::vector<uint8_t> data(block_size, 0);
    zero_pointer_name = bin2hex(hashcrc64(data));
    mkdir_p(data_dir);
}

void block_manager::write_in_block(const std::vector < uint8_t > & data) const
{
    if (data.size() != block_size) {
        throw block_manager_invalid_argument("Data size is not equal to block size");
    }

    const std::string file_name = bin2hex(hashcrc64(data));

    // skip writes for full zeros
    if (file_name == zero_pointer_name)
    {
        return;
    }

    const std::string path_name = data_dir + "/" + file_name;
    write_into(file_name, data);
}

void block_manager::set_block_attribute(const std::string & block_name, const block_attribute_t& attributes) const
{
    write_pod(data_dir + "/" + block_name + ".attr", attributes);
}

[[nodiscard]] block_attribute_t block_manager::get_block_attribute(const std::string & block_name) const
{
    block_attribute_t attr;
    std::ifstream file(data_dir + "/" + block_name + ".attr", std::ios::binary);
    file.read(reinterpret_cast<char*>(&attr), sizeof(attr));
    return attr;
}

[[nodiscard]] uint64_t block_manager::get_block_size() const
{
    return block_size;
}

void log_manager::trunc_log(uint64_t time_point) const
{
    log_t log { };
    std::ifstream file(log_dir + "/log", std::ios::binary);
    std::ofstream file_new(log_dir + "/log.new", std::ios::binary);

    if (!file || !file_new)
    {
        easy_throw_except(log_io_failed, "Failed to open log file");
    }

    while (log.timestamp.tv_sec < time_point)
    {
        file.read(reinterpret_cast<char*>(&log), sizeof(log));
    }

    while (file.read(reinterpret_cast<char*>(&log), sizeof(log)))
    {
        file_new.write(reinterpret_cast<const char*>(&log), sizeof(log));
    }

    std::filesystem::rename(log_dir + "/log.new", log_dir + "/log");
}

void log_manager::append_log(const uint64_t action,
            const uint64_t param1,
            const uint64_t param2,
            const uint64_t param3,
            const uint64_t param4,
            const uint64_t param5,
            const uint64_t param6,
            const uint64_t param7) const
{
    log_t log { };
    if (timespec_get(&log.timestamp, TIME_UTC) == 0)
    {
        warning_log("Failed to get current time for log\n");
    }

    log.params.generic.param1 = param1;
    log.params.generic.param2 = param2;
    log.params.generic.param3 = param3;
    log.params.generic.param4 = param4;
    log.params.generic.param5 = param5;
    log.params.generic.param6 = param6;
    log.params.generic.param7 = param7;
    log.action = action;
    std::ofstream file(log_dir + "/log", std::ios::binary | std::ios::app);
    if (!file)
    {
        easy_throw_except(log_io_failed, "Failed to open log file");
    }
    file.write(reinterpret_cast<const char*>(&log), sizeof(log));
    file.close();
}

[[nodiscard]] std::vector < log_manager::log_t > log_manager::get_last_n_logs(int64_t log_num) const
{
    std::vector < log_t > logs;
    std::ifstream file(log_dir + "/log", std::ios::binary);

    if (!file)
    {
        easy_throw_except(log_io_failed, "Failed to open log file");
    }

    for (int64_t i = 0; i < log_num; i++)
    {
        log_t log { };
        file.seekg(0, std::ios::end);
        const uint64_t size = file.tellg();
        if (size <= sizeof(log) * (i + 1))
        {
            break;
        }
        file.seekg(size - sizeof(log) * (i + 1));
        file.read(reinterpret_cast<char*>(&log), sizeof(log));
        logs.push_back(log);
    }

    return logs;
}
