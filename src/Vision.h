#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

struct ChessSquare {
    std::string name; // "a1", "e4", dll
    cv::Rect area;    // Lokasi pikselnya di layar
};

// Moved from main.cpp to Vision.h and added 'gray' field for Dual Matching (Task 5)
struct PieceTemplate {
    std::string piece;
    cv::Mat img;
    cv::Mat gray;  // Ditambahkan untuk grayscale matching
    cv::Mat edges; 
};

class Vision {
public:
    std::vector<cv::Point> findTemplate(cv::Mat mainImage, cv::Mat templateImage, double threshold = 0.8, cv::Mat mask = cv::Mat());
    std::vector<ChessSquare> createGrid(cv::Point topLeft, int width, int height);
    
    // --- TASK 7: Refactor Logic Into Vision Module ---
    bool isEmpty(cv::Mat edges);
    bool isWhitePiece(cv::Mat squareImgBGR);
    double matchTemplateScore(cv::Mat squareGray, cv::Mat squareEdges, const PieceTemplate& t);
    std::string detectPiece(cv::Mat squareImg, const std::vector<PieceTemplate>& templates, double& outScore);
};