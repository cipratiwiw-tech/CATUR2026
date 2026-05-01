#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <thread>
#include <opencv2/opencv.hpp>
#include "Overlay.h"
#include "ScreenCapture.h"
#include "Vision.h"

// Fungsi ini sekarang yang bertanggung jawab membuat DAN menjalankan jendela
void overlayWorker(Overlay* frame) {
    frame->start("SERET KE PAPAN CATUR", 450, 450);
}

int main() {
    SetProcessDPIAware();
    
    Overlay frame;
    ScreenCapture capturer;
    Vision vision;

    // Jalankan thread jendela
    std::thread t(overlayWorker, &frame);
    t.detach();

    // Tunggu sampai jendela benar-benar muncul sebelum lanjut
    while (!frame.isReady) { Sleep(10); }

    std::cout << "===========================================" << std::endl;
    std::cout << "      CHESS BOT MANUAL OVERLAY MODE        " << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << "1. Geser & Resize kotak merah ke papan catur" << std::endl;
    std::cout << "2. Tekan ENTER di sini untuk SCAN..." << std::endl;

    while (true) {
        std::string dummy;
        std::getline(std::cin, dummy); 

        RECT target = frame.getFrameRect();
        int w = target.right - target.left;
        int h = target.bottom - target.top;

        if (w <= 0 || h <= 0) continue;

        cv::Mat boardImg = capturer.captureRegion(target.left, target.top, w, h);
        
        if (!boardImg.empty()) {
            cv::cvtColor(boardImg, boardImg, cv::COLOR_BGRA2BGR);
            
            int boardSize = std::min(w, h);
            // Gunakan titik 0,0 karena kita sudah memotong tepat di area papan[cite: 6]
            std::vector<ChessSquare> grid = vision.createGrid(cv::Point(0,0), boardSize);

            for (const auto& sq : grid) {
                cv::rectangle(boardImg, sq.area, cv::Scalar(0, 255, 0), 1);
            }

            cv::imshow("HASIL SCAN", boardImg);
            cv::waitKey(1); 
            std::cout << "Scan Berhasil di " << w << "x" << h << " px! Tekan ENTER lagi..." << std::endl;
        }
    }
    return 0;
}