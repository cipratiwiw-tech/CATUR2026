#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct ChessSquare {
    std::string name; // "a1", "e4", dll
    cv::Rect area;    // Lokasi pikselnya di layar
};

class Vision {
public:
    std::vector<cv::Point> findTemplate(cv::Mat mainImage, cv::Mat templateImage, double threshold = 0.8, cv::Mat mask = cv::Mat());
    
    // Fungsi baru untuk membagi papan jadi 64 kotak
    std::vector<ChessSquare> createGrid(cv::Point topLeft, int boardSize);
};