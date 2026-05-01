#include "Vision.h"
#include <cmath> // std::isnan

std::vector<ChessSquare> Vision::createGrid(cv::Point topLeft, int width, int height) {
    std::vector<ChessSquare> grid;
    double stepW = (double)width / 8.0; // Gunakan double agar presisi
    double stepH = (double)height / 8.0;
    
    std::string files = "abcdefgh";
    std::string ranks = "87654321";

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            ChessSquare sq;
            sq.name = std::string(1, files[col]) + ranks[row];

            // Hitung koordinat X dan Y dengan double sebelum dibulatkan ke int
            int x1 = (int)(topLeft.x + (col * stepW));
            int y1 = (int)(topLeft.y + (row * stepH));
            
            // Hitung koordinat pojok kanan bawah kotak
            int x2 = (int)(topLeft.x + ((col + 1) * stepW));
            int y2 = (int)(topLeft.y + ((row + 1) * stepH));

            sq.area = cv::Rect(x1, y1, x2 - x1, y2 - y1);
            grid.push_back(sq);
        }
    }
    return grid;
}

// --- TASK 3: Improve Empty Detection (Abaikan Garis Pinggir) ---
bool Vision::isEmpty(cv::Mat edges) {
    // [FIX CRITICAL] Abaikan pinggiran kotak (garis highlight hijau/kuning) 
    // dengan hanya mengambil 60% area tengah saja
    int marginX = static_cast<int>(edges.cols * 0.20);
    int marginY = static_cast<int>(edges.rows * 0.20);
    cv::Rect centerCrop(marginX, marginY, edges.cols - (2 * marginX), edges.rows - (2 * marginY));
    cv::Mat centerEdges = edges(centerCrop);

    // Jika di tengah kotak tidak ada garis sama sekali (atau sangat sedikit), pasti kosong
    int edgePixels = cv::countNonZero(centerEdges);
    return edgePixels < 25; // Threshold ketat, tengah kotak harus bersih
}

// --- TASK 4: Improve Color Detection (Lebih Akurat untuk Kartun) ---
bool Vision::isWhitePiece(cv::Mat squareImgBGR) {
    // [FIX] Ambil area tengah saja untuk menghindari warna background papan
    int w = squareImgBGR.cols / 4;
    int h = squareImgBGR.rows / 4;
    cv::Rect centerRegion(squareImgBGR.cols/2 - w/2, squareImgBGR.rows/2 - h/2, w, h);
    cv::Mat centerImg = squareImgBGR(centerRegion);

    cv::Mat gray;
    cv::cvtColor(centerImg, gray, cv::COLOR_BGR2GRAY);
    cv::Scalar meanGray = cv::mean(gray);

    // Bidak putih kartun sangat terang (> 200)
    // Bidak hitam abu-abu gelap (< 100)
    // Background hijau/krem ada di tengah-tengah. Threshold 160 paling aman.
    return meanGray[0] > 160;
}

// --- TASK 5: Revert to Pure Edge Matching (Background Invariant) ---
double Vision::matchTemplateScore(cv::Mat squareGray, cv::Mat squareEdges, const PieceTemplate& t) {
    // [FIX CRITICAL] HANYA gunakan Edge (Siluet). 
    // Grayscale ikut mencocokkan warna papan dan bikin skor halusinasi jadi 0.90+
    cv::Mat resizedEdges;
    if (squareEdges.size() != t.edges.size()) {
        cv::resize(squareEdges, resizedEdges, t.edges.size());
    } else {
        resizedEdges = squareEdges;
    }

    int marginX = static_cast<int>(t.img.cols * 0.15); 
    int marginY = static_cast<int>(t.img.rows * 0.15);
    cv::Rect cropRegion(marginX, marginY, t.img.cols - (2 * marginX), t.img.rows - (2 * marginY));
    
    cv::Mat croppedEdges = t.edges(cropRegion);

    cv::Mat resultEdge;
    cv::matchTemplate(resizedEdges, croppedEdges, resultEdge, cv::TM_CCOEFF_NORMED);
    double minE, maxEdge;
    cv::minMaxLoc(resultEdge, &minE, &maxEdge);
    
    if (std::isnan(maxEdge)) maxEdge = 0;

    return maxEdge; // 100% Mengandalkan Bentuk Garis, kebal terhadap warna background!
}

// --- TASK 7: Detection Logic Orchestration ---
std::string Vision::detectPiece(cv::Mat squareImg, const std::vector<PieceTemplate>& templates, double& outScore) {
    cv::Mat squareImgBGR;
    if (squareImg.channels() == 4) {
        cv::cvtColor(squareImg, squareImgBGR, cv::COLOR_BGRA2BGR);
    } else {
        squareImgBGR = squareImg;
    }

    cv::Mat squareGray, squareEdges;
    cv::cvtColor(squareImgBGR, squareGray, cv::COLOR_BGR2GRAY);
    
    // Sedikit blur agar noise tekstur papan hilang, tapi garis bidak tetap tegas
    cv::GaussianBlur(squareGray, squareGray, cv::Size(3, 3), 0);
    cv::Canny(squareGray, squareEdges, 40, 120);

    if (isEmpty(squareEdges)) {
        outScore = 1.0; 
        return ".";
    }

    bool isLivePieceWhite = isWhitePiece(squareImgBGR);
    std::string detectedPiece = ".";
    
    double bestScore = 0.40; // Batas minimal toleransi bentuk

    for (const auto& t : templates) {
        if (t.piece == "E") continue;

        double finalScore = matchTemplateScore(squareGray, squareEdges, t);

        // Validasi Warna
        bool isTemplateWhite = isupper(t.piece[0]);
        if (isTemplateWhite != isLivePieceWhite) {
            finalScore -= 0.50; // Penalti mematikan jika warnanya kebalik
        }

        if (finalScore > bestScore) {
            bestScore = finalScore;
            detectedPiece = t.piece;
        }
    }
    
    outScore = bestScore;
    return detectedPiece;
}