#include <iostream>
#include <filesystem>
#include <vector>
#include <numeric>

#include "gnuplot_i.hpp"

#include "table_printer.h"
#include "default_constants.h"
#include "image.h"
#include "entropy.h"
#include "prediction.h"
#include "evolvable_predictor.h"
#include "tree_context_ep.h"
#include "calic_context_ep.h"

const int colors = 1;

void exec(const char * dir) {
	std::cout << dir << '\n';
	print_header({ "entropy", "jpeg-ls", "calic", "ep", "context", "tree" });
    for (auto& p : std::filesystem::directory_iterator(dir)) {
        image img(p.path().string().c_str(), colors);
        
        double img_entropy = count_img_entropy(img);
        double jpeg_ls_entropy = get_err_entropy(img, jpeg_ls);
        double calic_entropy = get_err_entropy(img, calic);
        
		std::string img_name = p.path().filename().string();
		double evolvable_entropy = evolvable_predictor(img, img_name, 20, 5, 10);
		double evolvable_calic_entropy = evolvable_calic_context(img, img_name, 10, 5, 5, 5, 10);
		double evolvable_tree_entropy = evolvable_tree_context(img, img_name, 10, 5, 5, 5, 10);
		              
		print_row(p.path().filename(), { img_entropy, jpeg_ls_entropy, calic_entropy, evolvable_entropy, evolvable_calic_entropy, evolvable_tree_entropy });
    }
    print_bottom_line();
}

void plot_err_by_gen_no(const char* dir, int context_population, int pred_population, int generations) {
	for (auto& p : std::filesystem::directory_iterator(dir)) {
		image img(p.path().string().c_str(), colors);

		double img_entropy = count_img_entropy(img);
		double jpeg_ls_entropy = get_err_entropy(img, jpeg_ls);
		double calic_entropy = get_err_entropy(img, calic);

		std::string img_name = p.path().filename().string();
		
		double evolvable_entropy = evolvable_predictor(img, img_name, 20, 5, generations);
		double evolvable_calic_entropy = evolvable_calic_context(img, img_name, context_population, 5, pred_population, pred_population, generations);		
		double evolvable_tree_entropy = evolvable_tree_context(img, img_name, context_population, 5, pred_population, pred_population, generations);

		std::string graph_name = def_graph_filename(img_name, context_population, pred_population, generations);
		std::string graph_settings("pngcairo size 640, 480 enhanced font 'Verdana,9'");

		try {
			Gnuplot plotter;
			plotter
				.set_grid()
				.unset_legend()
				.set_title("Для изображения " + img_name)
				.set_xlabel("Номер поколения")
				.set_ylabel("Энтропия ошибки");

			plotter
				.cmd("set style line 1 lt 2 lw 2")
				.set_style("lines ls 1")
				.savetofigure(graph_name, graph_settings)
				.plot_slope(0, jpeg_ls_entropy, "JPEG-LS");

			plotter
				.cmd("set style line 2 lt 3 lw 2")
				.set_style("lines ls 2")
				.savetofigure(graph_name, graph_settings)
				.plot_slope(0, calic_entropy, "CALIC");

			plotter
				.cmd("set style line 3 lc rgb \"#FF8000\" pt 5 lw 2")
				.set_style("points ls 3")
				.savetofigure(graph_name, graph_settings)
				.plotfile_xy(def_filename("ep", img_name, 0, 20, generations), 1, 2, "EP");

			plotter
				.cmd("set style line 4 lc rgb \"#B266FF\" pt 9 lw 2")
				.set_style("points ls 4")
				.savetofigure(graph_name, graph_settings)
				.plotfile_xy(def_filename("tree", img_name, context_population, pred_population, generations), 1, 2, "EP, tree");

			plotter
				.cmd("set style line 5 lc rgb \"#FF007F\" pt 7 lw 2")
				.set_style("points ls 5")
				.savetofigure(graph_name, graph_settings)
				.plotfile_xy(def_filename("calic", img_name, context_population, pred_population, generations), 1, 2, "EP, CALIC");
		}
		catch (GnuplotException ge) {
			std::cout << ge.what() << '\n';
		}
	}
}

void convert_to_grey(const char* dir, const char* res_dir) {
	for (auto& p : std::filesystem::directory_iterator(dir)) {
		image img(p.path().string().c_str(), colors);
		image res = img.to_grey();
		res.write_to_file((std::string(res_dir) + "\\" + p.path().filename().string()).c_str());
	}
}

int main() {	
	exec("aerials");
	exec("artificial");
	exec("objects");
	exec("space");
	exec("people");
	exec("textures");
	exec("medical");
	exec("test_grey_256x256");
	exec("sayood_imgs");
    
    return 0;
}
