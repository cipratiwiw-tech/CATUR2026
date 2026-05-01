#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/utils/logger.hpp>
#include "ScreenCapture.h"
#include "Vision.h"

int main() {
    // --- 1. SAKLAR DPI AWARENESS (PENTING!) ---
    // Ini agar bot bisa melihat ukuran jendela Chrome yang sebenarnya (Piksel Asli)
    SetProcessDPIAware();

    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

    ScreenCapture capturer;
    RECT rect;

    std::cout << "Mencari jendela browser..." << std::endl;

    // Setiap kali run, fungsi ini akan mencari ulang jendela & ukurannya
    if (capturer.findWindowRect("Chess", rect)) {
        
        // Hitung lebar dan tinggi SECARA DINAMIS
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // Cetak ke terminal biar kamu bisa pantau
        std::cout << "--- DETEKSI ULANG BERHASIL ---" << std::endl;
        std::cout << "Lebar: " << width << " px" << std::endl;
        std::cout << "Tinggi: " << height << " px" << std::endl;
        std::cout << "------------------------------" << std::endl;

        // Ambil jepretan sesuai ukuran yang baru saja dideteksi
        cv::Mat browserImg = capturer.captureRegion(rect.left, rect.top, width, height);

        if (!browserImg.empty()) {
            // Ubah ke BGR (3 channel) agar bisa diproses OpenCV
            cv::cvtColor(browserImg, browserImg, cv::COLOR_BGRA2BGR);
            
            // Simpan dan beri tahu lokasi filenya
            cv::imwrite("hasil_browser.jpg", browserImg);
            
            std::cout << "Foto disimpan! Silakan cek hasil_browser.jpg" << std::endl;
            
            cv::imshow("Monitor Bot (Tekan apa saja untuk tutup)", browserImg);
            cv::waitKey(0);
            cv::destroyAllWindows();
        }
    } else {
        std::cout << "Jendela 'Chess' tidak ditemukan! Pastikan Chrome sudah terbuka." << std::endl;
    }

    return 0;
}