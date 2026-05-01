#include <iostream>
#include <opencv2/opencv.hpp>
#include "ScreenCapture.h"

int main() {
    // Tambahkan baris ini untuk mematikan log INFO yang berisik
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

    ScreenCapture capturer;
    RECT rect;

    std::cout << "Mencari jendela browser..." << std::endl;

    // Coba cari jendela yang judulnya ada kata "Chess"
    // Kalau kamu pakai Lichess, bisa ganti jadi "Lichess"
    if (capturer.findWindowRect("Chess", rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // Ambil gambar tepat di jendela tersebut
        cv::Mat browserImg = capturer.captureRegion(rect.left, rect.top, width, height);

        if (!browserImg.empty()) {
            cv::imwrite("hasil_browser.jpg", browserImg);
            std::cout << "BERHASIL! Cek jendela pop-up." << std::endl;
            
            cv::imshow("Hasil Tangkapan Otomatis", browserImg);
            cv::waitKey(0); 
        }
    } else {
        std::cout << "ERROR: Jendela 'Chess' tidak ditemukan! Pastikan Chrome sudah terbuka." << std::endl;
    }

    return 0;
}