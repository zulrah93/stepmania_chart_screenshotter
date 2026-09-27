#ifndef SM_SCREENSHOTTER_HPP
#define SM_SCREENSHOTTER_HPP

#include <stdint.h>
#include <string>
#include <vector>
#include <stdio.h>

constexpr const size_t MAX_BMP_BUFFER_SIZE{128000};

struct bitmap_header_t {
    char magic_field[2];
    uint32_t bitmap_total_size;
    uint32_t reserved;
    uint32_t offset_to_pixels;
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t plane_count;
    uint16_t bits_per_pixel;
    uint32_t compression_method;
    uint32_t data_size;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t color_pallete_count;
    uint32_t important_colors_used;
  } __attribute__((packed));

class sm_screenshotter_t {
public:

    sm_screenshotter_t(const std::string& assets_path) {
        const std::string left_arrow_path{assets_path + "/left_arrow.bmp"};
        const std::string right_arrow_path{assets_path + "/right_arrow.bmp"};
        const std::string up_arrow_path{assets_path + "/up_arrow.bmp"};
        const std::string down_arrow_path{assets_path + "/down_arrow.bmp"};
        
        m_loaded_all_assets = load_32_bits_per_pixel_bitmap(left_arrow_path, m_left_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(left_arrow_path, m_up_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(left_arrow_path, m_right_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(left_arrow_path, m_down_arrow_bmp_buffer);
    }

     operator bool() const {
        return m_loaded_all_assets;
     }

     bool loaded_all_assets() const {
        return m_loaded_all_assets;
     }

     bool save() {
        if (!m_loaded_all_assets) {
            return false;
        }
     }

private:

    std::vector<uint8_t> m_left_arrow_bmp_buffer;
    std::vector<uint8_t> m_right_arrow_bmp_buffer;
    std::vector<uint8_t> m_up_arrow_bmp_buffer;
    std::vector<uint8_t> m_down_arrow_bmp_buffer;
    bool m_loaded_all_assets;

    static bool load_32_bits_per_pixel_bitmap(const std::string& path, std::vector<uint8_t>& bitmap_buffer) {
        FILE* file_handle = fopen(path.c_str(), "rb");
        if (nullptr == file_handle) {
            return false;
        }
        char buffer[MAX_BMP_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        size_t bytes_read = fread(buffer, sizeof(buffer), sizeof(uint8_t), file_handle);
        if (0 == bytes_read) {
            return false;
        }

        bitmap_buffer.append_range(buffer);

        return true;
    }
};

#endif