#pragma once
#include <windows.h>
#include <opencv2/opencv.hpp>

class ScreenCapture {
public:
    ScreenCapture();
    ~ScreenCapture();
    
    // Fungsi utama: mengambil gambar layar pada kordinat tertentu
    // x, y = posisi pojok kiri atas
    // width, height = lebar dan tinggi area yang mau direkam
    cv::Mat captureRegion(int x, int y, int width, int height);

private:
    HWND hwndDesktop;
};