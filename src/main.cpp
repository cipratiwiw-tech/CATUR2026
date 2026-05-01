#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <cmath>
#include "Overlay.h"
#include "ScreenCapture.h"
#include "Vision.h"
#include <map>
#include <fstream>
#include <sstream>
namespace fs = std::filesystem;

// Struktur dan fungsi Callback untuk Mouse di Jendela Popup
struct MouseCallbackData {
    cv::Mat dialogImg;
    cv::Mat boardImage;
    int width, height;
    std::vector<std::string> pieces;
    std::vector<std::string> pieceKeys;
    std::vector<int> maxLimits;
    std::vector<int> selectionCounts;
    std::vector<cv::Rect> buttonRects;
    std::vector<ChessSquare> grid;
    std::vector<bool> selectedCells; // Menyimpan status apakah cell sudah di-klik
    std::vector<std::string> cellLabels; // TAMBAHAN: Menyimpan huruf FEN (P, N, k, dll)

    cv::Rect btnReset; // Area tombol Reset
    cv::Rect btnSave;  // Area tombol Save
    cv::Rect btnAutoWBottom; // Tombol Scan Auto (Putih Bawah)
    cv::Rect btnAutoWTop;    // Tombol Scan Auto (Putih Atas)
    int activePieceIndex = -1;
    bool needsRedraw = false;
    bool closeDialog = false; // Penanda untuk keluar dialog
};

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        MouseCallbackData* data = static_cast<MouseCallbackData*>(userdata);
        
        // Cek klik di area tombol Reset
        if (data->btnReset.contains(cv::Point(x, y))) {
            std::fill(data->selectionCounts.begin(), data->selectionCounts.end(), 0);
            std::fill(data->selectedCells.begin(), data->selectedCells.end(), false);
            data->activePieceIndex = -1;
            data->needsRedraw = true;
            return;
        }
        
        // Cek klik di area tombol Save
        if (data->btnSave.contains(cv::Point(x, y))) {
            data->closeDialog = true;
            return;
        }

        // Cek klik di area tombol Auto Scan
        if (data->btnAutoWBottom.contains(cv::Point(x, y)) || data->btnAutoWTop.contains(cv::Point(x, y))) {
            bool whiteBottom = data->btnAutoWBottom.contains(cv::Point(x, y));
            
            // Bersihkan seleksi sebelumnya
            std::fill(data->selectionCounts.begin(), data->selectionCounts.end(), 0);
            std::fill(data->selectedCells.begin(), data->selectedCells.end(), false);
            
            const char* layoutWB[8] = {
                "rnbqkbnr", "pppppppp", "EEEEEEEE", "EEEEEEEE",
                "EEEEEEEE", "EEEEEEEE", "PPPPPPPP", "RNBQKBNR"
            };
            const char* layoutWT[8] = {
                "RNBKQBNR", "PPPPPPPP", "EEEEEEEE", "EEEEEEEE",
                "EEEEEEEE", "EEEEEEEE", "pppppppp", "rnbkqbnr"
            };
            const char** layout = whiteBottom ? layoutWB : layoutWT;
            
            for (int i = 0; i < 64; i++) {
                char p = layout[i / 8][i % 8]; // Dapatkan karakter susunan awal sesuai baris & kolom
                
                int idx = -1;
                for (size_t j = 0; j < data->pieceKeys.size(); j++) { if (data->pieceKeys[j][0] == p) { idx = static_cast<int>(j); break; } }
                
                if (idx != -1 && data->selectionCounts[idx] < data->maxLimits[idx]) {
                    data->selectionCounts[idx]++;
                    cv::Mat cellImg = data->boardImage(data->grid[i].area);
                    data->selectedCells[i] = true;
                    // TAMBAHKAN BARIS INI: Agar tombol Auto Scan juga menyimpan identitas FEN bidak ke template!
                    data->cellLabels[i] = data->pieceKeys[idx]; 
                }
            }
            data->activePieceIndex = -1;
            data->needsRedraw = true;
            return;
        }

        // Cek klik di area tombol (panel kanan)
        for (int i = 0; i < data->buttonRects.size(); i++) {
            if (data->buttonRects[i].contains(cv::Point(x, y))) {
                data->activePieceIndex = i;
                data->needsRedraw = true; // Picu penggambaran ulang UI
                return;
            }
        }
        
        // Cek klik di area grid papan catur (panel kiri)
        if (x >= 0 && x < data->width && y >= 0 && y < data->height) {
            if (data->activePieceIndex != -1) {
                for (int i = 0; i < data->grid.size(); i++) {
                    const auto& sq = data->grid[i];
                    if (sq.area.contains(cv::Point(x, y))) {
                        // Cegah seleksi berulang di kotak yang sama
                        if (data->selectedCells[i]) return;
                        
                        int idx = data->activePieceIndex;
                        
                        // Cek apakah sudah mencapai batas maksimal seleksi untuk bidak ini
                        if (data->selectionCounts[idx] >= data->maxLimits[idx]) return;
                        
                        data->selectionCounts[idx]++; // Tambah hitungan

                        cv::Mat cellImg = data->boardImage(sq.area);
                        
                        // Tandai bahwa cell ini sudah pernah di-select
                        data->selectedCells[i] = true;
                        data->cellLabels[i] = data->pieceKeys[idx]; // TAMBAHAN: Rekam label bidak untuk kotak ini
                        

                        // Efek visual sementara (kotak merah) saat gambar di-crop
                        cv::rectangle(data->dialogImg, sq.area, cv::Scalar(0, 0, 255, 255), 2);
                        cv::imshow("Screenshot Area Grid", data->dialogImg);
                        cv::waitKey(50); // Tahan sebentar
                        
                        data->needsRedraw = true; // Gambar ulang agar kotak merah hilang
                        return;
                    }
                }
            }
        }
    }
}

std::vector<std::string> loadBoardFromJson(const std::string& filename) {
    std::vector<std::string> board(64, ".");

    std::ifstream file(filename);
    if (!file.is_open()) return board;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    int idx = 0;
    for (char c : content) {
        if (std::isalpha(c) || c == '.') {
            if (idx < 64) {
                board[idx++] = std::string(1, c);
            }
        }
    }

    return board;
}

int main() {
    SetProcessDPIAware();

    Overlay frame;
    
    std::thread t([&]() { frame.start("EDITOR BIDAK CATUR", 450, 450); });
    t.detach();

    while (!frame.isReady) { Sleep(10); }

    ScreenCapture capture;
    Vision vision;

    while (true) {
        // Jika menu belum diklik, aplikasi cukup "tidur" dan cek kembali
        if (!frame.isAnalyzing && !frame.showPieceSelector) {
            Sleep(100);
            continue;
        }

        // 1. Dapatkan posisi & dimensi kotak grid dari jendela overlay layar
        RECT clientRect;
        GetClientRect(frame.hwnd, &clientRect);
        
        int padding = 20; // Sesuai dengan ukuran padding yang digambar di Overlay.cpp
        int barWidth = 20;
        int barGap = 15;
        POINT topLeft = { clientRect.left + padding, clientRect.top + padding };
        POINT bottomRight = { clientRect.right - padding - barWidth - barGap, clientRect.bottom - padding };
        
        ClientToScreen(frame.hwnd, &topLeft);
        ClientToScreen(frame.hwnd, &bottomRight);
        
        int width = bottomRight.x - topLeft.x;
        int height = bottomRight.y - topLeft.y;

        cv::Mat boardImage;
        if (width > 0 && height > 0) {
            // 2. Ambil screenshot murni dari area papan catur yang ditargetkan
            boardImage = capture.captureRegion(topLeft.x, topLeft.y, width, height);
            
            // 3. Bagi gambar ke dalam grid 8x8 kotak (Dipindah agar bisa digunakan di Callback)
            std::vector<ChessSquare> grid = vision.createGrid(cv::Point(0, 0), width, height);

            // Tampilkan POPUP SCREENSHOT jika menu Pilih Bidak diklik
            if (frame.showPieceSelector) {
                // --- MEMBUAT PANEL UI DI KANAN ---
                int panelWidth = 220; // Diperlebar sedikit agar teks limit "[0/8]" muat
                int minHeight = 560; // Ruang ekstra untuk tombol Auto Scan
                int dialogHeight = height > minHeight ? height : minHeight;
                
                // Siapkan kumpulan data untuk dikirim ke event detektor mouse
                MouseCallbackData cbData;
                cbData.width = width;
                cbData.height = height;
                cbData.boardImage = boardImage;
                cbData.grid = grid;
                cbData.selectedCells.assign(64, false); // Set awal ke-64 kotak belum di-select
                cbData.cellLabels.assign(64, "E"); // TAMBAHAN: Inisialisasi awal list label ke-64 kotak
                cbData.dialogImg = cv::Mat(dialogHeight, width + panelWidth, CV_8UC4, cv::Scalar(45, 45, 45, 255));
                 // Gambar list tombol bidak di sisi kanan
                cbData.pieces = {
                    "Putih: Pawn (P)", "Putih: Knight (N)", "Putih: Bishop (B)", 
                    "Putih: Rook (R)", "Putih: Queen (Q)", "Putih: King (K)",
                    "Hitam: Pawn (p)", "Hitam: Knight (n)", "Hitam: Bishop (b)", 
                    "Hitam: Rook (r)", "Hitam: Queen (q)", "Hitam: King (k)",
                    "Kosong: Empty (E)"
                };
                cbData.pieceKeys = {"P", "N", "B", "R", "Q", "K", "p", "n", "b", "r", "q", "k", "E"}; // Nama File Referensi FEN
                cbData.maxLimits = {8, 2, 2, 2, 1, 1, 8, 2, 2, 2, 1, 1, 64}; // Batas seleksi per-bidak
                cbData.selectionCounts.assign(13, 0); // Jumlah seleksi dimulai dari 0

                int startX = width + 10;
                int startY = 10;
                int bottomAreaHeight = 90; // Sediakan ruang untuk 4 tombol khusus di bawah
                int btnHeight = (dialogHeight - bottomAreaHeight - 20) / 13; // Bagi rata sisa tinggi ruang
                if (btnHeight > 35) btnHeight = 35; // Batasi tinggi maksimal tombol
                
                for (int i = 0; i < cbData.pieces.size(); i++) {
                    cbData.buttonRects.push_back(cv::Rect(startX, startY + (i * btnHeight), panelWidth - 20, btnHeight - 5));
                }
                
                // Definisikan letak Tombol Reset, Save dan Auto Scan
                int btnW = (panelWidth - 30) / 2;
                cbData.btnReset = cv::Rect(startX, dialogHeight - 85, btnW, 35);
                cbData.btnSave = cv::Rect(startX + btnW + 10, dialogHeight - 85, btnW, 35);
                cbData.btnAutoWBottom = cv::Rect(startX, dialogHeight - 45, btnW, 35);
                cbData.btnAutoWTop = cv::Rect(startX + btnW + 10, dialogHeight - 45, btnW, 35);
                // ---------------------------------
                cbData.needsRedraw = true; // Nyalakan render UI awal
                
                // Buat jendelanya terlebih dahulu secara eksplisit
                cv::namedWindow("Screenshot Area Grid", cv::WINDOW_AUTOSIZE);
                cv::setWindowProperty("Screenshot Area Grid", cv::WND_PROP_TOPMOST, 1); // Memaksa OpenCV merender di paling depan
                
                // Daftarkan event deteksi Mouse
                cv::setMouseCallback("Screenshot Area Grid", onMouse, &cbData);

                // Kunci (disable) jendela utama agar tidak bisa diklik
                EnableWindow(frame.hwnd, FALSE);

                // Memastikan popup bertindak sebagai dialog (anak) dari jendela utama Overlay
                HWND popupHwnd = FindWindowA(NULL, "Screenshot Area Grid");
                if (popupHwnd) {
                    SetWindowLongPtrA(popupHwnd, GWLP_HWNDPARENT, (LONG_PTR)frame.hwnd);
                    // Ubah Kursor Mouse jadi Normal Arrow (Menghilangkan Crosshair bawaan OpenCV)
                    SetClassLongPtrA(popupHwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
                }
                
                // Tahan popup sampai user menekan tombol di keyboard atau menekan tombol 'X'
                while (true) {
                    // Pengecekan Dinamis: Gambar Ulang Antarmuka Jendela saat Mouse Mengeklik 
                    if (cbData.needsRedraw) {
                        // 1. Reset background UI panel
                        cv::rectangle(cbData.dialogImg, cv::Rect(width, 0, panelWidth, dialogHeight), cv::Scalar(45, 45, 45, 255), cv::FILLED);
                        
                        // 2. Gambar ulang Papan (Biar efek klik sebelumnya terhapus)
                        cv::Mat leftROI = cbData.dialogImg(cv::Rect(0, 0, width, height));
                        boardImage.copyTo(leftROI);
                        
                        // 3. Highlight sel yang sudah dipilih (Garis hijau OpenCV dihapus agar tidak menumpuk dengan asli)
                        for (int i = 0; i < grid.size(); i++) {
                            const auto& sq = grid[i];
                            if (cbData.selectedCells[i]) {
                                // Jika kotak SUDAH di-select: Bingkai tebal warna Oranye
                                cv::rectangle(cbData.dialogImg, sq.area, cv::Scalar(0, 165, 255, 255), 3);
                            }
                        }

                        // 4. Gambar ulang status masing-masing tombol
                        for (int i = 0; i < cbData.pieces.size(); i++) {
                            cv::Rect btn = cbData.buttonRects[i];
                            bool isActive = (i == cbData.activePieceIndex); // Ngecek tombol mana yang diklik
                            
                            // Warna tombol jadi Hijau terang kalau Aktif
                            cv::Scalar btnColor = isActive ? cv::Scalar(0, 200, 0, 255) : 
                                ((i < 6) ? cv::Scalar(200, 200, 200, 255) : 
                                 (i < 12) ? cv::Scalar(80, 80, 80, 255) : cv::Scalar(120, 120, 150, 255)); // Tombol Empty (E)
                            cv::Scalar textColor = (i < 6 && !isActive) ? cv::Scalar(0, 0, 0, 255) : cv::Scalar(255, 255, 255, 255);
                            
                            cv::rectangle(cbData.dialogImg, btn, btnColor, cv::FILLED);
                            
                            int thickness = isActive ? 2 : 1;
                            cv::Scalar borderColor = isActive ? cv::Scalar(0, 255, 0, 255) : cv::Scalar(0, 0, 0, 255);
                            cv::rectangle(cbData.dialogImg, btn, borderColor, thickness);
                            
                            // Gabungkan nama tombol dengan informasi sisa limit kuota (Contoh: "Putih: Pawn (P) [1/8]")
                            std::string text = cbData.pieces[i] + " [" + std::to_string(cbData.selectionCounts[i]) + "/" + std::to_string(cbData.maxLimits[i]) + "]";
                            
                            cv::putText(cbData.dialogImg, text, cv::Point(btn.x + 10, btn.y + (btn.height / 2) + 5), 
                                        cv::FONT_HERSHEY_SIMPLEX, 0.40, textColor, 1);
                        }
                        
                        // 5. Gambar Tombol Reset dan Save
                        cv::rectangle(cbData.dialogImg, cbData.btnReset, cv::Scalar(60, 60, 200), cv::FILLED); // Merah untuk Reset
                        cv::rectangle(cbData.dialogImg, cbData.btnReset, cv::Scalar(0, 0, 0), 1);
                        cv::putText(cbData.dialogImg, "Reset", cv::Point(cbData.btnReset.x + 25, cbData.btnReset.y + 22), 
                                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);

                        cv::rectangle(cbData.dialogImg, cbData.btnSave, cv::Scalar(200, 100, 40), cv::FILLED); // Biru untuk Save
                        cv::rectangle(cbData.dialogImg, cbData.btnSave, cv::Scalar(0, 0, 0), 1);
                        cv::putText(cbData.dialogImg, "Save", cv::Point(cbData.btnSave.x + 30, cbData.btnSave.y + 22), 
                                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);

                        // 6. Gambar Tombol Auto Scan
                        cv::rectangle(cbData.dialogImg, cbData.btnAutoWBottom, cv::Scalar(100, 150, 100), cv::FILLED); // Hijau Redup
                        cv::rectangle(cbData.dialogImg, cbData.btnAutoWBottom, cv::Scalar(0, 0, 0), 1);
                        cv::putText(cbData.dialogImg, "W.Bottom", cv::Point(cbData.btnAutoWBottom.x + 10, cbData.btnAutoWBottom.y + 22), 
                                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);

                        cv::rectangle(cbData.dialogImg, cbData.btnAutoWTop, cv::Scalar(100, 100, 150), cv::FILLED); // Ungu Redup
                        cv::rectangle(cbData.dialogImg, cbData.btnAutoWTop, cv::Scalar(0, 0, 0), 1);
                        cv::putText(cbData.dialogImg, "W.Top", cv::Point(cbData.btnAutoWTop.x + 20, cbData.btnAutoWTop.y + 22), 
                                    cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);
                        
                        cv::imshow("Screenshot Area Grid", cbData.dialogImg);
                        cbData.needsRedraw = false;
                    }

                    if (cv::waitKey(50) >= 0) break; // Delay sengaja dibuat lebih kecil (50ms) agar klik mouse sangat responsif
                    
                    if (cbData.closeDialog) break; // Keluar loop jika tombol Save diklik
                    
                    try {
                        // Periksa apakah jendela masih ada (belum ditutup manual via tombol 'X')

                        if (cv::getWindowProperty("Screenshot Area Grid", cv::WND_PROP_VISIBLE) < 1) {
                            break;
                        }
                    } catch (...) {
                        break; // Jendela sudah tidak ditemukan
                    }
                }
                
                // Buka kembali kunci jendela utama TERLEBIH DAHULU sebelum menutup popup
                EnableWindow(frame.hwnd, TRUE);
                SetForegroundWindow(frame.hwnd);
                
                // Tutup popup secara aman, abaikan jika jendela sudah terlanjur hancur
                try { cv::destroyWindow("Screenshot Area Grid"); } catch (...) {}

                // TAMBAHAN: Tulis pemetaan template ke JSON untuk dibaca Python
                std::ofstream mapFile("template_mapping.json");
                if (mapFile.is_open()) {
                    mapFile << "[\n";
                    for (int i = 0; i < 64; i++) {
                        mapFile << "  \"" << cbData.cellLabels[i] << "\"";
                        if (i < 63) mapFile << ",\n";
                        else mapFile << "\n";
                    }
                    mapFile << "]\n";
                    mapFile.close();
                }
                
                frame.showPieceSelector = false; // Reset status penanda
            }
        }

        // Hanya update logika FEN jika mode Analyze AKTIF
        if (frame.isAnalyzing) {
            if (!boardImage.empty()) {
                // 1. Save image ke file
                cv::imwrite("frame.png", boardImage);

                // 2. Panggil Python (blocking dulu, nanti kita async)
                system(".venv\\Scripts\\python detect.py");

                // 3. Load hasil dari Python
                std::vector<std::string> board = loadBoardFromJson("board.json");

                // 4. Update state
                for (int i = 0; i < 64; i++) {
                    frame.boardState[i] = board[i];
                }
            }

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
            
            // 5. Update FEN ke Overlay UI agar tampil di jendela aplikasi
            frame.currentFEN = fen + " w - - 0 1";
            InvalidateRect(frame.hwnd, NULL, TRUE); 
            
            Sleep(1000); // Jeda 1 detik agar tidak memberatkan performa
        }
    }
    return 0;
}