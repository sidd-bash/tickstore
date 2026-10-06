#include <cstdint>
#include <vector>

struct file_header {
    uint32_t magic_num;
    uint16_t version;
    char symbol[8];
    int8_t price_precision;
    int8_t volume_precision;
};


struct block_header {
    int64_t ts_min;
    int64_t ts_max;
    uint32_t num_ticks;
    uint32_t length;
    uint8_t codec[3];
    uint8_t reserved[1];
    uint32_t checksum;
    uint32_t sequence;
};

static_assert(sizeof(file_header) == 16);
static_assert(sizeof(block_header) == 40);

struct tick {
    int64_t ts; 
    int64_t price;
    int64_t volume;
};

// false return value means failure, true means success
bool serialize_block (const std::vector<tick>& ticks, block_header& bh, uint8_t* out, size_t out_size);

// false return value means failure, true means success
bool serialize_file_header (const file_header& fh, uint8_t* out, size_t out_size);


bool deserialize_block (std::vector<tick>& ticks, block_header& bh, const uint8_t* buf, size_t buf_size);

// false return value means failure, true means success
bool deserialize_file_header (file_header& fh, const uint8_t* buf, size_t buf_size);

