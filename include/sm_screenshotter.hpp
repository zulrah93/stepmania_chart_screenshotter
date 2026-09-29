#ifndef SM_SCREENSHOTTER_HPP
#define SM_SCREENSHOTTER_HPP

#include <cstdint>
#include <cstdio>
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
        m_left_arrow_bmp_header = {};
        m_right_arrow_bmp_header = {};
        m_up_arrow_bmp_header = {};
        m_down_arrow_bmp_header = {};
        m_loaded_all_assets = load_32_bits_per_pixel_bitmap(left_arrow_path, m_left_arrow_bmp_header, m_left_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(right_arrow_path, m_right_arrow_bmp_header, m_right_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(up_arrow_path, m_up_arrow_bmp_header, m_up_arrow_bmp_buffer);
        m_loaded_all_assets &= load_32_bits_per_pixel_bitmap(down_arrow_path, m_down_arrow_bmp_header, m_down_arrow_bmp_buffer);
        m_loaded_sim_file = {};
        m_loaded_all_assets &= load_sim_file(chart_path, m_loaded_sim_file);

        m_height = 0;
        for(const auto& measure : m_loaded_sim_file.charts[0].measures) {
           m_height += (measure.notes.size() / 4ul);
        }

        m_height *= 132;

        const size_t total_size{m_width * m_height * sizeof(uint32_t)};

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
                m_pixel_buffer.push_back(0xff);
                m_pixel_buffer.push_back(0xff);
                m_pixel_buffer.push_back(0xff);
                m_pixel_buffer.push_back(0xff);
                //plot_pixel(x, y, 0xff, 0xff, 0xff, 0xff);
            }
        }

        plot_left_arrow(10, 800);
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
        
        size_t bytes_read{0};
        bytes_read = fwrite(reinterpret_cast<uint8_t*>(&m_bitmap_header), sizeof(uint8_t), sizeof(bitmap_header_t), bitmap_handle);
        
        if (sizeof(bitmap_header_t) != bytes_read) {
            fclose(bitmap_handle);
            return false;
        }

        bytes_read += fwrite(m_pixel_buffer.data(), sizeof(uint8_t), m_pixel_buffer.size(), bitmap_handle);
        fclose(bitmap_handle);
        return bytes_read == (sizeof(bitmap_header_t) + m_pixel_buffer.size());
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
    const size_t m_width{600};
    size_t m_height;
    bitmap_header_t m_bitmap_header;
    std::vector<uint8_t> m_pixel_buffer;

    void plot_byte(size_t x, size_t y, uint8_t byte) {
        //std::cout << "plot_pixel x=" << x << " y=" << y << " rgb=" << rgb << std::endl;
        m_pixel_buffer[(y * m_width * 4) + x + 1] = byte;
    }

    void plot_pixel(size_t x, size_t y, uint8_t alpha, uint8_t red, uint8_t green, uint8_t blue) {
        //std::cout << "plot_pixel x=" << x << " y=" << y << " rgb=" << rgb << std::endl;
        m_pixel_buffer[(y * m_width) + x] = blue;
        m_pixel_buffer[((y * m_width) + x) + 1] = green;
        m_pixel_buffer[((y * m_width) + x) + 2] = red;
        m_pixel_buffer[((y * m_width) + x) + 3] = alpha;
    }



    void plot_left_arrow(size_t x, size_t y) {
        const uint32_t width = m_left_arrow_bmp_header.width * 4;
        const uint32_t height = m_left_arrow_bmp_header.height * 4;
        size_t index{((y * width) + x) + (width * height)};
        std::cout << m_left_arrow_bmp_buffer.size() << std::endl;
        for(const auto& byte : m_left_arrow_bmp_buffer) {
            plot_byte(index / width, index % width, byte);
            index--;
        }
    }

    void plot_right_arrow(size_t x, size_t y) {
        
    }
    void plot_up_arrow(size_t x, size_t y) {
        
    }
    void plot_down_arrow(size_t x, size_t y) {
        
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

    static bool load_32_bits_per_pixel_bitmap(const std::string& path, bitmap_header_t& header, std::vector<uint8_t>& pixel_buffer) {
        
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

        for(size_t index = sizeof(bitmap_header_t); index < (bytes_read - sizeof(bitmap_header_t)); index++) {
            pixel_buffer.push_back(buffer[index]);
        }

        return true;
    }
};

#endif