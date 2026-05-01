# Chess Overlay Analyzer (Realtime Screen-Based Engine)

## Overview

Chess Overlay Analyzer adalah aplikasi desktop berbasis overlay transparan yang mampu:

* Mengambil area papan catur langsung dari layar (screen capture)
* Mengenali posisi bidak secara otomatis
* Mengirim posisi ke engine (Stockfish)
* Menampilkan evaluasi secara real-time
* Menampilkan panah rekomendasi langkah langsung di atas papan

Aplikasi berjalan sebagai **single executable (.exe)** tanpa setup tambahan.

---

## Key Features

* Transparent overlay (drag & resize)
* Real-time board recognition → FEN
* Engine analysis (Stockfish, UCI)
* Evaluation bar (vertical)
* Best move arrow overlay
* One-click run (.exe)

---

## Application Flow

### 1. Startup

* Jalankan `app.exe`
* Overlay transparan muncul

### 2. Area Selection

* Drag & resize frame
* Sesuaikan dengan papan catur di layar

### 3. Start Analyze

* Klik tombol **Start Analyze**
* Engine + processing mulai

### 4. Realtime Loop

Loop berjalan terus:

1. Capture ROI
2. Detect board → generate FEN
3. Kirim ke engine:

   ```
   position fen <FEN>
   go depth 15
   ```
4. Parse output:

   * score
   * bestmove
5. Update UI:

   * evaluation bar
   * arrow overlay

---

## System Architecture

* UI Layer → overlay + render
* Core → state & control
* Vision → capture + detection
* Engine → Stockfish interface

---

## Project Structure

```
/project-root
│
├── /bin
│   ├── app.exe
│   └── stockfish.exe
│
├── /src
│   ├── main.cpp
│   ├── ui/
│   ├── overlay/
│   ├── engine/
│   ├── vision/
│   └── utils/
│
├── CMakeLists.txt
└── README.md
```

---

## Build Instructions

### Requirements

* C++17+
* CMake
* vcpkg
* OpenCV

---

### Setup vcpkg

```
git clone https://github.com/microsoft/vcpkg
cd vcpkg
bootstrap-vcpkg.bat
```

Install dependencies:

```
vcpkg install opencv:x64-windows
```

---

### Build Project

```
mkdir build
cd build

cmake .. -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

---

## Running

Setelah build:

```
/bin/app.exe
```

Engine akan otomatis dijalankan dari:

```
/bin/stockfish.exe
```

---

## Engine Integration

Menggunakan UCI protocol:

Init:

```
uci
isready
ucinewgame
```

Analyze:

```
position fen <FEN>
go depth 15
```

Output:

```
bestmove e2e4
```

---

## Evaluation Logic

Range:

```
-10 → +10
```

Normalisasi:

```
normalized = (eval + 10) / 20
```

---

## Rendering Logic

### Arrow

* Mapping 8x8 grid → pixel
* Draw line + arrow head

### Eval Bar

* Vertical bar
* White (atas), Black (bawah)

---

## Threading

* UI Thread
* Vision Thread
* Engine Thread

Gunakan queue untuk sinkronisasi.

---

## Performance Notes

* Gunakan ROI capture (bukan full screen)
* Limit FPS: 10–20
* Delay engine: 200–500 ms

---

## Packaging

Distribusi:

* app.exe
* stockfish.exe

Tanpa dependency eksternal.

---

## Future Improvements

* Multi PV
* Auto board detection
* GPU acceleration
* Highlight squares

---

## Summary

Aplikasi ini:

* Mengambil input visual dari layar
* Mengubah ke data catur (FEN)
* Menganalisis dengan engine
* Menampilkan hasil secara real-time

Semua dalam satu executable.
