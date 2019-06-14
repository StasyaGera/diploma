#pragma once

#include <string>
#include <fstream>
#include <iostream>

#include "prediction.h"
#include "image.h"
#include "default_constants.h"
#include "evolvable_predictor.h"

const int calic_cont_ind_size = 12;

double evolvable_calic_context(const image& img, std::string img_name,
	int context_pop_size = 10, int context_elite_size = 5,
	int predictor_pop_size = 5, int predictor_elite_size = 5,
	int generations_cnt = 10) {
	std::ofstream logfile(def_filename("calic", img_name, context_pop_size, predictor_pop_size, generations_cnt));
	init_fitness_fun();

	// initialise context population with random values
	vv_int context_population(context_pop_size, v_int(calic_cont_ind_size));
	for (int i = 0; i < context_pop_size - 1; i++) {
		for (int j = 0; j < calic_cont_ind_size; j += 2) {
			do {
				context_population[i][j] = (int)std::floor(rnd01() * 8.0);
				context_population[i][j + 1] = (int)std::floor(rnd01() * 8.0);
			} while (context_population[i][j] == context_population[i][j + 1]);
		}
	}
	context_population.back() = calic_contexts;

	// initialise population of predictors for each context individual
	std::vector<vvv_int> predictor_population(context_pop_size);
	for (int i = 0; i < context_pop_size; i++) {
		predictor_population[i] = init_predictors(predictor_pop_size);
	}

	// store best context individual and its fitness
	std::pair<double, v_int> best_context_values({ 0.0, v_int(calic_cont_ind_size) });
	vv_int best_predictor = vv_int(contexts, v_int(pred_ind_size));
	for (int g = 0; g < generations_cnt; g++) {
		// evolve current contexts population
		for (int i = 0; i < context_pop_size; i++) {
			// crossover
			if (rnd01() < pcross) {
				int j;
				do {
					j = (int)std::floor(rnd01() * (double)context_pop_size);
				} while (j == i);
				int p = 2 * ((int)std::floor(rnd01() * 5.0) + 1);

				v_int child1(calic_cont_ind_size);
				std::copy(context_population[i].begin(), context_population[i].begin() + p, child1.begin());
				std::copy(context_population[j].begin() + p, context_population[j].end(), child1.begin() + p);
				context_population.push_back(child1);

				v_int child2(calic_cont_ind_size);
				std::copy(context_population[j].begin(), context_population[j].begin() + p, child2.begin());
				std::copy(context_population[i].begin() + p, context_population[i].end(), child2.begin() + p);
				context_population.push_back(child2);

				// copy parents' predictors
				p = predictor_pop_size / 2;
				vvv_int children_predictors(contexts, vv_int(predictor_pop_size));
				for (int k = 0; k < contexts; k++) {
					std::copy(predictor_population[i][k].begin(), predictor_population[i][k].begin() + p,
						children_predictors[k].begin());
					std::copy(predictor_population[j][k].begin() + p, predictor_population[j][k].begin() + predictor_pop_size,
						children_predictors[k].begin() + p);
				}
				predictor_population.push_back(children_predictors);
				predictor_population.push_back(children_predictors);
			}
			// mutation
			if (rnd01() < pmut) {
				int p = (int)std::floor(rnd01() * (double)calic_cont_ind_size);
				v_int child(context_population[i]);
				int new_val;
				do {
					new_val = (int)std::floor(rnd01() * 8.0);
				} while (new_val == context_population[i][p] || new_val == context_population[i][(p % 2 == 0 ? p + 1 : p - 1)]);
				child[p] = new_val;
				context_population.push_back(child);
				predictor_population.push_back(predictor_population[i]);
			}
		}

		std::vector<std::vector<std::pair<double, v_int>>> best_individuals(context_population.size(),
			std::vector<std::pair<double, v_int>>(contexts, { 0.0, v_int(pred_ind_size) }));
		std::vector<std::pair<double, int>> context_fitness(context_population.size());
		double sum_ctx_fitness = 0.0;
		for (int i = 0; i < context_population.size(); i++) {
			evolve_predictors(predictor_population[i]);
			best_individuals[i] = select_predictors(img, predictor_population[i], predictor_pop_size, predictor_elite_size,
				get_ctx_calic, context_population[i]);

			vv_int coeffs(contexts);
			for (int k = 0; k < contexts; k++) {
				coeffs[k] = best_individuals[i][k].second;
			}
			double entr = 1.0 / get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, coeffs, context_population[i], get_ctx_calic));
			context_fitness[i] = { entr, i };
			sum_ctx_fitness += entr;
		}

		std::sort(context_fitness.begin(), context_fitness.end());
		double best_fitness = context_fitness.back().first;
		double best_i = context_fitness.back().second;
		if (best_fitness > best_context_values.first) {
			best_context_values.first = best_fitness;
			best_context_values.second = context_population[best_i];
			for (int k = 0; k < contexts; k++) {
				best_predictor[k] = best_individuals[best_i][k].second;
			}
		}

		vv_int selected(context_pop_size);
		std::vector<vvv_int> selected_pred(context_pop_size);
		// elite selection
		for (int i = 0; i < context_elite_size; i++) {
			int j = context_fitness.size() - i - 1;
			sum_ctx_fitness -= context_fitness[j].first;
			selected[i] = context_population[context_fitness[j].second];
			selected_pred[i] = predictor_population[context_fitness[j].second];
		}

		// roulette wheel selection
		std::vector<double> wheel(context_fitness.size() - context_elite_size);
		double prev = 0.0;
		for (int i = 0; i < context_fitness.size() - context_elite_size; i++) {
			prev += context_fitness[i].first / sum_ctx_fitness;
			wheel[i] = prev;
		}
		for (int i = context_elite_size; i < context_pop_size; i++) {
			double rand = rnd01();
			int j = 0;
			while (rand >= wheel[j]) {
				j++;
			}
			selected[i] = context_population[context_fitness[j].second];
			selected_pred[i] = predictor_population[context_fitness[j].second];
		}
		context_population = selected;
		predictor_population = selected_pred;

		logfile << g + 1 << ' ' << get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, best_predictor, best_context_values.second, get_ctx_calic)) << '\n';
		logfile << "{ ";
		for (int i = 0; i < calic_cont_ind_size; i++) {
			logfile << best_context_values.second[i] << ' ';
		}
		logfile << "}\n";
	}
	logfile.close();
	return get_err_entropy(img, std::bind(evolvable, _1, _2, _3, _4, best_predictor, best_context_values.second, get_ctx_calic));
}
