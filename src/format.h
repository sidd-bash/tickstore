#include <cstdint>

struct file_header {
    uint32_t magic_num;
    uint16_t version;
    char symbol[8];
    uint8_t reserved[1];
};


struct block_header {
    int64_t ts_min;
    int64_t ts_max;
    uint32_t num_ticks;
    uint32_t length;
    uint8_t codec[3];
    uint8_t reserved[1];
    uint32_t checksum;
};

static_assert(sizeof(file_header) == 16);
static_assert(sizeof(block_header) == 32);