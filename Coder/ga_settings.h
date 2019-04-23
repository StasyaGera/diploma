#pragma once

#include <functional>
#include <fstream>
#include <string>
#include <random>
#include <cmath>
#include <numeric>

#include "openga.hpp"

#include "image.h"
#include "prediction.h"
#include "default_constants.h"

typedef std::vector<std::vector<short>> vvshort;

using namespace std::placeholders;

std::ofstream log_file, gbg_file;

const int max_alpha_value = 15;
const int bits_in_alpha = std::log2(max_alpha_value + 1);
const int choices_num = 6;
const int alphas_per_choice = 3;
const vvshort jpeg_ls_emulation = vvshort({ {15, 0, 0}, {1, 1, -1}, {0, 15, 0}, {0, 15, 0}, {1, 1, -1}, {15, 0, 0} });

struct MyWeights
{
	vvshort alphas = vvshort(choices_num, std::vector<short>(alphas_per_choice, 0));

    std::string to_string() const
    {
        std::string result = "{";
        for (int i = 0; i < choices_num; i++) {
			for (int j = 0; j < alphas_per_choice; j++) {
				result += 
					(j == 0 ? "{" : " ") + 
					std::to_string(alphas[i][j]) + 
					(j == alphas_per_choice - 1 ? "}" : ",");
			}
			result += (i == choices_num - 1 ? "}" : ", ");
        }
        return result;
    }
};

struct MyMiddleCost
{
    double entropy;
};

typedef EA::Genetic<MyWeights, MyMiddleCost> GA_Type;
typedef EA::GenerationType<MyWeights, MyMiddleCost> Generation_Type;

void init_genes(MyWeights& p, const std::function<double(void)> &rnd01, bool init_jpeg = true)
{
    p.alphas = vvshort(6, std::vector<short>(3, 0));
	double v = rnd01();
	if (init_jpeg && v < 0.15 * elite_percentage) {
		p.alphas = jpeg_ls_emulation;
	}
	else {
		for (int i = 0; i < choices_num; i++) {
			for (int j = 0; j < alphas_per_choice; j++) {
				p.alphas[i][j] = std::floor(rnd01() * (double)(2 * max_alpha_value + 1)) - max_alpha_value;
			}
		}
	}
}

bool eval_solution(
    const MyWeights& p,
    MyMiddleCost& c,
    const image& img,
    const double& max_entropy)
{
    c.entropy = get_err_entropy(img, bind_genetic(p.alphas));
    return c.entropy < max_entropy;
}

MyWeights mutate_by_bit_inversion(
    const MyWeights& X_base,
    const std::function<double(void)> &rnd01,
    double shrink_scale)
{
    MyWeights X_new;
    X_new = X_base;

	int r = std::floor(rnd01() * (choices_num * alphas_per_choice * bits_in_alpha));
	int i = r / 12, j = r % 3, k = r % 4;

    //int i = std::floor(rnd01() * choices_num);
    //int j = std::floor(rnd01() * alphas_per_choice);
    //int k = std::floor(rnd01() * bits_in_alpha);
    X_new.alphas[i][j] ^= (1 << k);

    return X_new;
}

MyWeights mutate_by_random_increase(
    const MyWeights& X_base,
    const std::function<double(void)> &rnd01,
    double shrink_scale)
{
    MyWeights X_new;
	int i = std::floor(rnd01() * (double)choices_num);
	int j = std::floor(rnd01() * (double)alphas_per_choice);
    bool in_range;
    do {
        X_new = X_base;
        X_new.alphas[i][j] += 0.2*(rnd01() - rnd01())*shrink_scale;
        in_range = (X_new.alphas[i][j] >= (double)(-max_alpha_value) && X_new.alphas[i][j] <= (double)max_alpha_value);
    } while (!in_range);

    return X_new;
}

MyWeights crossover_on_block(
    const MyWeights& X1,
    const MyWeights& X2,
    const std::function<double(void)> &rnd01)
{
    MyWeights X_new;
    int r = std::floor(rnd01() * (double)choices_num);
    std::copy(X1.alphas.begin(), X1.alphas.begin() + r, X_new.alphas.begin());
    std::copy(X2.alphas.begin() + r, X2.alphas.end(), X_new.alphas.begin() + r);

    return X_new;
}

MyWeights crossover_on_random(
    const MyWeights& X1,
    const MyWeights& X2,
    const std::function<double(void)> &rnd01)
{
    MyWeights X_new;
    int r = std::floor(rnd01() * (double)(choices_num * alphas_per_choice));
    int b = r / alphas_per_choice;
    int a = r % alphas_per_choice;

    std::copy(X1.alphas.begin(), X1.alphas.begin() + b, X_new.alphas.begin());
    std::copy(X1.alphas[b].begin(), X1.alphas[b].begin() + a, X_new.alphas[b].begin());
    std::copy(X2.alphas[b].begin() + a, X2.alphas[b].end(), X_new.alphas[b].begin() + a);
    std::copy(X2.alphas.begin() + b + 1, X2.alphas.end(), X_new.alphas.begin() + b + 1);

    return X_new;
}

void SO_report_generation(
    int generation_number,
    const EA::GenerationType<MyWeights, MyMiddleCost> &last_generation,
    const MyWeights& best_genes)
{
    log_file
        << generation_number << " | "
        << best_genes.to_string() << " | "
        << last_generation.average_cost << " | "
        << last_generation.best_total_cost << "\n";

	gbg_file << generation_number << ' ' << last_generation.best_total_cost << '\n';
}

double calculate_SO_total_fitness(const GA_Type::thisChromosomeType &X)
{
    return X.middle_costs.entropy;
}

vvshort generate_weights(const image& img, std::string img_name, double entropy, int population = def_population_sz, bool init_jpeg = true, int generation_max = def_generations) 
{
    log_file.open(def_verbose_log_filename(img_name, population, init_jpeg));
    gbg_file.open(def_err_gen_by_gen_filename(img_name, population, init_jpeg));
    log_file << "step" << "\t" << "x_best" << "\t" << "y_best" << "\t" << "cost_avg" << "\t" << "cost_best" << "\n";

    EA::Chronometer timer;
    timer.tic();

    GA_Type ga_obj;
    ga_obj.problem_mode = EA::GA_MODE::SOGA; // single objective
    ga_obj.multi_threading = true;
    ga_obj.idle_delay_us = 1; // switch between threads quickly
    ga_obj.dynamic_threading = false; // gives thread a task if it is free
    ga_obj.verbose = false;

    ga_obj.population = population;
    ga_obj.generation_max = generation_max;
    ga_obj.best_stall_max = 10; // max generations with no changes in best score
    ga_obj.elite_count = elite_percentage * population; 
    ga_obj.crossover_fraction = 0.7;
    ga_obj.mutation_rate = 0.3;

    ga_obj.calculate_SO_total_fitness = calculate_SO_total_fitness;
    ga_obj.init_genes = std::bind(init_genes, _1, _2, init_jpeg);
    ga_obj.SO_report_generation = SO_report_generation;
    ga_obj.eval_solution = std::bind(eval_solution, _1, _2, img, entropy);

    ga_obj.mutate = mutate_by_bit_inversion;
    ga_obj.crossover = crossover_on_block;

    auto reason = ga_obj.solve();

    log_file << "The problem is optimized in " << timer.toc() << " seconds." << std::endl;
    log_file << "Stop reason: " << ga_obj.stop_reason_to_string(reason) << std::endl;
    log_file.close();
    gbg_file.close();
	
    return ga_obj.last_generation.chromosomes[ga_obj.last_generation.best_chromosome_index].genes.alphas;
}
