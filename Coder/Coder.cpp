#include <iostream>
#include <filesystem>
#include <vector>

#include "gnuplot_i.hpp"

#include "table_printer.h"
#include "default_constants.h"
#include "image.h"
#include "entropy.h"
#include "prediction.h"
#include "ga_settings.h"

const char * sayood_dir = "sayood_imgs";
const char * test_dir = "test_imgs";
const char * test_grey_512 = "test_grey_512x512";
const char * test_color_512 = "test_color_512x512";

const int colors = 3;

void exec(const char * dir) {
    print_header();

    for (auto& p : std::filesystem::directory_iterator(dir)) {
        image img(p.path().string().c_str(), colors);
        
        double img_entropy = count_img_entropy(img);
        double prev_entropy = get_err_entropy(img, prev);
        double jpeg_ls_entropy = get_err_entropy(img, jpeg_ls);
        //double calic_entropy = get_err_entropy(img, calic);
        
		auto weights = generate_weights(img, p.path().filename().string(), img_entropy);
        double genetic_entropy = get_err_entropy(img, bind_genetic(weights));
        
		//std::vector<std::vector<short>> jpeg_ls_emulation = std::vector<std::vector<short>>({ {15, 0, 0}, {1, 1, 1}, {0, 15, 0}, {0, 15, 0}, {1, 1, 1}, {15, 0, 0} });
		//double jpeg_ls_emulated_entropy = get_err_entropy(img, genetic, jpeg_ls_emulation);

        print_row(p.path().filename(), img_entropy, prev_entropy, jpeg_ls_entropy/*, calic_entropy*/, genetic_entropy);
    }

    print_bottom_line();
}

void plot_frequency(const char * dir) {
    for (auto& p : std::filesystem::directory_iterator(dir)) {
        image img(p.path().string().c_str(), colors);

        std::vector<double> values(256, 0);
        for (int h = 0; h < img.height; h++) {
            for (int w = 0; w < img.width; w++) {
                values[img(w, h, 0)]++;
            }
        }
		std::cout << p.path().filename().string() + " entopy = " << count_img_entropy(img) << '\n';

		image res(img);
		std::vector<double> err(512, 0);
		std::vector<double> brightness(512, 0);
		std::iota(brightness.begin(), brightness.end(), -255);
		for (size_t y = 0; y < img.height; y++) {
			for (size_t x = 0; x < img.width; x++) {
				for (int col = 0; col < img.colors; col++) {
					res.set(x, y, col, prev(img, x, y, col));
					err[255 + (int)(img(x, y, col) - res(x, y, col))]++;
				}
			}
		}
		std::cout << p.path().filename().string() + " err_entropy = " << count_err_entropy(img, res) << '\n';

		try {
			Gnuplot plotter("lines");
			plotter
				.set_grid()
				.set_title("Для изображения " + p.path().filename().string());

			plotter
				.set_xlabel("Яркость пикселя")
				.set_ylabel("Количество пикселей")
				.set_xrange(0, 255)
				.savetofigure("graph/frequencies_" + p.path().filename().string() + ".png", "pngcairo size 640, 480 enhanced font 'Verdana,9'")
				.plot_x(values);

			plotter
				.reset_plot()
				.set_xlabel("Значение ошибки предсказания")
				.set_ylabel("Количество пикселей")
				.set_xrange(-255, 255)
				.savetofigure("graph/err_frequencies_" + p.path().filename().string() + ".png", "pngcairo size 640, 480 enhanced font 'Verdana,9'")
				.plot_xy(brightness, err);
		}
		catch (GnuplotException ge) {
			std::cout << ge.what() << '\n';
		}
    }
}

void plot_err_by_pop_sz(const char * dir, int max_population = def_population_sz) {
    for (auto& p : std::filesystem::directory_iterator(dir)) {
        image img(p.path().string().c_str(), colors);

        double img_entropy = count_img_entropy(img);

        std::vector<double> population_sz;
        std::vector<double> err_entr;

		population_sz.reserve(100);
        err_entr.reserve(100);

		int population = 20;
        while (population < max_population) {
            population_sz.push_back(population);
			auto weights = generate_weights(img, p.path().filename().string(), img_entropy, population);
            err_entr.push_back(get_err_entropy(img, bind_genetic(weights)));

			if (population >= 1000) {
                population += 500;
            }
            if (population >= 500) {
                population += 100;
            }
            else if (population >= 100) {
                population += 50;
            }
            else {
                population += 20;
            }
        }

		try {
			Gnuplot plotter;
			plotter
				.set_grid()
				.set_title("Для изображения " + p.path().filename().string())
				.set_xlabel("Размер популяции")
				.set_ylabel("Энтропия ошибки");

			plotter
				.set_style("points")
				.savetofigure("graph/err_by_pop_sz_" + p.path().filename().string() + ".png", "pngcairo size 640, 480 enhanced font 'Verdana,9'")
				.plot_xy(population_sz, err_entr);
		}
		catch (GnuplotException ge) {
			std::cout << ge.what() << '\n';
		}
    }
}

void plot_err_by_gen_no(const char* dir, int population = def_population_sz) {
	for (auto& p : std::filesystem::directory_iterator(dir)) {
		image img(p.path().string().c_str(), colors);

		double img_entropy = count_img_entropy(img);
		double jpeg_entropy = get_err_entropy(img, jpeg_ls);

		auto weights = generate_weights(img, p.path().filename().string(), img_entropy, population);
		weights = generate_weights(img, p.path().filename().string(), img_entropy, population, false);

		try {
			Gnuplot plotter;
			plotter
				.set_grid()
				.set_title("Для изображения " + p.path().filename().string())
				.set_xlabel("Номер поколения")
				.set_ylabel("Энтропия ошибки");
			
			plotter
				.set_style("lines")
				.savetofigure("graph/err_by_gen_no_" + p.path().filename().string() + ".png", "pngcairo size 640, 480 enhanced font 'Verdana,9'")
				.plot_slope(0, jpeg_entropy, "JPEG-LS");

			plotter
				.set_style("points")
				.savetofigure("graph/err_by_gen_no_" + p.path().filename().string() + ".png", "pngcairo size 640, 480 enhanced font 'Verdana,9'")
				.plotfile_xy(def_err_gen_by_gen_filename(p.path().filename().string(), population, true), 1, 2, "ГА, иниц. части особей JPEG-LS");

			plotter
				.set_style("points")
				.savetofigure("graph/err_by_gen_no_" + p.path().filename().string() + ".png", "pngcairo size 640, 480 enhanced font 'Verdana,9'")
				.plotfile_xy(def_err_gen_by_gen_filename(p.path().filename().string(), population, false), 1, 2, "ГА, иниц. случ. знач.");
		}
		catch (GnuplotException ge) {
			std::cout << ge.what() << '\n';
		}
	}
}


int main() {
    //exec(test_grey_512);
    //exec(test_color_512);
    //exec(sayood_dir);
    //exec("med-test");
    //exec("lena-test");
    //exec("screenshot-test");

	//plot_err_by_pop_sz("lena-test", 200);
	//plot_err_by_gen_no("dollar-test");
	//plot_err_by_pop_sz(test_color_512, 1000);
	//plot_err_by_gen_no(test_grey_512, 500);
	//plot_err_by_gen_no(test_color_512, 500);
	//plot_err_by_gen_no(sayood_dir, 500);
	
    //plot_frequency(sayood_dir);
    //plot_frequency("lena-test");

    return 0;
}
