#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

class Vision {
public:
    // Fungsi untuk mencari koordinat template di dalam gambar utama
    // threshold = tingkat kemiripan (0.0 sampai 1.0)
    std::vector<cv::Point> findTemplate(cv::Mat mainImage, cv::Mat templateImage, double threshold = 0.8);
};