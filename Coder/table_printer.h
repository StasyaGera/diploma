#pragma once

#include <iostream>
#include <string>
#include <iomanip>

const int img_name_width = 18;
const int entropy_width = 12;
//const int predictors = 5;
const int predictors = 4;
const std::string sep = " |";
int total_width = img_name_width + entropy_width * predictors + sep.size() * (predictors + 1);
std::string line = sep + std::string(total_width - 1, '-') + '|';

void print_header() {    
    std::cout << line << '\n' << sep
        << std::setw(img_name_width) << "image" << sep
        << std::setw(entropy_width) << "entropy" << sep
        << std::setw(entropy_width) << "prev" << sep
        << std::setw(entropy_width) << "jpeg-ls" << sep
        //<< std::setw(entropy_width) << "calic" << sep
        << std::setw(entropy_width) << "genetic" << sep << '\n' << line << '\n';
}

void print_row(std::filesystem::path filename, double img_entropy, double prev_entropy, double jpeg_ls_entropy/*, double calic_entropy*/, double genetic_entropy) {
    std::cout << sep
        << std::setw(img_name_width) << filename << sep
        << std::fixed << std::setprecision(3)
        << std::setw(entropy_width) << img_entropy << sep
        << std::setw(entropy_width) << prev_entropy << sep
        << std::setw(entropy_width) << jpeg_ls_entropy << sep
        //<< std::setw(entropy_width) << calic_entropy << sep
        << std::setw(entropy_width) << genetic_entropy << sep << '\n';
}

void print_bottom_line() {
    std::cout << line << std::endl;
}
