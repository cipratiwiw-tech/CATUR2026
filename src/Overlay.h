#pragma once
#include <windows.h>
#include <string>
#include <atomic> // Tambahkan ini

class Overlay {
public:
    Overlay();
    ~Overlay();
    // Ubah parameter create agar bisa dipanggil di dalam thread
    void start(const std::string& title, int w, int h); 
    void run();
    RECT getFrameRect();
    std::atomic<bool> isReady{ false }; // Flag biar main thread gak buru-buru

private:
    HWND hwnd;
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};