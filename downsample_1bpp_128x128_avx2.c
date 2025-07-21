#include <immintrin.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Helper: popcount for 128 bits using AVX2 intrinsics
static inline int popcount128(const uint8_t *data) {
    // Load 128 bits (16 bytes)
    __m128i v = _mm_loadu_si128((const __m128i*)data);
    // Split into two 64-bit chunks
    uint64_t lo = _mm_cvtsi128_si64(v);
    uint64_t hi = _mm_extract_epi64(v, 1);
    return (int)(__builtin_popcountll(lo) + __builtin_popcountll(hi));
}

// Downsample a 1bpp image by 128x128 using AVX2
// Input:  in_data   - pointer to input image (bitpacked, 1bpp)
//         in_w      - input width in pixels (must be divisible by 128)
//         in_h      - input height in pixels (must be divisible by 128)
// Output: out_data  - pointer to output image (bitpacked, 1bpp, size = (in_w/128)*(in_h/128)/8 bytes)
//         Each output bit is set if majority of the corresponding 128x128 input block is 1.
void downsample_1bpp_128x128_avx2(const uint8_t *in_data, int in_w, int in_h, uint8_t *out_data) {
    const int block_size = 128;
    const int out_w = in_w / block_size;
    const int out_h = in_h / block_size;
    const int in_row_bytes = in_w / 8;
    const int out_row_bytes = (out_w + 7) / 8;
    memset(out_data, 0, out_row_bytes * out_h);

    for (int out_y = 0; out_y < out_h; ++out_y) {
        for (int out_x = 0; out_x < out_w; ++out_x) {
            int sum = 0;
            // Process 128 rows for this block
            for (int dy = 0; dy < block_size; ++dy) {
                const uint8_t *row_ptr = in_data + (out_y * block_size + dy) * in_row_bytes + (out_x * 16);
                // Each 128 bits = 16 bytes
                sum += popcount128(row_ptr);
            }
            // Threshold: majority (8192 = 128*128/2)
            uint8_t out_bit = (sum >= 8192) ? 1 : 0;
            int bit_index = out_x % 8;
            int byte_index = (out_y * out_row_bytes) + (out_x / 8);
            if (out_bit)
                out_data[byte_index] |= (1 << (7 - bit_index));
            // else, bit is already zero due to memset
        }
    }
}