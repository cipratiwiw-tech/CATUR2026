#include "Vision.h"

std::vector<cv::Point> Vision::findTemplate(cv::Mat mainImage, cv::Mat templateImage, double threshold) {
    cv::Mat result;
    std::vector<cv::Point> foundPoints;

    // 1. Jalankan Template Matching
    cv::matchTemplate(mainImage, templateImage, result, cv::TM_CCOEFF_NORMED);

    // 2. Ambil semua kordinat yang kemiripannya tinggi
    for (int y = 0; y < result.rows; y++) {
        for (int x = 0; x < result.cols; x++) {
            if (result.at<float>(y, x) >= (float)threshold) {
                // Tambahkan hanya jika poin ini tidak terlalu dekat dengan poin yang sudah ada
                bool isDuplicate = false;
                for (const auto& pt : foundPoints) {
                    if (cv::norm(pt - cv::Point(x, y)) < 10) { // Jarak 10 pixel dianggap bidak yang sama
                        isDuplicate = true;
                        break;
                    }
                }
                if (!isDuplicate) {
                    foundPoints.push_back(cv::Point(x, y));
                }
            }
        }
    }
    return foundPoints;
}