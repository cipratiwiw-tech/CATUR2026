#include "Vision.h"

std::vector<ChessSquare> Vision::createGrid(cv::Point topLeft, int width, int height) {
    std::vector<ChessSquare> grid;
    
    // Paksa ukuran total agar bisa dibagi 8 secara bulat sempurna (menghindari sisa piksel)
    int safeWidth = (width / 8) * 8;
    int safeHeight = (height / 8) * 8;

    double stepW = (double)safeWidth / 8.0; 
    double stepH = (double)safeHeight / 8.0;
    
    std::string files = "abcdefgh";
    std::string ranks = "87654321";

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            ChessSquare sq;
            sq.name = std::string(1, files[col]) + ranks[row];

            int x1 = (int)(topLeft.x + (col * stepW));
            int y1 = (int)(topLeft.y + (row * stepH));
            int x2 = (int)(topLeft.x + ((col + 1) * stepW));
            int y2 = (int)(topLeft.y + ((row + 1) * stepH));

            // PERBAIKAN: Potong 2 piksel dari setiap sisi ke arah dalam (Shrink)
            // Ini adalah pelindung anti-meleset agar tidak memotong garis tepi papan
            int margin = 2;
            sq.area = cv::Rect(x1 + margin, y1 + margin, (x2 - x1) - (margin * 2), (y2 - y1) - (margin * 2));
            
            grid.push_back(sq);
        }
    }
    return grid;
}