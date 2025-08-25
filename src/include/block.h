#ifndef CPPCOWOVERLAY_BLOCK_H
#define CPPCOWOVERLAY_BLOCK_H

#include <fstream>
#include <utility>
#include <vector>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include "lz4.h"

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

    class block_manager
    {
        std::string attribute_dir;
        std::string data_dir;
        std::string zero_pointer_name;
        const uint64_t block_size;

    public:
        block_manager(std::string attribute_dir, std::string data_dir, const uint64_t blk_sz)
            : attribute_dir(std::move(attribute_dir)), data_dir(std::move(data_dir)), block_size(blk_sz)
        {
            const std::vector<uint8_t> data(block_size, 0);
            zero_pointer_name = bin2hex(hashcrc64(data));
        }

        void allocate_block()
        {
            if (const std::filesystem::path data_path(data_dir + "/" + zero_pointer_name);
                !std::filesystem::exists(data_path))
            {
                std::fstream file(data_path, std::ios::binary);
                file.close();
            }
        }

        class block_t
        {
        private:
            std::vector<uint8_t> data;

        public:
            explicit block_t(const uint64_t block_size, const std::string & zero_pointer_name)
            {

            }
        };

        ~block_manager() = default;
        block_manager(const block_manager &) = delete;
        block_manager(block_manager &&) = delete;
        block_manager &operator=(const block_manager &) = delete;
        block_manager &operator=(block_manager &&) = delete;
    };
}
#endif //CPPCOWOVERLAY_BLOCK_H