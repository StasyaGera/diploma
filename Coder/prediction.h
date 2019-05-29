#pragma once

#include <functional>

#include "entropy.h"

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

    if (c >= std::max(a, b)) return (unsigned char)std::min(a, b);
    if (c <= std::min(a, b)) return (unsigned char)std::max(a, b);
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
        short res = (short)((double)(w + n) / 2 + (double)(ne - nw) / 4);
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

unsigned char genetic(const image & img, size_t x, size_t y, int col, const std::vector<std::vector<short>>& alphas) {
    if (x == 0 || y == 0) return prev(img, x, y, col);

    short a = img(x - 1, y, col);
    short b = img(x, y - 1, col);
    short c = img(x - 1, y - 1, col);

    int i = 0;
    if (a > c) {
        if (a > b) {
            if (b > c) {
                i = 0;
            }
            else {
                i = 1;
            }
        }
        else {
            i = 2;
        }
    }
    else {
        if (a > b) {
            i = 3;
        }
        else {
            if (b > c) {
                i = 4;
            }
            else {
                i = 5;
            }
        }
    }
    short alpha_a = alphas[i][0];
    short alpha_b = alphas[i][1];
    short alpha_c = alphas[i][2];

	short result = (alpha_a + alpha_b + alpha_c == 0) ? 0 : (alpha_a * a + alpha_b * b + alpha_c * c) / (alpha_a + alpha_b + alpha_c);
	if (result < 0) return 0;
	if (result > 255) return 255;
	return (unsigned char)result;
}

inline int get_ctx_no(int D) {
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

inline unsigned char int_to_pix(const image& img, int x, int y, int col, int val) {
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

unsigned char sayood(const image& img, size_t x, size_t y, int col, const std::vector<std::vector<int>>& coeffs) {
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

	short a = img(x - 1, y, col);
	short b = img(x - 1, y - 1, col);
	short c = img(x, y - 1, col);
	short d = img(x + 1, y - 1, col);
	short e = img(x - 2, y, col);
	short f = img(x, y - 2, col);
	short g = img(x + 1, y - 2, col);
	short h = img(x - 2, y - 1, col);

	short dv = std::abs(a - b) + std::abs(c - f) + std::abs(d - g);
	short dh = std::abs(a - e) + std::abs(c - b) + std::abs(d - c);
	short D = dv - dh;

	int k = get_ctx_no(D);

	// E0 E1 E2 E3 C0 C1 C2 C3 P0 P1 P2 P3
	long long res1 = 0, res2 = 0;
	for (size_t i = 0; i < 4; i++) {
		unsigned char pix = int_to_pix(img, x, y, col, coeffs[k][i + 4]);
		res1 += coeffs[k][i] * pix * coeffs[k][i + 8];
		res2 += coeffs[k][i] * coeffs[k][i + 8];
	}

	return res2 == 0 ? 0 : res1 / res2;
}

unsigned char sayood2(const image& img, size_t x, size_t y, int col, const std::vector<int>& context_consts, const std::vector<std::vector<int>>& coeffs) {
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
	auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);
	short dv = std::abs(get_pix_val(context_consts[0]) - get_pix_val(context_consts[1])) 
		+ std::abs(get_pix_val(context_consts[2]) - get_pix_val(context_consts[3])) 
		+ std::abs(get_pix_val(context_consts[4]) - get_pix_val(context_consts[5]));
	short dh = std::abs(get_pix_val(context_consts[6]) - get_pix_val(context_consts[7])) 
		+ std::abs(get_pix_val(context_consts[8]) - get_pix_val(context_consts[9])) 
		+ std::abs(get_pix_val(context_consts[10]) - get_pix_val(context_consts[11]));
	short D = dv - dh;

	int k = get_ctx_no(D);

	// E0 E1 E2 E3 C0 C1 C2 C3 P0 P1 P2 P3
	long long res1 = 0, res2 = 0;
	for (size_t i = 0; i < 4; i++) {

		unsigned char pix = int_to_pix(img, x, y, col, coeffs[k][i + 4]);
		res1 += coeffs[k][i] * pix * coeffs[k][i + 8];
		res2 += coeffs[k][i] * coeffs[k][i + 8];
	}

	return res2 == 0 ? 0 : res1 / res2;
}

unsigned char sayood3(const image& img, size_t x, size_t y, int col, const std::vector<int>& context_consts, const std::vector<std::vector<int>>& coeffs) {
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
	auto get_pix_val = std::bind(int_to_pix, img, x, y, col, _1);

	int k = -1;
	if (std::abs(get_pix_val(context_consts[0]) - get_pix_val(context_consts[1])) > context_consts[2]) {
		if (std::abs(get_pix_val(context_consts[3]) - get_pix_val(context_consts[4])) > context_consts[5]) {
			if (std::abs(get_pix_val(context_consts[9]) - get_pix_val(context_consts[10])) > context_consts[11]) {
				k = 0;
			}
			else {
				k = 1;
			}
		}
		else {
			if (std::abs(get_pix_val(context_consts[12]) - get_pix_val(context_consts[13])) > context_consts[14]) {
				k = 2;
			}
			else {
				k = 3;
			}
		}
	}
	else {
		if (std::abs(get_pix_val(context_consts[6]) - get_pix_val(context_consts[7])) > context_consts[8]) {
			if (std::abs(get_pix_val(context_consts[15]) - get_pix_val(context_consts[16])) > context_consts[17]) {
				k = 4;
			}
			else {
				k = 5;
			}
		}
		else {
			if (std::abs(get_pix_val(context_consts[18]) - get_pix_val(context_consts[19])) > context_consts[20]) {
				k = 6;
			}
			else {
				k = 7;
			}
		}
	}

	// E0 E1 E2 E3 C0 C1 C2 C3 P0 P1 P2 P3
	long long res1 = 0, res2 = 0;
	for (size_t i = 0; i < 4; i++) {
		unsigned char pix = 0;
		switch (coeffs[k][i + 4]) {
		case 0: // a
			pix = img(x - 1, y, col);
			break;
		case 1: // b
			pix = img(x - 1, y - 1, col);
			break;
		case 2: // c
			pix = img(x, y - 1, col);
			break;
		case 3: // d
			pix = img(x + 1, y - 1, col);
			break;
		case 4: // e
			pix = img(x - 2, y, col);
			break;
		case 5: // f
			pix = img(x, y - 2, col);
			break;
		case 6: // g
			pix = img(x + 1, y - 2, col);
			break;
		case 7: // h
			pix = img(x - 2, y - 1, col);
			break;
		}
		res1 += coeffs[k][i] * pix * coeffs[k][i + 8];
		res2 += coeffs[k][i] * coeffs[k][i + 8];
	}

	return res2 == 0 ? 0 : res1 / res2;
}

std::function<unsigned char(const image&, size_t, size_t, int)> bind_genetic(const std::vector<std::vector<short>>& alphas) {
	return std::bind(genetic, _1, _2, _3, _4, alphas);
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

template <typename Chromo>
double get_err_entropy(const image& img, std::function<unsigned char(const image&, size_t, size_t, int, Chromo)> p, Chromo chromosome) {
	image res(img);

	for (size_t y = 0; y < img.height; y++) {
		for (size_t x = 0; x < img.width; x++) {
			for (int col = 0; col < img.colors; col++) {
				res.set(x, y, col, p(img, x, y, col, chromosome));
			}
		}
	}

	return count_err_entropy(img, res);
}

double get_sayood2_err_entropy(const image& img, const std::vector<int>& context_consts, const std::vector<std::vector<int>>& coeffs) {
	image res(img);

	for (size_t y = 0; y < img.height; y++) {
		for (size_t x = 0; x < img.width; x++) {
			for (int col = 0; col < img.colors; col++) {
				res.set(x, y, col, sayood2(img, x, y, col, context_consts, coeffs));
			}
		}
	}

	return count_err_entropy(img, res);
}
