#pragma once

struct image {
    image() {}

    image(const char * filename, int colors) :
        colors(colors)
    {
        parse(filename);
    }
    
    image(const image & other) :
        full_data_size(other.full_data_size),
        data_size(other.data_size),
        height(other.height),
        width(other.width),
        colors(other.colors),
        bytes_per_pixel(other.bytes_per_pixel),
        bytes_per_row(other.bytes_per_row)
    {
        full_data = (unsigned char *)malloc(full_data_size);
        size_t header_size = other.data - other.full_data;
        std::copy(other.full_data, other.full_data + other.full_data_size, full_data);
        data = full_data + (header_size);
    }

    ~image() 
    {
        if (full_data != nullptr)
            free(full_data);
    }

    void write_to_file(const char * filename) {
        FILE* output = fopen(filename, "wb");
        fwrite(full_data, 1, full_data_size, output);
        fclose(output);
    }

    unsigned char operator()(size_t x, size_t y, int col) const {
        return data[y * bytes_per_row + x * bytes_per_pixel + col];
    }

    void set(size_t x, size_t y, int col, unsigned char val) {
        data[y * bytes_per_row + x * bytes_per_pixel + col] = val;
    }

    unsigned char* data;
    size_t data_size;
    size_t width, height;
    int colors;
    
private:
    unsigned char* full_data;
    size_t full_data_size;
    int bytes_per_pixel;
    size_t bytes_per_row;

    int read_bytes(unsigned char* buffer, int n, size_t i) {
        int result = 0;
        for (int j = 0; j < n; j++) {
            result += buffer[i + j] * (1 << 8 * j);
        }
        return result;
    }

    void load_file(const char* filename) {
        FILE* input = fopen(filename, "rb+");
        fseek(input, 0, SEEK_END);
        full_data_size = ftell(input);
        rewind(input);
        full_data = (unsigned char *)malloc(full_data_size);
        if (fread(full_data, 1, full_data_size, input) != full_data_size) {
            printf("Could not read file '%s'", filename);
            free(full_data);
        }
        fclose(input);
    }

    void parse_bmp(const char* filename) {
        load_file(filename);

        data = full_data + read_bytes(full_data, 4, 10);
        width = read_bytes(full_data, 4, 18);
        height = read_bytes(full_data, 4, 22);
        int bits_per_pixel = read_bytes(full_data, 2, 28);
        bytes_per_pixel = bits_per_pixel / 8;
        bytes_per_row = (size_t)std::ceil((double)bits_per_pixel * width / 32) * 4;
        data_size = read_bytes(full_data, 4, 34);
    }

    void parse(const char* filename) {
        std::string fn_str = std::string(filename);
        std::string extention = fn_str.substr(fn_str.find_last_of(".") + 1);
        if (extention != "bmp") {
            printf("Only 24bpp bmp files supported");
            return;
        }
        parse_bmp(filename);
    }
};
