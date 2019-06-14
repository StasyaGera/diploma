#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <chrono>
#include <functional>

#include "image.h"
#include "prediction.h"

//typedef std::vector<int> v_int;
//typedef std::vector<v_int> vv_int;
//typedef std::vector<vv_int> vvv_int;

using namespace std::placeholders;

//std::random_device rd;
//std::mt19937_64 gen(0);
////std::mt19937_64 gen(rd());
//std::uniform_real_distribution<double> unif_dist(0.0, 1.0);
//double rnd01() { return unif_dist(gen); }

const int contexts = 8;
const int pred_ind_size = 12;
const int cont_ind_size = 12;
const int tree_ind_size = 21;

const double pmut = 0.01;
const double pcross = 0.5;

std::vector<double> fitness_fun(511);

const vv_int calic_predictor = {
	{ 1, 0,  1,  1, 2,  0,  1, 3, 0,  1, 1,  0 }, // a
	{ 1, 0,  6,  1, 2,  2,  1, 3, 1,  1, 1, -1 }, // (3a + c)/4 + (d - b)/8
	{ 1, 0, 10,  1, 2,  6,  1, 3, 3,  1, 1, -3 }, // (5a + 3c)/8 + (3d - 3b)/16
	{ 1, 0,  2,  1, 2,  2,  1, 3, 1,  1, 1, -1 }, // (a + c)/2 + (d - b)/4
	{ 1, 0,  2,  1, 2,  2,  1, 3, 1,  1, 1, -1 }, // (a + c)/2 + (d - b)/4
	{ 1, 0,  6,  1, 2, 10,  1, 3, 3,  1, 1, -3 }, // (3a + 5c)/8 + (3d - 3b)/16
	{ 1, 0,  2,  1, 2,  6,  1, 3, 1,  1, 1, -1 }, // (a + 3c)/4 + (d - b)/8
	{ 1, 0,  0,  1, 2,  1,  1, 3, 0,  1, 1,  0 } // c
};
const v_int calic_contexts = { 0, 1, 2, 5, 3, 6, 0, 4, 2, 1, 3, 2 }; // |a - b| |c - f| |d - g| |a - e| |c - b| |d - c|

vvv_int init_predictors(int population_size) {
	vvv_int population = vvv_int(contexts, vv_int(population_size, v_int(pred_ind_size)));
	for (int k = 0; k < contexts; k++) {
		for (int i = 0; i < population_size - 1; i++) {
			for (int j = 0; j < pred_ind_size; j += 3) {
				population[k][i][j]     = (int)std::floor(rnd01() * 2.0);
				population[k][i][j + 1] = (int)std::floor(rnd01() * 8.0);
				population[k][i][j + 2] = (int)std::floor(rnd01() * 256.0);
			}
		}
		population[k].back() = calic_predictor[k];
	}
	return population;
}

vv_int init_tree_contexts(int population_size) {
	std::vector<v_int> population = std::vector<v_int>(population_size, v_int(tree_ind_size));
	for (int i = 0; i < population_size; i++) {
		for (int j = 0; j < tree_ind_size; j += 3) {
			do {
				population[i][j]     = (int)std::floor(rnd01() * 8.0);
				population[i][j + 1] = (int)std::floor(rnd01() * 8.0);
			} while (population[i][j] == population[i][j + 1]);
			population[i][j + 2] = (int)std::floor(rnd01() * 256.0);
		}
	}
	return population;
}

std::vector<v_int> init_contexts(int population_size) {
	std::vector<v_int> population = std::vector<v_int>(population_size, v_int(cont_ind_size));
	for (int i = 0; i < population_size - 1; i++) {
		for (int j = 0; j < cont_ind_size; j += 2) {
			do {
				population[i][j]     = (int)std::floor(rnd01() * 8.0);
				population[i][j + 1] = (int)std::floor(rnd01() * 8.0);
			} while (population[i][j] == population[i][j + 1]);
		}
	}
	population.back() = calic_contexts;
	return population;
}

vv_int evolve_predictors(vv_int population) {
	int population_size = population.size();
	for (int i = 0; i < population_size; i++) {
		if (rnd01() <= pcross) {
			// crossover
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

			v_int child1 = v_int(pred_ind_size);
			std::copy(population[i].begin(), population[i].begin() + l, child1.begin());
			std::copy(population[j].begin() + l, population[j].begin() + r, child1.begin() + l);
			std::copy(population[i].begin() + r, population[i].end(), child1.begin() + r);
			population.push_back(child1);

			v_int child2 = v_int(pred_ind_size);
			std::copy(population[j].begin(), population[j].begin() + l, child2.begin());
			std::copy(population[i].begin() + l, population[i].begin() + r, child2.begin() + l);
			std::copy(population[j].begin() + r, population[j].end(), child2.begin() + r);
			population.push_back(child2);
		}

		if (rnd01() <= pmut) {
			// mutate
			int p = (int)std::floor(rnd01() * (double)pred_ind_size);

			v_int child = v_int(population[i]);
			if (p % 3 == 0) {
				child[p] = population[i][p] == 1 ? 0 : 1;
			}
			else if (p % 3 == 1) {
				int new_val;
				do {
					new_val = (int)std::floor(rnd01() * 8.0);
				} while (new_val == population[i][p]);
				child[p] = new_val;
			}
			else {
				int d;
				bool in_range;
				do {
					d = (int)std::floor((rnd01() - rnd01()) * 500.0);
					in_range = (d != 0 && population[i][p] + d >= -255 && population[i][p] + d <= 255);
				} while (!in_range);
				child[p] += d;
			}
			population.push_back(child);
		}
	}
	return population;
}

std::pair<std::vector<v_int>, std::vector<std::vector<vv_int>>> evolve_tree_contexts(
	std::vector<v_int> context_population, std::vector<std::vector<vv_int>> predictor_population, int pred_pop_size) {
	int ctx_pop_size = context_population.size();
	for (int i = 0; i < ctx_pop_size; i++) {
		if (rnd01() <= pcross) {
			// crossover
			int j;
			do {
				j = (int)std::floor(rnd01() * (double)ctx_pop_size);
			} while (j == i);
		
			int p = 3 * ((int)std::floor(rnd01() * 7.0) + 1);
			
			v_int child1 = v_int(tree_ind_size);
			std::copy(context_population[i].begin(), context_population[i].begin() + p, child1.begin());
			std::copy(context_population[j].begin() + p, context_population[j].end(), child1.begin() + p);
			context_population.push_back(child1);

			v_int child2 = v_int(tree_ind_size);
			std::copy(context_population[j].begin(), context_population[j].begin() + p, child2.begin());
			std::copy(context_population[i].begin() + p, context_population[i].end(), child2.begin() + p);
			context_population.push_back(child2);

			p = pred_pop_size / 2;
			std::vector<vv_int> children_pred(contexts, vv_int(pred_pop_size));
			for (int k = 0; k < contexts; k++) {
				std::copy(predictor_population[i][k].begin(), predictor_population[i][k].begin() + p, children_pred[k].begin());
				std::copy(predictor_population[j][k].begin() + p, predictor_population[j][k].begin() + pred_pop_size, children_pred[k].begin() + p);
			}
			predictor_population.push_back(children_pred);
			predictor_population.push_back(children_pred);
		}

		if (rnd01() <= pmut) {
			// mutate
			int p = (int)std::floor(rnd01() * (double)tree_ind_size);
			v_int child = v_int(context_population[i]);
			int new_val;
			if (p % 3 == 2) {
				int d;
				bool in_range;
				do {
					d = (int)std::floor((rnd01() - rnd01()) * 500.0);
					in_range = (d != 0 && context_population[i][p] + d >= -255 && context_population[i][p] + d <= 255);
				} while (!in_range);
				child[p] += d;
			}
			else {
				do {
					new_val = (int)std::floor(rnd01() * 8.0);
				} while (new_val == context_population[i][p] || new_val == context_population[i][(p % 3 == 0 ? p + 1 : p - 1)]);
				child[p] = new_val;
			}
			context_population.push_back(child);
			predictor_population.push_back(predictor_population[i]);
		}
	}
	return { context_population, predictor_population };
}

std::pair<std::vector<v_int>, std::vector<std::vector<vv_int>>> evolve_contexts(
	std::vector<v_int> context_population, std::vector<std::vector<vv_int>> predictor_population, int pred_pop_size) {
	int ctx_pop_size = context_population.size();
	for (int i = 0; i < ctx_pop_size; i++) {
		if (rnd01() <= pcross) {
			// crossover
			int j;
			do {
				j = (int)std::floor(rnd01() * (double)ctx_pop_size);
			} while (j == i);
		
			int p = 2 * ((int)std::floor(rnd01() * 6.0) + 1);
			
			v_int child1 = v_int(cont_ind_size);
			std::copy(context_population[i].begin(), context_population[i].begin() + p, child1.begin());
			std::copy(context_population[j].begin() + p, context_population[j].end(), child1.begin() + p);
			context_population.push_back(child1);

			v_int child2 = v_int(cont_ind_size);
			std::copy(context_population[j].begin(), context_population[j].begin() + p, child2.begin());
			std::copy(context_population[i].begin() + p, context_population[i].end(), child2.begin() + p);
			context_population.push_back(child2);

			p = pred_pop_size / 2;
			std::vector<vv_int> children_pred(contexts, vv_int(pred_pop_size));
			for (int k = 0; k < contexts; k++) {
				std::copy(predictor_population[i][k].begin(), predictor_population[i][k].begin() + p, children_pred[k].begin());
				std::copy(predictor_population[j][k].begin() + p, predictor_population[j][k].begin() + pred_pop_size, children_pred[k].begin() + p);
			}
			predictor_population.push_back(children_pred);
			predictor_population.push_back(children_pred);
		}

		if (rnd01() <= pmut) {
			// mutate
			int p = (int)std::floor(rnd01() * (double)cont_ind_size);
			v_int child = v_int(context_population[i]);
			int new_val;
			do {
				new_val = (int)std::floor(rnd01() * 8.0);
			} while (new_val == context_population[i][p] || new_val == context_population[i][(p % 2 == 0 ? p + 1 : p - 1)]);
			child[p] = new_val;
			context_population.push_back(child);
			predictor_population.push_back(predictor_population[i]);
		}
	}
	return { context_population, predictor_population };
}

vv_int tune_predictor(const image& img, std::vector<vv_int> population, 
	int population_size = 20, int generations_cnt = 20, 
	v_int ctx_vals = calic_contexts, std::function<int(const image&, size_t, size_t, int, const v_int&)> get_ctx = get_ctx_calic) 
{
	std::vector<std::pair<v_int, double>> best_vals(contexts, { v_int(), 0.0 }); // a pair of best predictor and its fitness for each context
	for (int g = 0; g < generations_cnt; g++) {
		// evolve current population
		for (int k = 0; k < contexts; k++) {
			population[k] = evolve_predictors(population[k]);
		}

		// prepare to count fitness for each individual
		std::vector<std::vector<std::pair<double, int>>> fitness(contexts);
		for (int k = 0; k < contexts; k++) {
			for (int i = 0; i < population[k].size(); i++) {
				fitness[k].push_back({ 0.0, i });
			}
		}
		std::vector<double> sum_fitness(contexts, 0.0);

		// exec full image with current population
		for (int x = 2; x < img.width - 1; x++) {
			for (int y = 2; y < img.height; y++) {
				for (int col = 0; col < img.colors; col++) {
					int k = get_ctx(img, x, y, col, ctx_vals);

					for (int i = 0; i < population[k].size(); i++) {
						// for each individual in current population for context k
						int res1 = 0, res2 = 0;
						for (size_t j = 0; j < pred_ind_size; j += 3) {
							int pix = int_to_pix(img, x, y, col, population[k][i][j + 1]);
							res1 += population[k][i][j] * pix * population[k][i][j + 2];
							res2 += population[k][i][j] * population[k][i][j + 2];
						}
						int pred = res2 == 0 ? 0 : res1 / res2;
						double f = fitness_fun[std::abs((int)img(x, y, col) - pred)];
						
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
			vv_int selected(0);
			for (int i = 0; i < population_size; i++) {
				selected.push_back(population[k][fitness[k][i].second]);
				if (fitness[k][i].first > best_vals[k].second) {
					best_vals[k].first = population[k][fitness[k][i].second];
					best_vals[k].second = fitness[k][i].first;
				}
			}
			population[k] = selected;
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

		//	vv_int selected(0);
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

//vv_int tune_pred(const image& img, std::vector<vv_int> population, 
//	int population_size = 20, int generations_cnt = 20, 
//	v_int ctx_vals = { 0, 1, 2, 5, 3, 6, 0, 4, 2, 1, 3, 2 }) 
//{
//	std::vector<std::pair<v_int, double>> best_vals(contexts, { v_int(), 0.0 });
//
//	for (int g = 0; g < generations_cnt; g++) {
//		// evolve current population
//		for (int k = 0; k < contexts; k++) {
//			population[k] = evolve_predictors(population[k]);
//		}
//
//		// prepare to count fitness for each individual
//		std::vector<std::vector<std::pair<double, int>>> fitness(contexts);
//		for (int k = 0; k < contexts; k++) {
//			for (int i = 0; i < population[k].size(); i++) {
//				fitness[k].push_back({ 0.0, i });
//			}
//		}
//		std::vector<double> sum_fitness(contexts, 0.0);
//
//		// exec full image with current population
//		for (int x = 2; x < img.width - 1; x++) {
//			for (int y = 2; y < img.height; y++) {
//				for (int col = 0; col < img.colors; col++) {
//					auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
//					short dv = 
//						std::abs(get_pix_val(ctx_vals[0]) - get_pix_val(ctx_vals[1])) +
//						std::abs(get_pix_val(ctx_vals[2]) - get_pix_val(ctx_vals[3])) +
//						std::abs(get_pix_val(ctx_vals[4]) - get_pix_val(ctx_vals[5]));
//					short dh = 
//						std::abs(get_pix_val(ctx_vals[6]) - get_pix_val(ctx_vals[7])) +
//						std::abs(get_pix_val(ctx_vals[8]) - get_pix_val(ctx_vals[9])) +
//						std::abs(get_pix_val(ctx_vals[10]) - get_pix_val(ctx_vals[11]));
//					short D = dv - dh;
//
//					int k = get_ctx(D);
//
//					for (int i = 0; i < population[k].size(); i++) {
//						// for each individual in current population for context k
//						int res1 = 0, res2 = 0;
//						for (size_t j = 0; j < pred_ind_size; j += 3) {
//							int pix = int_to_pix(img, x, y, col, population[k][i][j + 1]);
//							res1 += population[k][i][j] * pix * population[k][i][j + 2];
//							res2 += population[k][i][j] * population[k][i][j + 2];
//						}
//						int pred = res2 == 0 ? 0 : res1 / res2;
//						double f = fitness_fun[std::abs((int)img(x, y, col) - pred)];
//						
//						fitness[k][i].first += f;
//						sum_fitness[k] += f;
//					}
//				}
//			}
//		}
//
//		// selection
//		for (int k = 0; k < contexts; k++) {
//			std::sort(fitness[k].begin(), fitness[k].end());
//			std::reverse(fitness[k].begin(), fitness[k].end());
//			vv_int selected(0);
//			for (int i = 0; i < population_size; i++) {
//				selected.push_back(population[k][fitness[k][i].second]);
//				if (fitness[k][i].first > best_vals[k].second) {
//					best_vals[k].first = population[k][fitness[k][i].second];
//					best_vals[k].second = fitness[k][i].first;
//				}
//			}
//			population[k] = selected;
//		}
//
//		// roulette wheel
//		//std::vector<std::vector<double>> wheel(contexts, std::vector<double>(0));
//		//for (int k = 0; k < contexts; k++) {
//		//	wheel[k].resize(fitness[k].size());
//		//	double prev = 0.0;
//		//	std::sort(fitness[k].begin(), fitness[k].end());
//		//	for (int i = 0; i < fitness[k].size(); i++) {
//		//		prev += fitness[k][i].first / sum_fitness[k];
//		//		wheel[k][i] = prev;
//		//	}
//
//		//	vv_int selected(0);
//		//	for (int i = 0; i < population_size; i++) {
//		//		double rand = rnd01();
//		//		int j = 0;
//		//		while (rand >= wheel[k][j]) {
//		//			j++;
//		//		}
//		//		selected.push_back(full_population[k][fitness[k][j].second]);
//		//		if (fitness[k][j].first > best_vals[k].second) {
//		//			best_vals[k].first = full_population[k][fitness[k][j].second];
//		//			best_vals[k].second = fitness[k][j].first;
//		//		}
//		//	}
//		//	full_population[k] = selected;
//		//}
//	}
//
//	std::vector<std::vector<int>> coeffs(contexts);
//	for (int i = 0; i < contexts; i++) {
//		coeffs[i] = best_vals[i].first;
//	}
//	return coeffs;
//}

double gen_in_gen_tree(const image& img, 
	int context_pop_size = 10, int context_elite_size = 3, 
	int predictor_pop_size = 5,	int generations_cnt = 5) {
	init_fitness_fun();

	auto ctx_vals = init_tree_contexts(context_pop_size);
	std::vector<std::vector<vv_int>> predictors(ctx_vals.size());
	for (int i = 0; i < predictors.size(); i++) {
		predictors[i] = init_predictors(predictor_pop_size);
	}

	std::pair<v_int, double> best_ctx_vals({ v_int(), 1000.0 });
	std::vector<vv_int> best_pred_pop = std::vector<vv_int>(contexts, std::vector<v_int>(predictor_pop_size, v_int(pred_ind_size)));
	for (int g = 0; g < generations_cnt; g++) {
		std::cout << "gen " << g << ": ";

		// evolve current population
		auto evolved = evolve_tree_contexts(ctx_vals, predictors, predictor_pop_size);
		ctx_vals = evolved.first;
		predictors = evolved.second;
		
		// prepare to count fitness for each individual
		std::vector<std::pair<double, int>> ctx_fitness(ctx_vals.size());
		double sum_ctx_fitness = 0.0;
		for (int i = 0; i < ctx_vals.size(); i++) {
			std::vector<std::pair<v_int, double>> best_pred_vals(contexts, { v_int(), 0.0 });

			// evolve current population
			for (int k = 0; k < contexts; k++) {
				predictors[i][k] = evolve_predictors(predictors[i][k]);
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
						int k = get_ctx_tree(img, x, y, col, ctx_vals[i]);

						for (int j = 0; j < predictors[i][k].size(); j++) {
							// for each individual in current population for context k
							int res1 = 0, res2 = 0;
							for (int r = 0; r < pred_ind_size; r += 3) {
								int pix = int_to_pix(img, x, y, col, predictors[i][k][j][r + 1]);
								res1 += predictors[i][k][j][r] * pix * predictors[i][k][j][r + 2];
								res2 += predictors[i][k][j][r] * predictors[i][k][j][r + 2];
							}
							int pred = res2 == 0 ? 0 : res1 / res2;
							double f = fitness_fun[std::abs((int)img(x, y, col) - pred)];

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
				vv_int selected(predictor_pop_size);
				for (int j = 0; j < predictor_pop_size; j++) {
					selected[j] = predictors[i][k][pred_fitness[k][j].second];
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

			double entr = get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, ctx_vals[i], coeffs, get_ctx_tree));
			ctx_fitness[i] = { entr, i };
			sum_ctx_fitness += entr;
		}

		// selection
		//vv_int selected(context_pop_size);
		//std::vector<std::vector<vv_int>> selected_pred(0);

		//std::sort(ctx_fitness.begin(), ctx_fitness.end());

		//// elite selection
		//for (int i = 0; i < context_elite_size; i++) {
		//	sum_ctx_fitness -= ctx_fitness[i].first;
		//	selected[i] = ctx_vals[ctx_fitness[i].second];
		//	selected_pred.push_back(predictors[ctx_fitness[i].second]);
		//	if (ctx_fitness[i].first < best_ctx_vals.second) {
		//		best_ctx_vals.first = ctx_vals[ctx_fitness[i].second];
		//		//for (int k = 0; k < 8; k++) {
		//		//	best_values.first.second[k] = best_predictors[context_fitness[j].second][k].first;
		//		//}
		//		best_ctx_vals.second = ctx_fitness[i].first;
		//		best_pred_pop = predictors[ctx_fitness[i].second];
		//	}
		//}

		//// roulette wheel selection
		//std::vector<double> wheel(ctx_fitness.size() - context_elite_size);
		//double prev = 0.0;
		//for (int i = 0; i < ctx_fitness.size() - context_elite_size; i++) {
		//	prev += 1 - (ctx_fitness[ctx_fitness.size() - i - 1].first / sum_ctx_fitness);
		//	wheel[i] = prev;
		//}
		//for (int i = context_elite_size; i < context_pop_size; i++) {
		//	double rand = rnd01();
		//	int j = 0;
		//	while (rand >= wheel[j]) {
		//		j++;
		//	}
		//	selected[i] = ctx_vals[ctx_fitness[ctx_fitness.size() - j - 1].second];
		//	selected_pred.push_back(predictors[ctx_fitness[ctx_fitness.size() - j - 1].second]);
		//}
		//ctx_vals = selected;
		//predictors = selected_pred;

		// selection
		std::sort(ctx_fitness.begin(), ctx_fitness.end());
		vv_int selected(0);
		std::vector<std::vector<vv_int>> selected_pred(0);
		for (int i = 0; i < context_pop_size; i++) {
			selected.push_back(ctx_vals[ctx_fitness[i].second]);
			selected_pred.push_back(predictors[ctx_fitness[i].second]);
			if (ctx_fitness[i].first < best_ctx_vals.second) {
				best_ctx_vals.first = ctx_vals[ctx_fitness[i].second];
				best_ctx_vals.second = ctx_fitness[i].first;
				best_pred_pop = predictors[ctx_fitness[i].second];
			}
		}
		ctx_vals = selected;
		predictors = selected_pred;

		std::cout << best_ctx_vals.second << '\n';
	}

	vv_int best_coeffs = tune_predictor(img, best_pred_pop, predictor_pop_size, 10, best_ctx_vals.first, get_ctx_tree);
	return get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, best_ctx_vals.first, best_coeffs, get_ctx_tree));
}

double gen_in_gen(const image& img, std::string img_name,
	int context_pop_size = 10, int context_elite_size = 3, 
	int predictor_pop_size = 5,	int generations_cnt = 5) {
	std::ofstream logfile(def_filename("calic", img_name, context_pop_size, predictor_pop_size, generations_cnt));

	init_fitness_fun();

	auto ctx_vals = init_contexts(context_pop_size);
	std::vector<std::vector<vv_int>> predictors(ctx_vals.size());
	for (int i = 0; i < predictors.size(); i++) {
		predictors[i] = init_predictors(predictor_pop_size);
	}

	std::pair<v_int, double> best_ctx_vals({ v_int(), 1000.0 });
	std::vector<vv_int> best_pred_pop = std::vector<vv_int>(contexts, std::vector<v_int>(predictor_pop_size, v_int(pred_ind_size)));
	for (int g = 0; g < generations_cnt; g++) {
		// evolve current population
		auto evolved = evolve_contexts(ctx_vals, predictors, predictor_pop_size);
		ctx_vals = evolved.first;
		predictors = evolved.second;
		
		// prepare to count fitness for each individual
		std::vector<std::pair<double, int>> ctx_fitness(ctx_vals.size());
		double sum_ctx_fitness = 0.0;
		for (int i = 0; i < ctx_vals.size(); i++) {
			std::vector<std::pair<v_int, double>> best_pred_vals(contexts, { v_int(), 0.0 });

			// evolve current population
			for (int k = 0; k < contexts; k++) {
				predictors[i][k] = evolve_predictors(predictors[i][k]);
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
						int k = get_ctx_calic(img, x, y, col, ctx_vals[i]);

						for (int j = 0; j < predictors[i][k].size(); j++) {
							// for each individual in current population for context k
							int res1 = 0, res2 = 0;
							for (int r = 0; r < pred_ind_size; r += 3) {
								int pix = int_to_pix(img, x, y, col, predictors[i][k][j][r + 1]);
								res1 += predictors[i][k][j][r] * pix * predictors[i][k][j][r + 2];
								res2 += predictors[i][k][j][r] * predictors[i][k][j][r + 2];
							}
							int pred = res2 == 0 ? 0 : res1 / res2;
							double f = fitness_fun[std::abs((int)img(x, y, col) - pred)];

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
				vv_int selected(predictor_pop_size);
				for (int j = 0; j < predictor_pop_size; j++) {
					selected[j] = predictors[i][k][pred_fitness[k][j].second];
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

			double entr = get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, ctx_vals[i], coeffs, get_ctx_calic));
			ctx_fitness[i] = { entr, i };
			sum_ctx_fitness += entr;
		}

		// selection
		//vv_int selected(context_pop_size);
		//std::vector<std::vector<vv_int>> selected_pred(0);

		//std::sort(ctx_fitness.begin(), ctx_fitness.end());

		//// elite selection
		//for (int i = 0; i < context_elite_size; i++) {
		//	sum_ctx_fitness -= ctx_fitness[i].first;
		//	selected[i] = ctx_vals[ctx_fitness[i].second];
		//	selected_pred.push_back(predictors[ctx_fitness[i].second]);
		//	if (ctx_fitness[i].first < best_ctx_vals.second) {
		//		best_ctx_vals.first = ctx_vals[ctx_fitness[i].second];
		//		//for (int k = 0; k < 8; k++) {
		//		//	best_values.first.second[k] = best_predictors[context_fitness[j].second][k].first;
		//		//}
		//		best_ctx_vals.second = ctx_fitness[i].first;
		//		best_pred_pop = predictors[ctx_fitness[i].second];
		//	}
		//}

		//// roulette wheel selection
		//std::vector<double> wheel(ctx_fitness.size() - context_elite_size);
		//double prev = 0.0;
		//for (int i = 0; i < ctx_fitness.size() - context_elite_size; i++) {
		//	prev += 1 - (ctx_fitness[ctx_fitness.size() - i - 1].first / sum_ctx_fitness);
		//	wheel[i] = prev;
		//}
		//for (int i = context_elite_size; i < context_pop_size; i++) {
		//	double rand = rnd01();
		//	int j = 0;
		//	while (rand >= wheel[j]) {
		//		j++;
		//	}
		//	selected[i] = ctx_vals[ctx_fitness[ctx_fitness.size() - j - 1].second];
		//	selected_pred.push_back(predictors[ctx_fitness[ctx_fitness.size() - j - 1].second]);
		//}
		//ctx_vals = selected;
		//predictors = selected_pred;

		// selection
		std::sort(ctx_fitness.begin(), ctx_fitness.end());
		vv_int selected(0);
		std::vector<std::vector<vv_int>> selected_pred(0);
		for (int i = 0; i < context_pop_size; i++) {
			selected.push_back(ctx_vals[ctx_fitness[i].second]);
			selected_pred.push_back(predictors[ctx_fitness[i].second]);
			if (ctx_fitness[i].first < best_ctx_vals.second) {
				best_ctx_vals.first = ctx_vals[ctx_fitness[i].second];
				best_ctx_vals.second = ctx_fitness[i].first;
				best_pred_pop = predictors[ctx_fitness[i].second];
			}
		}
		ctx_vals = selected;
		predictors = selected_pred;

		logfile << g << ' ' << best_ctx_vals.second << '\n';
	}

	//auto random_preds = init_predictors(20 - predictor_pop_size);
	//for (int k = 0; k < contexts; k++) {
	//	best_pred_pop[k].insert(best_pred_pop[k].end(), random_preds[k].begin(), random_preds[k].end());
	//}
	//vv_int best_coeffs = tune_pred(img, best_pred_pop, 20, 20, best_ctx_vals.first);
	//vv_int best_coeffs = tune_predictor(img, best_pred_pop, predictor_pop_size, 10, best_ctx_vals.first, get_ctx_calic);
	//return get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, best_ctx_vals.first, best_coeffs, get_ctx_calic));
	return best_ctx_vals.second;
}

double basic_context(const image& img, int population_size = 30, int elite_size = 5, int generations = 5) {
	vv_int ctx_vals = init_contexts(population_size);
	
	vvv_int predictors = vvv_int(population_size, vv_int(contexts, v_int(pred_ind_size)));
	for (int i = 0; i < population_size - 1; i++) {
		for (int k = 0; k < contexts; k++) {
			for (int j = 0; j < pred_ind_size; j += 3) {
				predictors[i][k][j] = (int)std::floor(rnd01() * 2.0);
				predictors[i][k][j + 1] = (int)std::floor(rnd01() * 8.0);
				predictors[i][k][j + 2] = (int)std::floor(rnd01() * 256.0);
			}
		}
	}
	predictors.back() = calic_predictor;

	std::pair<double, int> best_pair({ 1000.0, -1 });
	for (int g = 0; g < generations; g++) {
		// evolve
		for (int i = 0; i < population_size; i++) {
			if (rnd01() <= pcross) {
				int j;
				do {
					j = (int)std::floor(rnd01() * (double)population_size);
				} while (j == i);

				// crossover predictors
				vv_int child1 = vv_int(contexts, v_int(pred_ind_size));
				vv_int child2 = vv_int(contexts, v_int(pred_ind_size));
				int p1, p2;
				do {
					p1 = (int)std::floor(rnd01() * (double)(pred_ind_size - 1)) + 1;
					p2 = (int)std::floor(rnd01() * (double)(pred_ind_size - 1)) + 1;
				} while (p1 == p2);
				int l = std::min(p1, p2);
				int r = std::max(p1, p2);
				for (int k = 0; k < contexts; k++) {
					std::copy(predictors[i][k].begin(), predictors[i][k].begin() + l, child1[k].begin());
					std::copy(predictors[j][k].begin() + l, predictors[j][k].begin() + r, child1[k].begin() + l);
					std::copy(predictors[i][k].begin() + r, predictors[i][k].end(), child1[k].begin() + r);

					std::copy(predictors[j][k].begin(), predictors[j][k].begin() + l, child2[k].begin());
					std::copy(predictors[i][k].begin() + l, predictors[i][k].begin() + r, child2[k].begin() + l);
					std::copy(predictors[j][k].begin() + r, predictors[j][k].end(), child2[k].begin() + r);
				}
				predictors.push_back(child1);
				predictors.push_back(child2);
				
				// crossover contexts
				int p = 2 * ((int)std::floor(rnd01() * 6.0) + 1);
				v_int child3 = v_int(cont_ind_size);
				std::copy(ctx_vals[i].begin(), ctx_vals[i].begin() + p, child3.begin());
				std::copy(ctx_vals[j].begin() + p, ctx_vals[j].end(), child3.begin() + p);
				ctx_vals.push_back(child3);

				std::copy(ctx_vals[j].begin(), ctx_vals[j].begin() + p, child3.begin());
				std::copy(ctx_vals[i].begin() + p, ctx_vals[i].end(), child3.begin() + p);
				ctx_vals.push_back(child3);
			}
			if (rnd01() <= pmut) {
				// mutate predictors
				int p = (int)std::floor(rnd01() * (double)pred_ind_size);

				int k = (int)std::floor(rnd01() * (double)contexts);
				vv_int child = vv_int(predictors[i]);
				if (p % 3 == 0) {
					child[k][p] = predictors[i][k][p] == 1 ? 0 : 1;
				}
				else if (p % 3 == 1) {
					int new_val;
					do {
						new_val = (int)std::floor(rnd01() * 8.0);
					} while (new_val == predictors[i][k][p]);
					child[k][p] = new_val;
				}
				else {
					int d;
					bool in_range;
					do {
						d = (int)std::floor((rnd01() - rnd01()) * 500.0);
						in_range = (d != 0 && predictors[i][k][p] + d >= -255 && predictors[i][k][p] + d <= 255);
					} while (!in_range);
					child[k][p] += d;
				}
				predictors.push_back(child);

				// mutate contexts
				p = (int)std::floor(rnd01() * (double)cont_ind_size);
				v_int child1 = v_int(ctx_vals[i]);
				int new_val;
				do {
					new_val = (int)std::floor(rnd01() * 8.0);
				} while (new_val == ctx_vals[i][p] || new_val == ctx_vals[i][(p % 2 == 0 ? p + 1 : p - 1)]);
				child1[p] = new_val;
				ctx_vals.push_back(child1);
			}
		}

		//count fitness
		std::vector<std::pair<double, int>> fitness(predictors.size());
		for (int i = 0; i < ctx_vals.size(); i++) {
			double entr = get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, ctx_vals[i], predictors[i], get_ctx_calic));
			fitness[i] = { entr, i };
			if (entr < best_pair.first) {
				best_pair.first = entr;
				best_pair.second = i;
			}
		}
		std::cout << "gen " << g << ": " << best_pair.first << '\n';

		// selection
		std::sort(fitness.begin(), fitness.end());
		vv_int selected(0);
		vvv_int selected_pred(0);
		for (int i = 0; i < population_size; i++) {
			selected.push_back(ctx_vals[fitness[i].second]);
			selected_pred.push_back(predictors[fitness[i].second]);
		}
		ctx_vals = selected;
		predictors = selected_pred;
	}

	return best_pair.first;
}

int bits_to_int(std::vector<int> values, int l, int r) {
	int result = 0;
	for (int i = l; i < r; i++) {
		result <<= 1;
		result += values[i];
	}
	return result;
}

//double evolvable_predictor(const image& img, int population_size = 20) {
//	init_fitness_fun();
//
//	std::vector<std::vector<std::vector<int>>> population(8, std::vector<std::vector<int>>(population_size, std::vector<int>(48)));
//	std::vector<std::pair<std::vector<int>, double>> best_individuals(8, { std::vector<int>(48), 0.0 });
//
//	// init population with random values
//	for (int k = 0; k < 8; k++) {
//		for (int i = 0; i < population_size; i++) {
//			for (int j = 0; j < 48; j++) {
//				population[k][i][j] = rnd01() < 0.5 ? 1 : 0;
//			}
//		}
//	}
//
//	// iterate the image
//	for (int x = 2; x < img.width - 1; x++) {
//		for (int y = 2; y < img.height; y++) {
//			for (int col = 0; col < img.colors; col++) {
//				// determine the context k
//				auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
//				int dh = 
//					std::abs(get_pix_val(0) - get_pix_val(4)) + 
//					std::abs(get_pix_val(2) - get_pix_val(1)) + 
//					std::abs(get_pix_val(3) - get_pix_val(2));
//				int dv = 
//					std::abs(get_pix_val(0) - get_pix_val(1)) + 
//					std::abs(get_pix_val(2) - get_pix_val(5)) + 
//					std::abs(get_pix_val(3) - get_pix_val(6));
//				//int k = get_ctx_no(dh - dv); // check
//
//				// evolve population for context k
//				for (int i = 0; i < population_size; i++) {
//					// crossover
//					if (rnd01() < 0.5) {
//						// select partner
//						int j;
//						do {
//							j = (int)std::floor(rnd01() * (double)population_size);
//						} while (j == i);
//
//						// select crossover points
//						int k1, k2;
//						do {
//							k1 = (int)std::floor(rnd01() * 46.0) + 1;
//							k2 = (int)std::floor(rnd01() * 46.0) + 1;
//						} while (k1 == k2);
//						int l = std::min(k1, k2);
//						int r = std::max(k1, k2);
//
//						std::vector<int> child1(48);
//						std::copy(population[k][i].begin(), population[k][i].begin() + l, child1.begin());
//						std::copy(population[k][j].begin() + l, population[k][j].begin() + r, child1.begin() + l);
//						std::copy(population[k][i].begin() + r, population[k][i].end(), child1.begin() + r);
//						population[k].push_back(child1);
//
//						std::vector<int> child2(48);
//						std::copy(population[k][j].begin(), population[k][j].begin() + l, child2.begin());
//						std::copy(population[k][i].begin() + l, population[k][i].begin() + r, child2.begin() + l);
//						std::copy(population[k][j].begin() + r, population[k][j].end(), child2.begin() + r);
//						population[k].push_back(child2);
//					}
//
//					// mutation
//					if (rnd01() < 0.01) {
//						int j = (int)std::floor(rnd01() * 48.0);
//
//						// save as a child
//						std::vector<int> child(population[k][i]);
//						child[j] = child[j] == 0 ? 1 : 0;
//						population[k].push_back(child);
//
//						// change individual
//						//population[k][i][j] = population[k][i][j] == 0 ? 1 : 0;
//					}
//				}
//
//				double sum_fitness = 0.0;
//				std::vector<std::pair<double, int>> fitness(population[k].size());
//				// count fitness for population for context k
//				for (int i = 0; i < population[k].size(); i++) {
//					// C0 P0 E0 E1 P1 C1 C2 P2 E2 E3 P3 C3
//					int e[4] = { 
//						population[k][i][11], 
//						population[k][i][12], 
//						population[k][i][35], 
//						population[k][i][36] };
//					int p[4] = { 
//						bits_to_int(population[k][i], 8, 11), 
//						bits_to_int(population[k][i], 13, 16), 
//						bits_to_int(population[k][i], 32, 35),
//						bits_to_int(population[k][i], 37, 40) };
//					int c[4] = { 
//						bits_to_int(population[k][i], 0, 8), 
//						bits_to_int(population[k][i], 16, 24), 
//						bits_to_int(population[k][i], 24, 32),
//						bits_to_int(population[k][i], 40, 48) };
//
//					int res1 = 0, res2 = 0;
//					for (size_t j = 0; j < 4; j++) {
//						int pix = int_to_pix(img, x, y, col, p[j]);
//						res1 += e[j] * pix * c[j];
//						res2 += e[j] * c[j];
//					}
//					unsigned char prediction = res2 == 0 ? 0 : res1 / res2;
//					fitness[i] = { fitness_fun[std::abs((int)img(x, y, col) - (int)prediction)], i };
//					sum_fitness += fitness[i].first;
//				}
//
//				// initialise roulette wheel
//				std::vector<double> wheel(0);
//				wheel.resize(fitness.size());
//				double prev = 0.0;
//				std::sort(fitness.begin(), fitness.end());
//				for (int i = 0; i < fitness.size(); i++) {
//					prev += fitness[i].first / sum_fitness;
//					wheel[i] = prev;
//				}
//				// selection
//				std::vector<std::vector<int>> selected(population_size);
//				for (int i = 0; i < population_size; i++) {
//					double rand = rnd01();
//					int j = 0;
//					while (rand >= wheel[j]) {
//						j++;
//					}
//					selected[i] = population[k][fitness[j].second];
//					if (fitness[j].first > best_individuals[k].second) {
//						best_individuals[k].first = population[k][fitness[j].second];
//						best_individuals[k].second = fitness[j].first;
//					}
//				}
//				population[k] = selected;
//			}
//		}
//	}
//
//	// convert to ints
//	std::vector<std::vector<int>> coeffs(8, std::vector<int>(12));
//	// E0 E1 E2 E3 P0 P1 P2 P3 C0 C1 C2 C3
//	for (int k = 0; k < 8; k++) {
//		coeffs[k][0] = (best_individuals[k].first)[11];
//		coeffs[k][1] = (best_individuals[k].first)[12];
//		coeffs[k][2] = (best_individuals[k].first)[35];
//		coeffs[k][3] = (best_individuals[k].first)[36];
//
//		coeffs[k][4] = bits_to_int(best_individuals[k].first, 8, 11);
//		coeffs[k][5] = bits_to_int(best_individuals[k].first, 13, 16);
//		coeffs[k][6] = bits_to_int(best_individuals[k].first, 32, 35);
//		coeffs[k][7] = bits_to_int(best_individuals[k].first, 37, 40);
//
//		coeffs[k][8] = bits_to_int(best_individuals[k].first, 0, 8);
//		coeffs[k][9] = bits_to_int(best_individuals[k].first, 16, 24);
//		coeffs[k][10] = bits_to_int(best_individuals[k].first, 24, 32);
//		coeffs[k][11] = bits_to_int(best_individuals[k].first, 40, 48);
//	}
//
//	return get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, coeffs));
//}

double modified_evolvable_predictor(const image& img, int population_size = 10, int generations = 5) {
	init_fitness_fun();

	std::vector<std::vector<std::vector<int>>> population(8, std::vector<std::vector<int>>(population_size, std::vector<int>(12)));
	std::vector<std::pair<std::vector<int>, double>> best_individuals(8, { std::vector<int>(12), 0.0 });

	// init population with random values
	// E0 P0 C0 E1 P1 C1 E2 P2 C2 E3 P3 C3
	for (int k = 0; k < 8; k++) {
		for (int i = 0; i < population_size - 1; i++) {
			for (int j = 0; j < 4; j++) {
				population[k][i][3 * j] = (int)std::floor(rnd01() * 2.0);
				population[k][i][3 * j + 1] = (int)std::floor(rnd01() * 8.0);
				population[k][i][3 * j + 2] = (int)std::floor(rnd01() * 256.0);
			}
		}
	}
	population[0].push_back({ 1, 0,  1,  1, 2,  0,  1, 3, 0,  1, 1, 0 }); // a
	population[1].push_back({ 1, 0,  6,  1, 2,  2,  1, 3, 1,  1, 1, -1 }); // (3a + c)/4 + (d - b)/8
	population[2].push_back({ 1, 0, 10,  1, 2,  6,  1, 3, 3,  1, 1, -3 }); // (5a + 3c)/8 + (3d - 3b)/16
	population[3].push_back({ 1, 0,  2,  1, 2,  2,  1, 3, 1,  1, 1, -1 }); // (a + c)/2 + (d - b)/4
	population[4].push_back({ 1, 0,  2,  1, 2,  2,  1, 3, 1,  1, 1, -1 }); // (a + c)/2 + (d - b)/4
	population[5].push_back({ 1, 0,  6,  1, 2, 10,  1, 3, 3,  1, 1, -3 }); // (3a + 5c)/8 + (3d - 3b)/16
	population[6].push_back({ 1, 0,  2,  1, 2,  6,  1, 3, 1,  1, 1, -1 }); // (a + 3c)/4 + (d - b)/8
	population[7].push_back({ 1, 0,  0,  1, 2,  1,  1, 3, 0,  1, 1, 0 }); // c
	
	for (int g = 0; g < generations; g++) {
		// evolve populations for each context
		for (int k = 0; k < 8; k++) {
			for (int i = 0; i < population_size; i++) {
				// crossover
				if (rnd01() < 0.5) {
					// select partner
					int j;
					do {
						j = (int)std::floor(rnd01() * (double)population_size);
					} while (j == i);

					// select crossover points
					int k1, k2;
					do {
						k1 = (int)std::floor(rnd01() * 12.0) + 1;
						k2 = (int)std::floor(rnd01() * 12.0) + 1;
					} while (k1 == k2);
					int l = std::min(k1, k2);
					int r = std::max(k1, k2);

					std::vector<int> child1(12);
					std::copy(population[k][i].begin(), population[k][i].begin() + l, child1.begin());
					std::copy(population[k][j].begin() + l, population[k][j].begin() + r, child1.begin() + l);
					std::copy(population[k][i].begin() + r, population[k][i].end(), child1.begin() + r);
					population[k].push_back(child1);

					std::vector<int> child2(12);
					std::copy(population[k][j].begin(), population[k][j].begin() + l, child2.begin());
					std::copy(population[k][i].begin() + l, population[k][i].begin() + r, child2.begin() + l);
					std::copy(population[k][j].begin() + r, population[k][j].end(), child2.begin() + r);
					population[k].push_back(child2);
				}

				// mutation
				if (rnd01() < 0.01) {
					int j = (int)std::floor(rnd01() * 12.0);

					std::vector<int> child(population[k][i]);
					if (j % 3 == 0) {
						child[j] = child[j] == 0 ? 1 : 0;

					}
					else if (j % 3 == 1) {
						int new_val;
						do {
							new_val = (int)std::floor(rnd01() * 8.0);
						} while (new_val == population[k][i][j]);
						child[j] = new_val;
					}
					else {
						int d;
						bool in_range;
						do {
							d = (int)std::floor((rnd01() - rnd01()) * 500.0);
							in_range = (d != 0 && population[k][i][j] + d >= -255 && population[k][i][j] + d <= 255);
						} while (!in_range);
						child[j] += d;
					}
					population[k].push_back(child);
				}
			}
		}

		std::vector<double> sum_fitness(8, 0.0);
		std::vector<std::vector<std::pair<double, int>>> fitness(8);
		for (int k = 0; k < 8; k++) {
			fitness[k] = std::vector<std::pair<double, int>>(population[k].size());
			for (int i = 0; i < population[k].size(); i++) {
				fitness[k][i] = { 0.0, i };
			}
		}

		// iterate the image and count mean fitness
		for (int x = 2; x < img.width - 1; x++) {
			for (int y = 2; y < img.height; y++) {
				for (int col = 0; col < img.colors; col++) {
					// determine the context k
					auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
					int dh =
						std::abs(get_pix_val(0) - get_pix_val(4)) +
						std::abs(get_pix_val(2) - get_pix_val(1)) +
						std::abs(get_pix_val(3) - get_pix_val(2));
					int dv =
						std::abs(get_pix_val(0) - get_pix_val(1)) +
						std::abs(get_pix_val(2) - get_pix_val(5)) +
						std::abs(get_pix_val(3) - get_pix_val(6));
					int D = dv - dh;
					int k = -1;
					if (D > 80) {
						k = 0;
					}
					else if (D < -80) {
						k = 7;
					}
					else if (D > 32) {
						k = 1;
					}
					else if (D < -32) {
						k = 6;
					}
					else if (D > 8) {
						k = 2;
					}
					else if (D < -8) {
						k = 5;
					}
					else if (D > 0) {
						k = 3;
					}
					else {
						k = 4;
					}
					
					for (int i = 0; i < population[k].size(); i++) {
						int res1 = 0, res2 = 0;
						for (size_t j = 0; j < 4; j++) {
							int pix = int_to_pix(img, x, y, col, population[k][i][3 * j + 1]);
							res1 += population[k][i][3 * j] * pix * population[k][i][3 * j + 2];
							res2 += population[k][i][3 * j] * population[k][i][3 * j + 2];
						}
						unsigned char prediction = res2 == 0 ? 0 : res1 / res2;
						fitness[k][i].first += fitness_fun[std::abs((int)img(x, y, col) - (int)prediction)];
						sum_fitness[k] += fitness_fun[std::abs((int)img(x, y, col) - (int)prediction)];
					}
				}
			}
		}

		for (int k = 0; k < 8; k++) {
			std::sort(fitness[k].begin(), fitness[k].end());
			for (int i = 0; i < 5; i++) {
				sum_fitness[k] -= fitness[k][fitness[k].size() - i - 1].first;
			}

			// initialise roulette wheel
			std::vector<double> wheel(fitness[k].size() - 5);
			//std::vector<double> wheel(fitness[k].size());
			double prev = 0.0;
			for (int i = 0; i < fitness[k].size() - 5; i++) {
			//for (int i = 0; i < fitness[k].size(); i++) {
				prev += fitness[k][i].first / sum_fitness[k];
				wheel[i] = prev;
			}
			// selection
			std::vector<std::vector<int>> selected(population_size);
			for (int i = 0; i < 5; i++) {
				int j = fitness[k].size() - i - 1;
				selected[i] = population[k][fitness[k][j].second];
				if (fitness[k][j].first > best_individuals[k].second) {
					best_individuals[k].first = population[k][fitness[k][j].second];
					best_individuals[k].second = fitness[k][j].first;
				}
			}
			for (int i = 5; i < population_size; i++) {
			//for (int i = 0; i < population_size; i++) {
				double rand = rnd01();
				int j = 0;
				while (rand >= wheel[j]) {
					j++;
				}
				selected[i] = population[k][fitness[k][j].second];
			}
			population[k] = selected;
		}

		std::vector<std::vector<int>> coeffs(8, std::vector<int>(12));
		for (int k = 0; k < 8; k++) {
			coeffs[k] = best_individuals[k].first;
		}
		std::cout << "gen " << g << ": " << get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, calic_contexts, coeffs, get_ctx_calic)) << '\n';
	}

	std::vector<std::vector<int>> coeffs(8, std::vector<int>(12));
	for (int k = 0; k < 8; k++) {
		coeffs[k] = best_individuals[k].first;
	}
	return get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, calic_contexts, coeffs, get_ctx_calic));
}
