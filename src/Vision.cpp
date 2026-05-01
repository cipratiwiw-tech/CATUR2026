#include "Vision.h"

std::vector<ChessSquare> Vision::createGrid(cv::Point topLeft, int boardSize) {
    std::vector<ChessSquare> grid;
    double step = (double)boardSize / 8.0; // Gunakan double agar presisi
    
    std::string files = "abcdefgh";
    std::string ranks = "87654321";

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            ChessSquare sq;
            sq.name = std::string(1, files[col]) + ranks[row];

            // Hitung koordinat X dan Y dengan double sebelum dibulatkan ke int
            int x1 = (int)(topLeft.x + (col * step));
            int y1 = (int)(topLeft.y + (row * step));
            
            // Hitung koordinat pojok kanan bawah kotak
            int x2 = (int)(topLeft.x + ((col + 1) * step));
            int y2 = (int)(topLeft.y + ((row + 1) * step));

            sq.area = cv::Rect(x1, y1, x2 - x1, y2 - y1);
            grid.push_back(sq);
        }
    }
    return grid;
}