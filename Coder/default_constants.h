#pragma once
#include <string>

const long long seed = 100;

const double elite_percentage = 0.2;
const int def_population_sz = 250;
const int def_generations = 1000;

std::string def_verbose_log_filename(std::string img_name, int population, bool init_jpeg) {
	return "./log/v_logdata_" + img_name + "_" + std::to_string(population) + (init_jpeg ? "jpeg_" : "") + ".txt";
}

std::string def_err_gen_by_gen_filename(std::string img_name, int population, bool init_jpeg) {
	return "./log/logdata_" + img_name + "_" + std::to_string(population) + (init_jpeg ? "jpeg_" : "") + ".txt";
}
