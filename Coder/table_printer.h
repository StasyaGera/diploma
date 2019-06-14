#pragma once

#include <iostream>
#include <string>
#include <iomanip>

const int img_name_width = 20;
const int entropy_width = 10;
const std::string sep = " |";

size_t predictors_count;
size_t total_width;
std::string line;

void print_header(std::initializer_list<std::string> predictors) {
	predictors_count = predictors.size();
	total_width = img_name_width + entropy_width * predictors_count + sep.size() * (predictors_count + 1);
	line = sep + std::string(total_width - 1, '-') + '|';

	std::cout << line << '\n' << sep << std::setw(img_name_width) << "image" << sep;
	for (std::string predictor : predictors) {
		std::cout << std::setw(entropy_width) << predictor << sep;
	}
	std::cout << '\n' << line << std::endl;
}

void print_row(std::filesystem::path filename, std::initializer_list<double> values) {
	std::cout << sep << std::setw(img_name_width) << filename << sep;
	std::cout << std::fixed << std::setprecision(3);
	for (double entropy : values) {
		std::cout << std::setw(entropy_width) << entropy << sep;
	}
	std::cout << std::endl;
}

void print_bottom_line() {
    std::cout << line << std::endl;
}
