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
            uint64_t snapshot_version_count; // how many snapshots referenced this block
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
        block_manager(std::string data_dir, uint64_t blk_sz);

        /// @brief Write to a block whose path is $DATA_DIR/CRC64_STR(data)
        /// @param data Data of the block whose size must be the same with block_size
        void write_in_block(const std::vector < uint8_t > & data) const;

        /// @brief set block attribute
        /// @param block_name Name for the block
        /// @param attributes Block attributes
        void set_block_attribute(const std::string & block_name, const block_attribute_t& attributes) const;

        /// @brief get block attribute
        /// @param block_name Name of the block
        /// @return Attributes for the block
        [[nodiscard]] block_attribute_t get_block_attribute(const std::string & block_name) const;

        /// @brief get block size
        /// @return Block size
        [[nodiscard]] uint64_t get_block_size() const;

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

        /// remove all logs before time_point
        /// @param time_point Log validation point
        void trunc_log(uint64_t time_point) const;

    public:
        explicit log_manager(std::string log_dir) : log_dir(std::move(log_dir))
        {
            mkdir_p(this->log_dir);
        }

        /// @brief Append log
        /// @param action Log Action
        /// @param param... Parameter ... (the action determines Parameter number)
        void append_log(uint64_t action,
            uint64_t param1 = 0, uint64_t param2 = 0,
            uint64_t param3 = 0, uint64_t param4 = 0,
            uint64_t param5 = 0, uint64_t param6 = 0,
            uint64_t param7 = 0) const;
        [[nodiscard]] std::vector < log_t > get_last_n_logs(int64_t log_num) const;

        ~log_manager() = default;
        log_manager(const log_manager &) = delete;
        log_manager(log_manager&&) = delete;
        log_manager& operator=(const log_manager&) = delete;
        log_manager& operator=(log_manager&&) = delete;
    };
}
#endif //CPPCOWOVERLAY_BLOCK_H
