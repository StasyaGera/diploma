#pragma once

#include <string>
#include <vector>
#include <random>

const std::vector<std::vector<int>> calic_predictor = {
	{ 1, 0,  1,  1, 2,  0,  1, 3, 0,  1, 1,  0 }, // a
	{ 1, 0,  6,  1, 2,  2,  1, 3, 1,  1, 1, -1 }, // (3a + c)/4 + (d - b)/8
	{ 1, 0, 10,  1, 2,  6,  1, 3, 3,  1, 1, -3 }, // (5a + 3c)/8 + (3d - 3b)/16
	{ 1, 0,  2,  1, 2,  2,  1, 3, 1,  1, 1, -1 }, // (a + c)/2 + (d - b)/4
	{ 1, 0,  2,  1, 2,  2,  1, 3, 1,  1, 1, -1 }, // (a + c)/2 + (d - b)/4
	{ 1, 0,  6,  1, 2, 10,  1, 3, 3,  1, 1, -3 }, // (3a + 5c)/8 + (3d - 3b)/16
	{ 1, 0,  2,  1, 2,  6,  1, 3, 1,  1, 1, -1 }, // (a + 3c)/4 + (d - b)/8
	{ 1, 0,  0,  1, 2,  1,  1, 3, 0,  1, 1,  0 } // c
};

const std::vector<int> calic_contexts = { 0, 1, 2, 5, 3, 6, 0, 4, 2, 1, 3, 2 }; // |a - b| |c - f| |d - g| |a - e| |c - b| |d - c|

std::string def_filename(std::string algo, std::string img_name, int ctx_population, int pred_population, int generations) {
	return "./log/selected/" + algo + "_" + img_name + "_" 
		+ std::to_string(ctx_population) + "_ctx_" 
		+ std::to_string(pred_population) + "_pred_" 
		+ std::to_string(generations) + "_gen" + ".txt";
}

std::string def_graph_filename(std::string img_name, int ctx_population, int pred_population, int generations) {
	return "./graph2/" + img_name + "_" 
		+ std::to_string(ctx_population) + "_ctx_" 
		+ std::to_string(pred_population) + "_pred_" 
		+ std::to_string(generations) + "_gen" + ".png";
}

