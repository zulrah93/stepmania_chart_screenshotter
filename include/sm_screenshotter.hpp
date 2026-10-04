#ifndef SM_SCREENSHOTTER_HPP
#define SM_SCREENSHOTTER_HPP

#include <cstdint>
#include <cstdio>
#include <iterator>
#include <stdint.h>
#include <algorithm>
#include <bitset>
#include <utility>
#include <string>
#include <sys/fcntl.h>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

constexpr const size_t MAX_BMP_BUFFER_SIZE{128000};
constexpr const size_t MAX_FONT_BUFFER_SIZE{9000};

using rgb_t = uint32_t;
using pixel_data_t = std::vector<rgb_t>;

constexpr rgb_t make_rgb(uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) {
    return 0;
}

// To render the screenshot...
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

// For rendering the text a linux screen font format...
 struct psf_header_t {
    uint32_t magic;
    uint32_t version;
    uint32_t this_header_size; // Note: Will be always set to sizeof(psf_header_t)
    uint32_t has_unicode_table;
    uint32_t glyph_size; //
    uint32_t bytes_per_glyph;
    uint32_t glyph_height;
    uint32_t glyph_width;
  };

  static constexpr size_t MAX_FONT_BITMAP_SIZE{16*512};

  struct font_t {
    psf_header_t font_header;
    uint8_t font_bitmap[MAX_FONT_BITMAP_SIZE];
  };


enum stepmania_note_type_t : char {
    empty = '0',
    tap = '1',
    hold = '2',
    roll = '3',
    mine = 'M'
};

struct stepmania_measure_t {
    std::vector<stepmania_note_type_t> notes;
};

struct stepmania_chart_t {
    bool is_single_chart;
    uint16_t level;
    std::vector<stepmania_measure_t> measures;
};


struct stepmania_sim_file_t {
    double bpm; // lowest bpm to highest
    std::string title;
    std::string artist;
    std::vector<stepmania_chart_t> charts;
};

class sm_screenshotter_t {
public:

    sm_screenshotter_t(const std::string& assets_path, const std::string& chart_path) {
        const std::string left_arrow_path{assets_path + "/left_arrow.bmp"};
        const std::string right_arrow_path{assets_path + "/right_arrow.bmp"};
        const std::string up_arrow_path{assets_path + "/up_arrow.bmp"};
        const std::string down_arrow_path{assets_path + "/down_arrow.bmp"};
        const std::string mine_arrow_path{assets_path + "/mine.bmp"};
        m_left_arrow_bmp_header = {};
        m_right_arrow_bmp_header = {};
        m_up_arrow_bmp_header = {};
        m_down_arrow_bmp_header = {};
        m_mine_bmp_header = {};
        m_loaded_all_assets = load_32_bits_per_pixel_bitmap(left_arrow_path, m_left_arrow_bmp_header, m_left_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(right_arrow_path, m_right_arrow_bmp_header, m_right_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(up_arrow_path, m_up_arrow_bmp_header, m_up_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(down_arrow_path, m_down_arrow_bmp_header, m_down_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(mine_arrow_path, m_mine_bmp_header, m_mine_bmp_buffer);
        m_loaded_sim_file = {};
        m_loaded_all_assets &= load_sim_file(chart_path, m_loaded_sim_file);
        m_loaded_font = {};
        m_loaded_all_assets &= load_pc_screen_font_file(assets_path + "/font.psf", m_loaded_font);

        m_height = 0;
        for(const auto& measure : m_loaded_sim_file.charts[0].measures) {
           m_height += (measure.notes.size() / 4ul);
        }

        m_height *= 132;

        std::cout << "width=" << m_width << " m_height=" << m_height << std::endl;

        initialize_bitmap();

        //plot_left_arrow(m_height - 300, 300);
       // plot_down_arrow(m_height - 300, 900);
        plot_up_arrow(m_height - 300  , 300);
        plot_right_arrow(m_height - 300, 600);
        plot_mine_arrow(m_height - 300, 10);

         //for(size_t y{}; y < 10000; y++) {
            //plot_pixel(400 + y, 400 + y, 0x0);
            plot_glyph(m_loaded_font, '$', m_height - 200, 400);
            for(size_t y{}; y <= 9; y++)
                plot_glyph(m_loaded_font, '0' + y, m_height - 220, 440 + y + 32);
      //  }

      /*  
       */
        //  plot_pixel(m_height - 1, 0, 0xff, 0x00, 0x00);
       // for(uint8_t offset{}; offset <= 127; offset++) 
         //   plot_glyph(m_loaded_font,  '\0' + offset,  m_height - 32, m_width - (32 * offset));
         

    }

     operator bool() const {
        return m_loaded_all_assets;
     }

     bool loaded_all_assets() const {
        return m_loaded_all_assets;
     }

     bool save(const std::string& path) {
        FILE* bitmap_handle = fopen(path.c_str(), "wb");
        if (nullptr == bitmap_handle) {
            fclose(bitmap_handle);
            return false;
        }
        
        size_t bytes_written{0};
        bytes_written = fwrite(reinterpret_cast<uint8_t*>(&m_bitmap_header), sizeof(uint8_t), sizeof(bitmap_header_t), bitmap_handle);
        
        if (sizeof(bitmap_header_t) != bytes_written) {
            fclose(bitmap_handle);
            return false;
        }

       // std::reverse(m_pixel_buffer.begin(), m_pixel_buffer.end());

        bytes_written += fwrite(m_pixel_buffer.data(), sizeof(rgb_t), m_pixel_buffer.size(), bitmap_handle);
        std::cout << "Saving " << bytes_written << std::endl;
        fclose(bitmap_handle);
        return bytes_written == (sizeof(bitmap_header_t) + m_pixel_buffer.size());
     }

private:

    pixel_data_t m_left_arrow_bmp_buffer;
    pixel_data_t m_right_arrow_bmp_buffer;
    pixel_data_t m_up_arrow_bmp_buffer;
    pixel_data_t m_down_arrow_bmp_buffer;
    pixel_data_t m_mine_bmp_buffer;
    bitmap_header_t m_left_arrow_bmp_header;
    bitmap_header_t m_right_arrow_bmp_header;
    bitmap_header_t m_up_arrow_bmp_header;
    bitmap_header_t m_down_arrow_bmp_header;
    bitmap_header_t m_mine_bmp_header;
    stepmania_sim_file_t m_loaded_sim_file;
    bool m_loaded_all_assets;
    const size_t m_width{800};
    size_t m_height;
    bitmap_header_t m_bitmap_header;
    pixel_data_t m_pixel_buffer;
    font_t m_loaded_font;

    void plot_pixel(size_t x, size_t y, rgb_t rgb) {
        m_pixel_buffer[(y * m_width) + x] = rgb;
    }

    void plot_left_arrow(size_t x, size_t y) {
        const uint32_t width = m_left_arrow_bmp_header.width;
        const uint32_t height = m_left_arrow_bmp_header.height;
        size_t index{};
        for(const rgb_t& rgb : m_left_arrow_bmp_buffer) {
            plot_pixel((index % width) + y, (index / width) + x, rgb);
            index++;
        }
    }

    void plot_right_arrow(size_t x, size_t y) {
        const uint32_t width = m_right_arrow_bmp_header.width;
        size_t index{};
        for(const rgb_t& rgb : m_right_arrow_bmp_buffer) {
            plot_pixel((index % width) + y, (index / width) + x, rgb);
            index++;
        }
    }

    void plot_up_arrow(const size_t x, const size_t y) {
        const uint32_t width = m_up_arrow_bmp_header.width;
        size_t index{};
        for(const rgb_t& rgb : m_up_arrow_bmp_buffer) {
            plot_pixel((index % width) + y, (index / width) + x, rgb);
            index++;
        }
    }

    void plot_down_arrow(const size_t x, const size_t y) {
        const uint32_t width = m_down_arrow_bmp_header.width;
        size_t index{};
        for(const rgb_t& rgb : m_down_arrow_bmp_buffer) {
            plot_pixel((index % width) + y, (index / width) + x, rgb);
            index++;
        }
    }

    void plot_mine_arrow(const size_t x, const size_t y) {
        const uint32_t width = m_mine_bmp_header.width;
        size_t index{};
        for(const rgb_t& rgb : m_mine_bmp_buffer) {
            plot_pixel((index % width) + y, (index / width) + x, rgb);
            index++;
        }
    }

    void plot_glyph(const font_t& font_handle, const uint8_t glyph_index, const size_t x, const size_t y) {
       const size_t index{static_cast<size_t>(font_handle.font_header.bytes_per_glyph) *  static_cast<size_t>(glyph_index)};
       for(size_t row_index{}; row_index < font_handle.font_header.bytes_per_glyph; row_index++) {
            std::bitset<8> row{font_handle.font_bitmap[index + row_index]};
            for(int32_t column_index{row.size() - 1}; column_index >= 0; column_index--) {
                if (row[column_index]) {
                    plot_pixel((y * font_handle.font_header.glyph_height) + row_index, 
                            (x + font_handle.font_header.glyph_width) + column_index, 0x00);
                }
            }
       }
    }

    void initialize_bitmap() {
        const size_t total_size{m_width * m_height};

        m_bitmap_header = {};
        m_bitmap_header.magic_field[0] = 'B';
        m_bitmap_header.magic_field[1] = 'M';
        m_bitmap_header.offset_to_pixels = sizeof(bitmap_header_t);
        m_bitmap_header.plane_count = 1;
        m_bitmap_header.compression_method = 0;
        m_bitmap_header.horizontal_resolution = m_bitmap_header.vertical_resolution = 1;
        m_bitmap_header.color_pallete_count = 0;
        m_bitmap_header.important_colors_used = 0;
        m_bitmap_header.bits_per_pixel = 32;
        m_bitmap_header.width = m_width;
        m_bitmap_header.height = m_height;
        m_bitmap_header.data_size = sizeof(bitmap_header_t) + total_size;
        m_bitmap_header.header_size = 40;

        m_pixel_buffer.reserve(total_size);
       
        for(size_t x = 0; x < m_width; x++) {
            for(size_t y = 0; y < m_height; y++) {
               m_pixel_buffer.push_back(0xffffffff);
            }
        }
    }

    static bool load_sim_file(const std::string& path, stepmania_sim_file_t& sim_file) {

        int sim_file_descriptor = open(path.c_str(), O_RDONLY);
        struct stat file_stat;
        if (-1 == fstat(sim_file_descriptor, &file_stat)) {
            close(sim_file_descriptor);
            return false;
        }

        const size_t bytes_to_read{static_cast<size_t>(file_stat.st_size)};
        char* entire_file_data = static_cast<char*>(mmap(nullptr, 
                  bytes_to_read, PROT_READ, MAP_PRIVATE, sim_file_descriptor, 0));

        if (MAP_FAILED == entire_file_data) {
            close(sim_file_descriptor);
            return false;
        }

        if (nullptr == entire_file_data) {
            close(sim_file_descriptor);
            return false;
        }

        std::string current_line;
        for(size_t index = 0; index < bytes_to_read; index++) {
            const char& current_char = entire_file_data[index];
            switch(current_char) {

                case '\n': {
                    continue;
                }

                case '\r': {
                    continue;
                }

                case ';': {
                    current_line += ';';
                    std::string key_name;
                    std::string temp_string;
                    std::string value_as_string;
                    for(size_t index = 0; index < current_line.size(); index++) {
                        const char& current_char = current_line[index];
                        switch(current_char) {
                            case ';': {
                                 value_as_string = temp_string;
                                 temp_string = "";
                                 break;
                            };
                            case ':': {
                                key_name = temp_string;
                                temp_string = "";
                                break;
                            }
                            default: {
                                temp_string += current_char;
                                break;
                            }
                        }
                    }
                    
                    if ("#TITLE" == key_name) {
                        sim_file.title = value_as_string;
                    }

                    if ("#ARTIST" == key_name) {
                        sim_file.artist = value_as_string;
                    }

                    if ("#BPMS" == key_name) {
                        value_as_string += ';';
                        std::string temp_string;
                        for(size_t index = 0; index < value_as_string.size(); index++) {
                            const char& current_char = value_as_string[index];
                            switch(current_char) {
                                case ';': {
                                    sim_file.bpm = std::stod(temp_string);
                                    temp_string = "";
                                    break;
                                };
                                case '=': {
                                    temp_string = "";
                                    break;
                                }
                                default: {
                                    temp_string += current_char;
                                    break;
                                }
                            }
                        }
                    }

                    stepmania_chart_t chart;

                    if ("#STEPSTYPE" == key_name) {
                        chart.is_single_chart =  ("#STEPSTYPE" == key_name) && ("dance-single" == value_as_string);
                    }

                    if ("#METER" == key_name) {
                         chart.level = std::stod(value_as_string);
                    }
 
                    if ("#NOTES" == key_name) {
                   
                     stepmania_measure_t current_measure;
                     for(size_t index = 0; index < value_as_string.size(); index++) {
                        const char current_char = value_as_string[index];
                        
                          switch(current_char) {
                                case ',': {
                                    chart.measures.push_back(current_measure);
                                    current_measure = {};
                                    break;
                                }
                                case stepmania_note_type_t::empty: {
                                    current_measure.notes.push_back(stepmania_note_type_t::empty);
                                    break;
                                }
                                case stepmania_note_type_t::hold: {
                                    current_measure.notes.push_back(stepmania_note_type_t::hold);
                                    break;
                                }
                                case stepmania_note_type_t::mine: {
                                    current_measure.notes.push_back(stepmania_note_type_t::mine);
                                    break;
                                }
                                case stepmania_note_type_t::tap: {
                                    current_measure.notes.push_back(stepmania_note_type_t::tap);
                                    break;
                                }
                                default: {
                                    current_measure = {};
                                    break;
                                }
                            }
                      }
                      sim_file.charts.push_back(chart);
                    }

                    current_line = "";
                    break;
                }

                default: {
                    current_line += current_char;
                    break;
                }

            };
            
        }

        close(sim_file_descriptor);

        return true;
    }

    static bool load_pc_screen_font_file(const std::string& path, font_t& font_handle) {

        FILE* file_handle = fopen(path.c_str(), "rb");
        if (nullptr == file_handle) {
            return false;
        }

        char buffer[MAX_FONT_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        size_t bytes_read = fread(buffer, sizeof(uint8_t), sizeof(buffer), file_handle);
        if (0 == bytes_read) {
            return false;
        }

        font_handle.font_header = *reinterpret_cast<psf_header_t*>(buffer);

        std::cout << "Loaded a font @ " << path << " that has a size of " << font_handle.font_header.glyph_width << " x " << font_handle.font_header.glyph_height << std::endl;


        std::cout << "header_size=" << font_handle.font_header.this_header_size << std::endl;

        memset(font_handle.font_bitmap, 0, sizeof(font_handle.font_bitmap));
        size_t bitmap_index{};
        for(size_t index = font_handle.font_header.this_header_size; index < bytes_read; index++) {
            font_handle.font_bitmap[bitmap_index++] = buffer[index];
        }

        return true;
    }

    static bool load_32_bits_per_pixel_bitmap(const std::string& path, bitmap_header_t& header, pixel_data_t& pixel_buffer) {
        
        pixel_buffer.reserve(MAX_BMP_BUFFER_SIZE);

        FILE* file_handle = fopen(path.c_str(), "rb");
        if (nullptr == file_handle) {
            return false;
        }
        char buffer[MAX_BMP_BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        size_t bytes_read = fread(buffer, sizeof(uint8_t), sizeof(buffer), file_handle);
        if (0 == bytes_read) {
            return false;
        }

        header = *reinterpret_cast<bitmap_header_t*>(buffer);

        std::cout << "path=" << path << " header_size=" << header.header_size  << " offset_to_pixels=" << header.offset_to_pixels << std::endl;

        rgb_t* pixel_data = reinterpret_cast<rgb_t*>(buffer + header.offset_to_pixels);

        for(size_t index = header.offset_to_pixels; index < ((bytes_read - header.offset_to_pixels) / sizeof(rgb_t)); index++) {
            pixel_buffer.push_back(pixel_data[index]);
        }

        std::reverse(pixel_buffer.begin(), pixel_buffer.end());

        return true;
    }
};

#endif
