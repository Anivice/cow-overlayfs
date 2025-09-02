#ifndef CPPCOWOVERLAY_BLOCK_H
#define CPPCOWOVERLAY_BLOCK_H

#include <cstdint>
#include <fstream>
#include <utility>
#include <vector>
#include <cstring>
#include <filesystem>
#include "lz4.h"
#include "error.h"
#include "log.hpp"

#ifdef __unix__
# undef LITTLE_ENDIAN
# undef BIG_ENDIAN
#endif // __unix__

namespace cow_block
{
    enum endian_t { LITTLE_ENDIAN, BIG_ENDIAN };

    class CRC64 {
    public:
        CRC64();
        void update(const uint8_t* data, size_t length);

        [[nodiscard]] uint64_t get_checksum(endian_t endian = BIG_ENDIAN
            /* CRC64 tools like 7ZIP display in BIG_ENDIAN */) const;

    private:
        uint64_t crc64_value{};
        uint64_t table[256] {};

        void init_crc64();
        static uint64_t reverse_bytes(uint64_t x);
    };

    [[nodiscard]] inline
    uint64_t hashcrc64(const std::vector<uint8_t> & data) {
        CRC64 hash;
        hash.update(data.data(), data.size());
        return hash.get_checksum();
    }

    template < typename Type >
    concept PODType = std::is_standard_layout_v<Type> && std::is_trivial_v<Type>;

    template < PODType Type >
    [[nodiscard]] uint64_t hashcrc64(const Type & data) {
        CRC64 hash;
        hash.update(static_cast<uint8_t*>(&data), sizeof(data));
        return hash.get_checksum();
    }

    std::string bin2hex(const std::vector < char > &);
    template < PODType Type > std::string bin2hex(const Type & raw)
    {
        std::vector < char > vec(sizeof(raw));
        std::memcpy(vec.data(), &raw, sizeof(raw));
        return bin2hex(vec);
    }

    inline void touch(const std::string & path)
    {
        if (const std::filesystem::path data_path(path); !std::filesystem::exists(data_path))
        {
            std::fstream file(data_path, std::ios::binary);
            file.close();
        }
    }

    inline void mkdir_p(const std::string & path)
    {
        if (!std::filesystem::exists(path))
        {
            std::filesystem::create_directories(path);
        }
    }

    def_except_with_trace(write_into_data_block_failed);

    inline void write_into(const std::string & path, const std::vector < uint8_t > & data)
    {
        if (const std::filesystem::path data_path(path); !std::filesystem::exists(data_path))
        {
            std::ofstream file(data_path, std::ios::binary);
            file.write(reinterpret_cast<const char*>(data.data()), static_cast<ssize_t>(data.size()));
            if (file.tellp() != data.size())
            {
                easy_throw_except(write_into_data_block_failed, "Short write on data block " + path);
            }

            file.close();
        }
    }

    template < typename Type >
    void write_pod(const std::string & path, const Type & data)
    {
        write_into(path,
            std::vector<uint8_t>(
                reinterpret_cast<const uint8_t*>(&data),
                reinterpret_cast<const uint8_t*>(&data) + sizeof(data))
        );
    }

    struct block_attribute_t
    {
        struct {
            bool is_lz4_compressed;
            bool is_frozen;
            bool newly_allocated_block_thus_no_cow;
            enum data_block_type_t:uint8_t { BLOCK_METADATA, BLOCK_COW_REDUNDANCY } data_block_type;
            data_block_type_t data_block_type_backup;
            uint64_t refresh_count;
            uint64_t inode_link_count;
        } information { };
        char padding_[4096 - sizeof (information)] { };
    };
    static_assert(sizeof(block_attribute_t) == 4096);

    def_except_with_trace(block_manager_invalid_argument);

    class block_manager
    {
        std::string data_dir;           /// directory for data
        std::string zero_pointer_name;  /// name for zero pointer (unallocated zeros)
        const uint64_t block_size;      /// block size

    public:
        /// @brief Initializes class members
        /// @param data_dir Directory for all data files
        /// @param blk_sz Block size
        block_manager(std::string data_dir, const uint64_t blk_sz)
            : data_dir(std::move(data_dir)), block_size(blk_sz)
        {
            const std::vector<uint8_t> data(block_size, 0);
            zero_pointer_name = bin2hex(hashcrc64(data));
        }

        /// @brief Write to a block whose path is $DATA_DIR/CRC64_STR(data)
        /// @param data Data of the block whose size must be the same with block_size
        void write_in_block(const std::vector < uint8_t > & data) const
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

        /// @brief set block attribute
        /// @param block_name Name for the block
        /// @param attributes Block attributes
        void set_block_attribute(const std::string & block_name, const block_attribute_t& attributes) const
        {
            write_pod(data_dir + "/" + block_name + ".attr", attributes);
        }

        /// @brief get block attribute
        /// @param block_name Name of the block
        /// @return Attributes for the block
        [[nodiscard]] block_attribute_t get_block_attribute(const std::string & block_name) const
        {
            block_attribute_t attr;
            std::ifstream file(data_dir + "/" + block_name + ".attr", std::ios::binary);
            file.read(reinterpret_cast<char*>(&attr), sizeof(attr));
            return attr;
        }

        /// @brief get block size
        /// @return Block size
        [[nodiscard]] uint64_t get_block_size() const
        {
            return block_size;
        }

        ~block_manager() = default;
        block_manager(const block_manager &) = delete;
        block_manager(block_manager &&) = delete;
        block_manager &operator=(const block_manager &) = delete;
        block_manager &operator=(block_manager &&) = delete;
    };

    def_except_with_trace(log_io_failed);

    class log_manager
    {
    private:
        std::string log_dir;

        struct log_t
        {
            timespec timestamp;
            uint64_t action;
            struct
            {
                struct
                {
                    uint64_t param1;
                    uint64_t param2;
                    uint64_t param3;
                    uint64_t param4;
                    uint64_t param5;
                    uint64_t param6;
                    uint64_t param7;
                } generic;
            } params;
        };

        void trunc_log(uint64_t time_point) const
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

    public:
        explicit log_manager(std::string log_dir) : log_dir(std::move(log_dir))
        {
            mkdir_p(this->log_dir);
        }

        void append_log(const uint64_t action,
            const uint64_t param1 = 0,
            const uint64_t param2 = 0,
            const uint64_t param3 = 0,
            const uint64_t param4 = 0,
            const uint64_t param5 = 0,
            const uint64_t param6 = 0,
            const uint64_t param7 = 0) const
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

        [[nodiscard]] std::vector < log_t > get_last_n_logs(int64_t log_num) const
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
                file.seekg(size - sizeof(log) * (i + 1));
                file.read(reinterpret_cast<char*>(&log), sizeof(log));
                logs.push_back(log);
            }

            return logs;
        }

        ~log_manager() = default;
        log_manager(const log_manager &) = delete;
        log_manager(log_manager&&) = delete;
        log_manager& operator=(const log_manager&) = delete;
        log_manager& operator=(log_manager&&) = delete;
    };
}
#endif //CPPCOWOVERLAY_BLOCK_H
