#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <chrono>
#include <functional>

#include "image.h"
#include "prediction.h"
#include "default_constants.h"

const int contexts = 8;
const int chromo_size = 12;

const double pmut = 0.01;
const double pcross = 0.5;

std::random_device rd;
std::mt19937_64 gen(0);
//std::mt19937_64 gen(rd());
std::uniform_real_distribution<double> unif_dist(0.0, 1.0);
double rnd01() { return unif_dist(gen); }

std::vector<double> fitness_fun(511);

typedef std::vector<int> chromosome_t;
typedef std::vector<chromosome_t> context_t;

using namespace std::placeholders;

std::vector<context_t> init(int population_size) {
	std::vector<context_t> population = std::vector<context_t>(contexts, std::vector<chromosome_t>(population_size, chromosome_t(chromo_size)));
	for (int i = 0; i < contexts; i++) {
		for (int j = 0; j < population_size; j++) {
			population[i][j][0] = (int)std::floor(rnd01() * 2.0);
			population[i][j][1] = (int)std::floor(rnd01() * 2.0);
			population[i][j][2] = (int)std::floor(rnd01() * 2.0);
			population[i][j][3] = (int)std::floor(rnd01() * 2.0);

			population[i][j][4] = (int)std::floor(rnd01() * 8.0);
			population[i][j][5] = (int)std::floor(rnd01() * 8.0);
			population[i][j][6] = (int)std::floor(rnd01() * 8.0);
			population[i][j][7] = (int)std::floor(rnd01() * 8.0);

			population[i][j][8] = (int)std::floor(rnd01() * 256.0);
			population[i][j][9] = (int)std::floor(rnd01() * 256.0);
			population[i][j][10] = (int)std::floor(rnd01() * 256.0);
			population[i][j][11] = (int)std::floor(rnd01() * 256.0);
		}
	}
	population[0].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  1, 0, 0, 0 }); // a
	population[1].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  6, 2, 1, -1 }); // (3a + c)/4 + (d - b)/8
	population[2].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  10, 6, 3, -3 }); // (5a + 3c)/8 + (3d - 3b)/16
	population[3].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  2, 2, 1, -1 }); // (a + c)/2 + (d - b)/4
	population[4].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  2, 2, 1, -1 }); // (a + c)/2 + (d - b)/4
	population[5].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  6, 10, 3, -3 }); // (3a + 5c)/8 + (3d - 3b)/16
	population[6].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  2, 6, 1, -1 }); // (a + 3c)/4 + (d - b)/8
	population[7].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  0, 1, 0, 0 }); // c

	return population;
}

std::vector<chromosome_t> init_ctx(int population_size) {
	std::vector<chromosome_t> population = std::vector<chromosome_t>(population_size, chromosome_t(12));
	for (int i = 0; i < population_size; i++) {
		for (int j = 0; j < 6; j++) {
			do {
				population[i][2 * j] = (int)std::floor(rnd01() * 8.0);
				population[i][2 * j + 1] = (int)std::floor(rnd01() * 8.0);
			} while (population[i][2 * j] == population[i][2 * j + 1]);
		}
	}
	return population;
}

context_t evolve(context_t population) {
	context_t children(0);
	for (int i = 0; i < population.size(); i++) {
		if (rnd01() <= pcross) {
			// crossover
			int j;
			do {
				j = (int)std::floor(rnd01() * (double)population.size());
			} while (j == i);
		
			int k1, k2;
			do {
				k1 = (int)std::floor(rnd01() * (double)(chromo_size - 2)) + 1;
				k2 = (int)std::floor(rnd01() * (double)(chromo_size - 2)) + 1;
			} while (k1 == k2);
			int l = std::min(k1, k2);
			int r = std::max(k1, k2);

			chromosome_t child1 = chromosome_t(chromo_size);
			std::copy(population[i].begin(), population[i].begin() + l, child1.begin());
			std::copy(population[j].begin() + l, population[j].begin() + r, child1.begin() + l);
			std::copy(population[i].begin() + r, population[i].end(), child1.begin() + r);
			children.push_back(child1);

			chromosome_t child2 = chromosome_t(chromo_size);
			std::copy(population[j].begin(), population[j].begin() + l, child2.begin());
			std::copy(population[i].begin() + l, population[i].begin() + r, child2.begin() + l);
			std::copy(population[j].begin() + r, population[j].end(), child2.begin() + r);
			children.push_back(child2);
		}

		if (rnd01() <= pmut) {
			// mutate
			int j = (int)std::floor(rnd01() * (double)chromo_size);

			chromosome_t child = chromosome_t(population[i]);
			if (j < 4) {
				if (population[i][j] == 1)
					child[j] = 0;
				else
					child[j] = 1;
			}
			else if (j < 8) {
				int new_val;
				do {
					new_val = (int)std::floor(rnd01() * 8.0);
				} while (new_val == population[i][j]);
				child[j] = new_val;
			}
			else {
				int d;
				bool in_range;
				do {
					d = (int)std::floor((rnd01() - rnd01()) * 500.0);
					in_range = (d != 0 && population[i][j] + d >= -255 && population[i][j] + d <= 255);
				} while (!in_range);
				child[j] += d;
			}
			children.push_back(child);
		}
	}
	for (int i = 0; i < children.size(); i++) {
		population.push_back(children[i]);
	}
	return population;
}

std::vector<chromosome_t> evolve_ctx(std::vector<chromosome_t> population) {
	std::vector<chromosome_t> children(0);
	for (int i = 0; i < population.size(); i++) {
		if (rnd01() <= pcross) {
			// crossover
			int j;
			do {
				j = (int)std::floor(rnd01() * (double)population.size());
			} while (j == i);
		
			int r = 2 * ((int)std::floor(rnd01() * 5.0) + 1);
			
			chromosome_t child1 = chromosome_t(12);
			std::copy(population[i].begin(), population[i].begin() + r, child1.begin());
			std::copy(population[j].begin() + r, population[j].end(), child1.begin() + r);
			children.push_back(child1);

			chromosome_t child2 = chromosome_t(12);
			std::copy(population[j].begin(), population[j].begin() + r, child2.begin());
			std::copy(population[i].begin() + r, population[i].end(), child2.begin() + r);
			children.push_back(child2);
		}

		if (rnd01() <= pmut) {
			// mutate
			int j = (int)std::floor(rnd01() * 12.0);
			chromosome_t child = chromosome_t(population[i]);
			int new_val;
			do {
				new_val = (int)std::floor(rnd01() * 8.0);
			} while (new_val == population[i][j] || new_val == population[i][(j % 2 == 0 ? j + 1 : j - 1)]);
			child[j] = new_val;
			children.push_back(child);
		}
	}
	for (int i = 0; i < children.size(); i++) {
		population.push_back(children[i]);
	}
	return population;
}

inline void init_fitness() {
	for (int i = 0; i < 511; i++) {
		fitness_fun[i] = 100000.0 / (double)(i * i + 400);
	}
}

double sayood_ga1(const image& img, int population_size = 20, int generations_cnt = 30) {
	init_fitness();

	std::vector<std::pair<chromosome_t, double>> best_vals(contexts, { chromosome_t(), 0.0 });
	auto full_population = init(population_size);
	/*chromosome_t context_const(12);
	for (int i = 0; i < 6; i++) {
		do {
			context_const[2 * i] = (int)std::floor(rnd01() * 8.0);
			context_const[2 * i + 1] = (int)std::floor(rnd01() * 8.0);
		} while (context_const[2 * i] == context_const[2 * i + 1]);
	}*/

	for (int g = 0; g < generations_cnt; g++) {
		for (int x = 2; x < img.width - 1; x++) {
			for (int y = 2; y < img.height; y++) {
				for (int col = 0; col < img.colors; col++) {
					auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);

					short a = img(x - 1, y, col);
					short b = img(x - 1, y - 1, col);
					short c = img(x, y - 1, col);
					short d = img(x + 1, y - 1, col);
					short e = img(x - 2, y, col);
					short f = img(x, y - 2, col);
					short g = img(x + 1, y - 2, col);
					short h = img(x - 2, y - 1, col);

					short dh = std::abs(a - e) + std::abs(c - b) + std::abs(d - c);
					//short dh = std::abs(get_pix_val(context_const[0]) - get_pix_val(context_const[1])) + 
					//	std::abs(get_pix_val(context_const[2]) - get_pix_val(context_const[3])) + 
					//	std::abs(get_pix_val(context_const[4]) - get_pix_val(context_const[5]));
					short dv = std::abs(a - b) + std::abs(c - f) + std::abs(d - g);
					//short dv = std::abs(get_pix_val(context_const[6]) - get_pix_val(context_const[7])) +
					//	std::abs(get_pix_val(context_const[8]) - get_pix_val(context_const[9])) +
					//	std::abs(get_pix_val(context_const[10]) - get_pix_val(context_const[11]));
					short D = dv - dh;

					int k = get_ctx_no(D);

					full_population[k] = evolve(full_population[k]);

					// count fitness for each individual
					std::vector<std::pair<double, int>> fit(0);
					double sum_fit = 0.0;
					for (int i = 0; i < full_population[k].size(); i++) {
						int res1 = 0, res2 = 0;
						int prediction;
						for (size_t j = 0; j < 4; j++) {
							unsigned char pix = int_to_pix(img, x, y, col, full_population[k][i][j + 4]);
							res1 += full_population[k][i][j] * pix * full_population[k][i][j + 8];
							res2 += full_population[k][i][j] * full_population[k][i][j + 8];
						}
						unsigned char pred = res2 == 0 ? 0 : res1 / res2;
						double f = fitness_fun[std::abs(img(x, y, col) - pred)];
						if (f > best_vals[k].second) {
							best_vals[k].first = full_population[k][i];
							best_vals[k].second = f;
						}
						fit.push_back({ f, i });
						sum_fit += f;
					}

					// selection
					double prev = 0.0;
					std::sort(fit.begin(), fit.end());
					for (int i = 0; i < fit.size(); i++) {
						prev += (fit[i].first / sum_fit);
						fit[i].first = prev;
					}

					context_t selected(0);
					for (int i = 0; i < population_size; i++) {
						double rand = rnd01();
						int j = 0;
						while (rand >= fit[j].first) {
							j++;
						}
						selected.push_back(full_population[k][fit[j].second]);
					}
					full_population[k] = selected;
				}
			}
		}
	}

	std::vector<std::vector<int>> coeffs(contexts);
	for (int i = 0; i < contexts; i++) {
		coeffs[i] = best_vals[i].first;
	}

	return get_err_entropy<std::vector<std::vector<int>>>(img, sayood, coeffs);
}

std::vector<std::vector<int>> tune_pred(const image& img, int population_size = 20, int generations_cnt = 30, chromosome_t ctx_vals = { 0, 1, 2, 5, 3, 6, 0, 4, 2, 1, 3, 2 }) {
	std::vector<std::pair<chromosome_t, double>> best_vals(contexts, { chromosome_t(), 0.0 });
	auto full_population = init(population_size);
	full_population[0].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  1, 0, 0, 0 }); // a
	full_population[1].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  6, 2, 1, -1 }); // (3a + c)/4 + (d - b)/8
	full_population[2].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  10, 6, 3, -3 }); // (5a + 3c)/8 + (3d - 3b)/16
	full_population[3].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  2, 2, 1, -1 }); // (a + c)/2 + (d - b)/4
	full_population[4].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  2, 2, 1, -1 }); // (a + c)/2 + (d - b)/4
	full_population[5].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  6, 10, 3, -3 }); // (3a + 5c)/8 + (3d - 3b)/16
	full_population[6].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  2, 6, 1, -1 }); // (a + 3c)/4 + (d - b)/8
	full_population[7].push_back({ 1, 1, 1, 1,  0, 2, 3, 1,  0, 1, 0, 0 }); // c

	for (int g = 0; g < generations_cnt; g++) {
		// evolve current population
		for (int k = 0; k < contexts; k++) {
			full_population[k] = evolve(full_population[k]);
		}

		// prepare to count fitness for each individual
		std::vector<std::vector<std::pair<double, int>>> fitness(contexts);
		for (int k = 0; k < contexts; k++) {
			for (int i = 0; i < full_population[k].size(); i++) {
				fitness[k].push_back({ 0.0, i });
			}
		}
		std::vector<double> sum_fitness(contexts, 0.0);

		// exec full image with current population
		for (int x = 2; x < img.width - 1; x++) {
			for (int y = 2; y < img.height; y++) {
				for (int col = 0; col < img.colors; col++) {
					auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
					short dv = std::abs(get_pix_val(ctx_vals[0]) - get_pix_val(ctx_vals[1])) +
						std::abs(get_pix_val(ctx_vals[2]) - get_pix_val(ctx_vals[3])) +
						std::abs(get_pix_val(ctx_vals[4]) - get_pix_val(ctx_vals[5]));
					short dh = std::abs(get_pix_val(ctx_vals[6]) - get_pix_val(ctx_vals[7])) +
						std::abs(get_pix_val(ctx_vals[8]) - get_pix_val(ctx_vals[9])) +
						std::abs(get_pix_val(ctx_vals[10]) - get_pix_val(ctx_vals[11]));
					short D = dv - dh;

					int k = get_ctx_no(D);

					for (int i = 0; i < full_population[k].size(); i++) {
						// for each individual in current population for context k
						long long res1 = 0, res2 = 0;
						for (size_t j = 0; j < 4; j++) {
							unsigned char pix = int_to_pix(img, x, y, col, full_population[k][i][j + 4]);
							res1 += full_population[k][i][j] * pix * full_population[k][i][j + 8];
							res2 += full_population[k][i][j] * full_population[k][i][j + 8];
						}
						unsigned char pred = res2 == 0 ? 0 : res1 / res2;
						double f = fitness_fun[std::abs(img(x, y, col) - pred)];
						
						fitness[k][i].first += f;
						sum_fitness[k] += f;
					}
				}
			}
		}

		// selection
		for (int k = 0; k < contexts; k++) {
			std::sort(fitness[k].begin(), fitness[k].end());
			std::reverse(fitness[k].begin(), fitness[k].end());
			context_t selected(0);
			for (int i = 0; i < population_size; i++) {
				selected.push_back(full_population[k][fitness[k][i].second]);
				if (fitness[k][i].first > best_vals[k].second) {
					best_vals[k].first = full_population[k][fitness[k][i].second];
					best_vals[k].second = fitness[k][i].first;
				}
			}
			full_population[k] = selected;
		}

		// roulette wheel
		//std::vector<std::vector<double>> wheel(contexts, std::vector<double>(0));
		//for (int k = 0; k < contexts; k++) {
		//	wheel[k].resize(fitness[k].size());
		//	double prev = 0.0;
		//	std::sort(fitness[k].begin(), fitness[k].end());
		//	for (int i = 0; i < fitness[k].size(); i++) {
		//		prev += fitness[k][i].first / sum_fitness[k];
		//		wheel[k][i] = prev;
		//	}

		//	context_t selected(0);
		//	for (int i = 0; i < population_size; i++) {
		//		double rand = rnd01();
		//		int j = 0;
		//		while (rand >= wheel[k][j]) {
		//			j++;
		//		}
		//		selected.push_back(full_population[k][fitness[k][j].second]);
		//		if (fitness[k][j].first > best_vals[k].second) {
		//			best_vals[k].first = full_population[k][fitness[k][j].second];
		//			best_vals[k].second = fitness[k][j].first;
		//		}
		//	}
		//	full_population[k] = selected;
		//}
	}

	std::vector<std::vector<int>> coeffs(contexts);
	for (int i = 0; i < contexts; i++) {
		coeffs[i] = best_vals[i].first;
	}

	return coeffs;
}

double gen_in_gen(const image& img, int ctx_pop_size = 10, int generations_cnt = 10, int pred_pop_size = 5) {
	init_fitness();

	auto ctx_vals = init_ctx(ctx_pop_size);
	ctx_vals.push_back({ 0, 1, 2, 5, 3, 6, 0, 4, 2, 1, 3, 2 });
	std::pair<chromosome_t, double> best_ctx_vals({ chromosome_t(), 1000.0 });

	std::vector<std::vector<context_t>> predictors(ctx_vals.size());
	for (int i = 0; i < predictors.size(); i++) {
		predictors[i] = init(pred_pop_size);
	}

	for (int g = 0; g < generations_cnt; g++) {
		std::cout << "wow + " << g << '\n';
		// evolve current population
		int old_size = ctx_vals.size();
		ctx_vals = evolve_ctx(ctx_vals);
		int new_size = ctx_vals.size();
		for (int x = 0; x < (new_size - old_size); x++) {
			predictors.push_back(init(pred_pop_size));
		}

		// prepare to count fitness for each individual
		std::vector<std::pair<double, int>> ctx_fitness(0);
		for (int i = 0; i < ctx_vals.size(); i++) {
			std::vector<std::pair<chromosome_t, double>> best_pred_vals(contexts, { chromosome_t(), 0.0 });

			// evolve current population
			for (int k = 0; k < contexts; k++) {
				predictors[i][k] = evolve(predictors[i][k]);
			}
			// prepare to count fitness for each individual
			std::vector<std::vector<std::pair<double, int>>> pred_fitness(contexts);
			for (int k = 0; k < contexts; k++) {
				for (int j = 0; j < predictors[i][k].size(); j++) {
					pred_fitness[k].push_back({ 0.0, j });
				}
			}
			std::vector<double> sum_pred_fitness(contexts, 0.0);

			// exec full image with current population
			for (int x = 2; x < img.width - 1; x++) {
				for (int y = 2; y < img.height; y++) {
					for (int col = 0; col < img.colors; col++) {
						auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
						short dv = std::abs(get_pix_val(ctx_vals[i][0]) - get_pix_val(ctx_vals[i][1])) +
							std::abs(get_pix_val(ctx_vals[i][2]) - get_pix_val(ctx_vals[i][3])) +
							std::abs(get_pix_val(ctx_vals[i][4]) - get_pix_val(ctx_vals[i][5]));
						short dh = std::abs(get_pix_val(ctx_vals[i][6]) - get_pix_val(ctx_vals[i][7])) +
							std::abs(get_pix_val(ctx_vals[i][8]) - get_pix_val(ctx_vals[i][9])) +
							std::abs(get_pix_val(ctx_vals[i][10]) - get_pix_val(ctx_vals[i][11]));
						short D = dv - dh;

						int k = get_ctx_no(D);

						for (int j = 0; j < predictors[i][k].size(); j++) {
							// for each individual in current population for context k
							long long res1 = 0, res2 = 0;
							for (size_t r = 0; r < 4; r++) {
								unsigned char pix = int_to_pix(img, x, y, col, predictors[i][k][j][r + 4]);
								res1 += predictors[i][k][j][r] * pix * predictors[i][k][j][r + 8];
								res2 += predictors[i][k][j][r] * predictors[i][k][j][r + 8];
							}
							unsigned char pred = res2 == 0 ? 0 : res1 / res2;
							double f = fitness_fun[std::abs(img(x, y, col) - pred)];

							pred_fitness[k][j].first += f;
							sum_pred_fitness[k] += f;
						}
					}
				}
			}

			// selection
			for (int k = 0; k < contexts; k++) {
				std::sort(pred_fitness[k].begin(), pred_fitness[k].end());
				std::reverse(pred_fitness[k].begin(), pred_fitness[k].end());
				context_t selected(0);
				for (int j = 0; j < pred_pop_size; j++) {
					selected.push_back(predictors[i][k][pred_fitness[k][j].second]);
					if (pred_fitness[k][j].first > best_pred_vals[k].second) {
						best_pred_vals[k].first = predictors[i][k][pred_fitness[k][j].second];
						best_pred_vals[k].second = pred_fitness[k][j].first;
					}
				}
				predictors[i][k] = selected;
			}

			std::vector<std::vector<int>> coeffs(contexts);
			for (int k = 0; k < contexts; k++) {
				coeffs[k] = best_pred_vals[k].first;
			}

			//std::cout << "tuna!\n";
			double entr = get_err_entropy(img, std::bind(sayood2, _1, _2, _3, _4, ctx_vals[i], coeffs));
			ctx_fitness.push_back({ entr, i });
		}

		// selection
		std::sort(ctx_fitness.begin(), ctx_fitness.end());
		context_t selected(0);
		std::vector<std::vector<context_t>> selected_pred(0);
		for (int i = 0; i < ctx_pop_size; i++) {
			selected.push_back(ctx_vals[ctx_fitness[i].second]);
			selected_pred.push_back(predictors[ctx_fitness[i].second]);
			if (ctx_fitness[i].first < best_ctx_vals.second) {
				best_ctx_vals.first = ctx_vals[ctx_fitness[i].second];
				best_ctx_vals.second = ctx_fitness[i].first;
			}
		}
		ctx_vals = selected;
		predictors = selected_pred;
	}

	std::cout << "DONE\n";
	context_t best_coeffs = tune_pred(img, 20, 30, best_ctx_vals.first);
	return get_err_entropy(img, std::bind(sayood2, _1, _2, _3, _4, best_ctx_vals.first, best_coeffs));
}
	
double basic_sayood_init_calic(const image& img, int population_size = 20, int generations_cnt = 30) {
	init_fitness();
	auto coeffs = tune_pred(img, population_size, generations_cnt);
	return get_err_entropy(img, std::bind(sayood, _1, _2, _3, _4, coeffs));
}

double sayood_ga(const image& img, int population_size = 20, int generations_cnt = 100) {
	for (int i = 0; i < 511; i++) {
		fitness_fun[i] = 100000.0 / (double)(i * i + 400);
	}

	std::pair<chromosome_t, double> best_ctx_vals({ chromosome_t(), 0.0 });
	context_t best_coeffs;
	auto ctx_vals = init_ctx(population_size);
	for (int g = 0; g < generations_cnt; g++) {
		// evolve current population
		ctx_vals = evolve_ctx(ctx_vals);
	
		// prepare to count fitness for each individual
		std::vector<std::pair<double, int>> fitness(0);
		std::vector<context_t> coeffs(ctx_vals.size());
		for (int i = 0; i < ctx_vals.size(); i++) {
			fitness.push_back({ 0.0, i });
			coeffs[i] = tune_pred(img, population_size, generations_cnt, ctx_vals[i]);
		}
		double sum_fitness(0.0);

		// exec full image with current population
		for (int x = 2; x < img.width - 1; x++) {
			for (int y = 2; y < img.height; y++) {
				for (int col = 0; col < img.colors; col++) {
					auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
					
					for (int i = 0; i < ctx_vals.size(); i++) {
						short dh = std::abs(get_pix_val(ctx_vals[i][0]) - get_pix_val(ctx_vals[i][1])) +
							std::abs(get_pix_val(ctx_vals[i][2]) - get_pix_val(ctx_vals[i][3])) +
							std::abs(get_pix_val(ctx_vals[i][4]) - get_pix_val(ctx_vals[i][5]));
						short dv = std::abs(get_pix_val(ctx_vals[i][6]) - get_pix_val(ctx_vals[i][7])) +
							std::abs(get_pix_val(ctx_vals[i][8]) - get_pix_val(ctx_vals[i][9])) +
							std::abs(get_pix_val(ctx_vals[i][10]) - get_pix_val(ctx_vals[i][11]));
						short D = dh - dv;

						int k = get_ctx_no(D);

						// for each individual in current population for context k
						int res1 = 0, res2 = 0;
						for (size_t j = 0; j < 4; j++) {
							unsigned char pix = int_to_pix(img, x, y, col, coeffs[i][k][j + 4]);
							res1 += coeffs[i][k][j] * pix * coeffs[i][k][j + 8];
							res2 += coeffs[i][k][j] * coeffs[i][k][j + 8];
						}
						unsigned char pred = res2 == 0 ? 0 : res1 / res2;
						double f = fitness_fun[std::abs(img(x, y, col) - pred)];
						fitness[i].first += f;
						sum_fitness += f;
					}
				}
			}
		}

		// selection
		std::vector<double> wheel(0);
		wheel.resize(fitness.size());
		double prev = 0.0;
		std::sort(fitness.begin(), fitness.end());
		for (int i = 0; i < fitness.size(); i++) {
			prev += fitness[i].first / sum_fitness;
			wheel[i] = prev;
		}

		context_t selected(0);
		for (int i = 0; i < population_size; i++) {
			double rand = rnd01();
			int j = 0;
			while (rand >= wheel[j]) {
				j++;
			}
			selected.push_back(ctx_vals[fitness[j].second]);
			if (fitness[j].first > best_ctx_vals.second) {
				best_ctx_vals.first = ctx_vals[fitness[j].second];
				best_ctx_vals.second = fitness[j].first;
				best_coeffs = coeffs[fitness[j].second];
			}
		}
		ctx_vals = selected;
	}

	return get_err_entropy(img, std::bind(sayood2, _1, _2, _3, _4, best_ctx_vals.first, best_coeffs));
}
