#include <iostream>
#include <opencv2/opencv.hpp>
#include "ScreenCapture.h"

int main() {
    std::cout << "Memulai modul Screen Capture..." << std::endl;

    // Menghidupkan alat rekam
    ScreenCapture capturer;

    // Mengambil gambar layar di kordinat X=0, Y=0 (Pojok kiri atas)
    // Dengan lebar 500 pixel dan tinggi 500 pixel
    cv::Mat screenshot = capturer.captureRegion(0, 0, 500, 500);

    // Memeriksa apakah gambar berhasil ditangkap
    if (screenshot.empty()) {
        std::cout << "Gagal mengambil gambar layar!" << std::endl;
        return -1;
    }

    // Menyimpan hasil tangkapan layar ke dalam file
    cv::imwrite("hasil_jepretan.jpg", screenshot);
    
    std::cout << "Sukses! Coba cek folder build kamu, ada file 'hasil_jepretan.jpg'!" << std::endl;

    return 0;
}