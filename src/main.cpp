#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include "Overlay.h"

int main() {
    SetProcessDPIAware();
    Overlay frame;
    
    std::thread t([&]() { frame.start("EDITOR BIDAK CATUR", 450, 450); });
    t.detach();

    while (!frame.isReady) { Sleep(10); }

    std::cout << "--- EDITOR PAPAN CATUR AKTIF ---" << std::endl;
    std::cout << "1. Geser jendela merah ke papan browser" << std::endl;
    std::cout << "2. KLIK KANAN di kotak untuk pasang bidak" << std::endl;
    std::cout << "3. Tekan ENTER untuk cetak FEN" << std::endl;

    while (true) {
        std::string input;
        std::getline(std::cin, input);

        std::string fen = "";
        for (int i = 0; i < 8; i++) {
            int emptyCount = 0;
            for (int j = 0; j < 8; j++) {
                std::string p = frame.boardState[i * 8 + j];
                if (p == ".") {
                    emptyCount++;
                } else {
                    if (emptyCount > 0) { 
                        fen += std::to_string(emptyCount); 
                        emptyCount = 0; 
                    }
                    fen += p;
                }
            }
            if (emptyCount > 0) fen += std::to_string(emptyCount);
            if (i < 7) fen += "/";
        }
        
        std::cout << "\n>>> FEN SAAT INI:" << std::endl;
        std::cout << fen << " w - - 0 1" << std::endl;
        std::cout << "-------------------------------------------" << std::endl;
    }
    return 0;
}