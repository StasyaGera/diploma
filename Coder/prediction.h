#pragma once

#include <functional>

#include "image.h"
#include "entropy.h"
#include "default_constants.h"

using namespace std::placeholders;

unsigned char prev(const image & img, size_t x, size_t y, int col) {
    if (x == 0 && y == 0) return 128;
    if (x == 0) return img(x, y - 1, col);

    return img(x - 1, y, col);
}

unsigned char jpeg_ls(const image & img, size_t x, size_t y, int col) {
    if (x == 0 || y == 0) return prev(img, x, y, col);
    
    short a = img(x - 1, y, col);
    short b = img(x, y - 1, col);
    short c = img(x - 1, y - 1, col);

    if (c >= std::max(a, b)) return std::min(a, b);
    if (c <= std::min(a, b)) return std::max(a, b);
    return a + b - c;
}

unsigned char calic(const image& img, size_t x, size_t y, int col) {
    if (x < 2 || x == img.width - 1) {
		if (y != 0)	
			return img(x, y - 1, col);
		else 
			return 128;
	}
	if (y < 2) {
		if (x != 0)
			return img(x - 1, y, col);
		else
			return 128;
	}
    
    short w = img(x - 1, y, col);
    short ww = img(x - 2, y, col);
    short n = img(x, y - 1, col);
    short nn = img(x, y - 2, col);
    short nw = img(x - 1, y - 1, col);
    short ne = img(x + 1, y - 1, col);
    short nne = img(x + 1, y - 2, col);

    short dv = std::abs(w - nw) + std::abs(n - nn) + std::abs(ne - nne);
    short dh = std::abs(w - ww) + std::abs(n - nw) + std::abs(n - ne);
	short D = dv - dh;
	
    if (D > 80) {
        return (unsigned char)w;
    } 
    else if (D < -80) {
        return (unsigned char)n;
    }
    else {
        short res = (short)((w + n) / 2 + (ne - nw) / 4);
        if (D > 32) {
            return (res + w) / 2;
        }
        else if (D > 8) {
            return (3 * res + w) / 4;
        }
        else if (D < -32) {
            return (res + n) / 2;
        }
        else if (D < -8) {
            return (3 * res + n) / 4;
        }
        return res;
    }
}

unsigned char int_to_pix(const image& img, int x, int y, int col, int val) {
	switch (val) {
	case 0: // a
		return img(x - 1, y, col);
	case 1: // b
		return img(x - 1, y - 1, col);
	case 2: // c
		return img(x, y - 1, col);
	case 3: // d
		return img(x + 1, y - 1, col);
	case 4: // e
		return img(x - 2, y, col);
	case 5: // f
		return img(x, y - 2, col);
	case 6: // g
		return img(x + 1, y - 2, col);
	case 7: // h
		return img(x - 2, y - 1, col);
	}
}

int get_ctx_tree(const image& img, size_t x, size_t y, int col, const std::vector<int>& context) {
	auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
	int k = -1;
	if (std::abs(get_pix_val(context[0]) - get_pix_val(context[1])) > context[2]) {
		if (std::abs(get_pix_val(context[3]) - get_pix_val(context[4])) > context[5]) {
			if (std::abs(get_pix_val(context[9]) - get_pix_val(context[10])) > context[11]) {
				k = 0;
			}
			else {
				k = 1;
			}
		}
		else {
			if (std::abs(get_pix_val(context[12]) - get_pix_val(context[13])) > context[14]) {
				k = 2;
			}
			else {
				k = 3;
			}
		}
	}
	else {
		if (std::abs(get_pix_val(context[6]) - get_pix_val(context[7])) > context[8]) {
			if (std::abs(get_pix_val(context[15]) - get_pix_val(context[16])) > context[17]) {
				k = 4;
			}
			else {
				k = 5;
			}
		}
		else {
			if (std::abs(get_pix_val(context[18]) - get_pix_val(context[19])) > context[20]) {
				k = 6;
			}
			else {
				k = 7;
			}
		}
	}
	return k;
}

int get_ctx_calic(const image& img, size_t x, size_t y, int col, const std::vector<int>& context = calic_contexts) {
	auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
	short dv =
		std::abs(get_pix_val(context[0]) - get_pix_val(context[1])) +
		std::abs(get_pix_val(context[2]) - get_pix_val(context[3])) +
		std::abs(get_pix_val(context[4]) - get_pix_val(context[5]));
	short dh =
		std::abs(get_pix_val(context[6]) - get_pix_val(context[7])) +
		std::abs(get_pix_val(context[8]) - get_pix_val(context[9])) +
		std::abs(get_pix_val(context[10]) - get_pix_val(context[11]));
	short D = dv - dh;

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
	return k;
}

unsigned char evolvable(const image& img, size_t x, size_t y, int col, 
	const std::vector<std::vector<int>>& coeffs,
	const std::vector<int>& context,
	std::function<int(const image&, size_t, size_t, int, const std::vector<int>&)> get_ctx) 
{
	if (x < 2 || x == img.width - 1) {
		if (y != 0)
			return img(x, y - 1, col);
		else
			return 128;
	}
	if (y < 2) {
		if (x != 0)
			return img(x - 1, y, col);
		else
			return 128;
	}

	int k = get_ctx(img, x, y, col, context);
	// E0 P0 C0 E1 P1 C1 E2 P2 C2 E3 P3 C3
	int res1 = 0, res2 = 0;
	for (size_t i = 0; i < 4; i++) {
		int pix = int_to_pix(img, x, y, col, coeffs[k][3 * i + 1]);
		res1 += coeffs[k][3 * i] * pix * coeffs[k][3 * i + 2];
		res2 += coeffs[k][3 * i] * coeffs[k][3 * i + 2];
	}
	return res2 == 0 ? 0 : res1 / res2;
}

double get_err_entropy(const image & img, std::function<unsigned char(const image &, size_t, size_t, int)> p) {
    image res(img);
	for (size_t y = 0; y < img.height; y++) {
		for (size_t x = 0; x < img.width; x++) {
			for (int col = 0; col < img.colors; col++) {
				res.set(x, y, col, p(img, x, y, col));
			}
		}
	}
    return count_err_entropy(img, res);
}
