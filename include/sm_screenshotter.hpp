#ifndef SM_SCREENSHOTTER_HPP
#define SM_SCREENSHOTTER_HPP

#include <iterator>
#include <stdint.h>
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


enum stepmania_note_type_t : char {
    empty = '0',
    tap = '1',
    hold = '2',
    roll = '3',
    mine = 'M'
};

struct stepmania_note_rows_t {
    stepmania_note_type_t notes[4];
};

struct stepmania_chart_t {
    uint8_t level;
    bool is_single_chart;
    std::vector<stepmania_note_type_t> note_rows;
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
        
        m_loaded_all_assets = load_32_bits_per_pixel_bitmap(left_arrow_path, m_left_arrow_bmp_header, m_left_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(right_arrow_path, m_right_arrow_bmp_header, m_right_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(up_arrow_path, m_up_arrow_bmp_header, m_up_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(down_arrow_path, m_down_arrow_bmp_header, m_down_arrow_bmp_buffer);
        m_loaded_sim_file = {};
        m_loaded_all_assets &= load_sim_file(chart_path, m_loaded_sim_file);
    }

     operator bool() const {
        return m_loaded_all_assets;
     }

     bool loaded_all_assets() const {
        return m_loaded_all_assets;
     }

     bool save() {
        return m_loaded_all_assets;
     }

private:

    std::vector<uint8_t> m_left_arrow_bmp_buffer;
    std::vector<uint8_t> m_right_arrow_bmp_buffer;
    std::vector<uint8_t> m_up_arrow_bmp_buffer;
    std::vector<uint8_t> m_down_arrow_bmp_buffer;
    bitmap_header_t m_left_arrow_bmp_header;
    bitmap_header_t m_right_arrow_bmp_header;
    bitmap_header_t m_up_arrow_bmp_header;
    bitmap_header_t m_down_arrow_bmp_header;
    stepmania_sim_file_t m_loaded_sim_file;
    bool m_loaded_all_assets;

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

        std::cout << bytes_to_read << " bytes read using mmap..." << std::endl;

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
                        std::cout << "BPM is " << sim_file.bpm << std::endl;
                    }

                    if ("#NOTES" == key_name) {
                      std::cout << value_as_string << std::endl;
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

    static bool load_32_bits_per_pixel_bitmap(const std::string& path, bitmap_header_t& header, std::vector<uint8_t>& pixel_buffer) {
        
        pixel_buffer.reserve(MAX_BMP_BUFFER_SIZE);

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

        header = *reinterpret_cast<bitmap_header_t*>(buffer);

        for(size_t index = 0; index < bytes_read; index++) {
            pixel_buffer.push_back(buffer[index]);
        }

        return true;
    }
};

#endif