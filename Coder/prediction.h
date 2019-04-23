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

    if (c >= std::max(a, b)) return std::min(a, b);
    if (c <= std::min(a, b)) return std::max(a, b);
    return a + b - c;
}

unsigned char calic(const image & img, size_t x, size_t y, int col) {
    if (x == 0 && y == 0) return 128;
    if (x < 2) return img(x, y - 1, col);
    if (y < 2) return img(x - 1, y, col);
    if (x == img.width - 1) return jpeg_ls(img, x, y, col);

    //if (x < 2 || y < 2 || x == img.width - 1) return jpeg_ls(img, x, y, col);
    
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
        return w;
    } 
    else if (D < -80) {
        return n;
    }
    else {
        short res = (w + n) / 2 + (ne - nw) / 4;
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

    return (alpha_a + alpha_b + alpha_c == 0) ? 0 : (alpha_a * a + alpha_b * b + alpha_c * c) / (alpha_a + alpha_b + alpha_c);
	// проверить округление
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
