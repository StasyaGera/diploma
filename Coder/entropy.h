#pragma once

#include <vector>

double count_img_entropy(const image & img) {
    double res = 0;
    
    int freq[256];
    std::fill_n(freq, 256, 0);
    
    for (size_t y = 0; y < img.height; y++) {
        for (size_t x = 0; x < img.width; x++) {
            for (int col = 0; col < img.colors; col++) {
                freq[(int)img(x, y, col)]++;
            }
        }
    }

    for (size_t i = 0; i < 256; i++) {
        double p = (double)freq[i] / (double)(img.colors * img.width * img.height);
        res -= freq[i] == 0 ? 0 : p * std::log2(p);
    }
    
    return res;
}

double count_err_entropy(const image & img, const image & predicted) {
    double res = 0;

    int freq[511];
    std::fill_n(freq, 511, 0);

    for (size_t y = 2; y < img.height; y++) {
        for (size_t x = 2; x < img.width - 1; x++) {
            for (int col = 0; col < img.colors; col++) {
                freq[255 + ((short)img(x, y, col) - (short)predicted(x, y, col))]++;
            }
        }
    }

    double sz = (double)(img.colors * img.width * img.height);
    for (size_t i = 0; i < 511; i++) {
        double p = (double)freq[i] / sz;
        res -= freq[i] == 0 ? 0 : p * std::log2(p);
    }

    return res;
}
