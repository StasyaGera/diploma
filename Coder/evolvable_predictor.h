#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <functional>
#include <fstream>
#include <iostream>

#include "default_constants.h"
#include "image.h"
#include "prediction.h"

using namespace std::placeholders;

typedef std::vector<int> v_int;
typedef std::vector<v_int> vv_int;
typedef std::vector<vv_int> vvv_int;

std::random_device rd;
//std::mt19937_64 gen(0);
std::mt19937_64 gen(rd());
std::uniform_real_distribution<double> unif_dist(0.0, 1.0);
double rnd01() { return unif_dist(gen); }

const double pcross = 0.5;
const double pmut = 0.01;

const int contexts = 8;
const int pred_ind_size = 12;

std::vector<double> fitness_fun(511);

void init_fitness_fun() {
	for (size_t i = 0; i < 511; i++) {
		fitness_fun[i] = 100000.0 / (double)(i * i + 400);
	}
}

vvv_int init_predictors(int population_size) {
	vvv_int population = vvv_int(contexts, vv_int(population_size, v_int(pred_ind_size)));
	for (int k = 0; k < contexts; k++) {
		for (int i = 0; i < population_size - 1; i++) {
			for (int j = 0; j < pred_ind_size; j += 3) {
				population[k][i][j] = (int)std::floor(rnd01() * 2.0);
				population[k][i][j + 1] = (int)std::floor(rnd01() * 8.0);
				population[k][i][j + 2] = (int)std::floor(rnd01() * 256.0);
			}
		}
		population[k].back() = calic_predictor[k];
	}
	return population;
}

void evolve_predictors(vvv_int& population) {
	for (int k = 0; k < contexts; k++) {
		int population_size = population[k].size();
		for (int i = 0; i < population_size; i++) {
			// crossover
			if (rnd01() < pcross) {
				int j;
				do {
					j = (int)std::floor(rnd01() * (double)population_size);
				} while (j == i);

				int p1, p2;
				do {
					p1 = (int)std::floor(rnd01() * (double)(pred_ind_size - 1)) + 1;
					p2 = (int)std::floor(rnd01() * (double)(pred_ind_size - 1)) + 1;
				} while (p1 == p2);
				int l = std::min(p1, p2);
				int r = std::max(p1, p2);

				v_int child1(pred_ind_size);
				std::copy(population[k][i].begin(), population[k][i].begin() + l, child1.begin());
				std::copy(population[k][j].begin() + l, population[k][j].begin() + r, child1.begin() + l);
				std::copy(population[k][i].begin() + r, population[k][i].end(), child1.begin() + r);
				population[k].push_back(child1);

				v_int child2(pred_ind_size);
				std::copy(population[k][j].begin(), population[k][j].begin() + l, child2.begin());
				std::copy(population[k][i].begin() + l, population[k][i].begin() + r, child2.begin() + l);
				std::copy(population[k][j].begin() + r, population[k][j].end(), child2.begin() + r);
				population[k].push_back(child2);
			}
			// mutation
			if (rnd01() < pmut) {
				v_int child(population[k][i]);
				int p = (int)std::floor(rnd01() * (double)pred_ind_size);
				if (p % 3 == 0) {
					child[p] = population[k][i][p] == 1 ? 0 : 1;
				}
				else if (p % 3 == 1) {
					int new_val;
					do {
						new_val = (int)std::floor(rnd01() * 8.0);
					} while (new_val == population[k][i][p]);
					child[p] = new_val;
				}
				else {
					int d;
					bool in_range;
					do {
						d = (int)std::floor((rnd01() - rnd01()) * 500.0);
						in_range = (d != 0 && population[k][i][p] + d >= -255 && population[k][i][p] + d <= 255);
					} while (!in_range);
					child[p] += d;
				}
				population[k].push_back(child);
			}
		}
	}
}

std::vector<std::pair<double, v_int>> select_predictors(const image& img, vvv_int& population, int population_size, int elite_size,
	std::function<int(const image&, size_t, size_t, int, const std::vector<int>&)> get_ctx = get_ctx_calic, const std::vector<int>& context_values = calic_contexts) {
	std::vector<std::pair<double, v_int>> best_predictors(contexts, { 0.0, v_int(pred_ind_size) });

	std::vector<double> sum_fitness(contexts, 0.0);
	std::vector<std::vector<std::pair<double, int>>> fitness(contexts);
	for (int k = 0; k < contexts; k++) {
		fitness[k] = std::vector<std::pair<double, int>>(population[k].size());
		for (int i = 0; i < population[k].size(); i++) {
			fitness[k][i] = { 0.0, i };
		}
	}

	// iterate the image and count mean fitness
	for (int x = 2; x < img.width - 1; x++) {
		for (int y = 2; y < img.height; y++) {
			for (int col = 0; col < img.colors; col++) {
				int k = get_ctx(img, x, y, col, context_values);
				for (int i = 0; i < population[k].size(); i++) {
					int res1 = 0, res2 = 0;
					for (size_t j = 0; j < pred_ind_size; j += 3) {
						int pix = int_to_pix(img, x, y, col, population[k][i][j + 1]);
						res1 += population[k][i][j] * pix * population[k][i][j + 2];
						res2 += population[k][i][j] * population[k][i][j + 2];
					}
					unsigned char prediction = res2 == 0 ? 0 : res1 / res2;
					double f = fitness_fun[std::abs((int)img(x, y, col) - (int)prediction)];
					fitness[k][i].first += f;
					sum_fitness[k] += f;
				}
			}
		}
	}

	for (int k = 0; k < contexts; k++) {
		std::sort(fitness[k].begin(), fitness[k].end());
		double best_fitness = fitness[k].back().first;
		if (best_fitness > best_predictors[k].first) {
			best_predictors[k].first = best_fitness;
			best_predictors[k].second = population[k][fitness[k].back().second];
		}

		vv_int selected(population_size);
		// elite selection
		for (int i = 0; i < elite_size; i++) {
			int j = fitness[k].size() - i - 1;
			sum_fitness[k] -= fitness[k][j].first;
			selected[i] = population[k][fitness[k][j].second];
		}

		// roulette wheel selection
		std::vector<double> wheel(fitness[k].size() - elite_size);
		double prev = 0.0;
		for (int i = 0; i < fitness[k].size() - elite_size; i++) {
			prev += fitness[k][i].first / sum_fitness[k];
			wheel[i] = prev;
		}
		for (int i = elite_size; i < population_size; i++) {
			double rand = rnd01();
			int j = 0;
			while (rand > wheel[j]) {
				j++;
			}
			selected[i] = population[k][fitness[k][j].second];
		}
		population[k] = selected;
	}

	return best_predictors;
}

double evolvable_predictor(const image& img, std::string img_name, int population_size = 20, int elite_size = 5, int generations = 10) {
	std::ofstream logfile(def_filename("ep", img_name, 0, population_size, generations));
	init_fitness_fun();

	vvv_int population = init_predictors(population_size);
	// store best individual and his fitness for each context
	std::vector<std::pair<double, v_int>> best_individuals(contexts, { 0.0, v_int(pred_ind_size) });
	for (int g = 0; g < generations; g++) {
		evolve_predictors(population);
		best_individuals = select_predictors(img, population, population_size, elite_size);

		vv_int coeffs(contexts, v_int(pred_ind_size));
		for (int k = 0; k < contexts; k++) {
			coeffs[k] = best_individuals[k].second;
		}
		logfile << g + 1 << ' ' << get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, coeffs, calic_contexts, get_ctx_calic)) << '\n';
	}
	logfile.close();

	vv_int coeffs(contexts, v_int(pred_ind_size));
	for (int k = 0; k < contexts; k++) {
		coeffs[k] = best_individuals[k].second;
	}
	return get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, coeffs, calic_contexts, get_ctx_calic));
}
