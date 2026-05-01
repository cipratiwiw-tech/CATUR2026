# Chess Overlay Analyzer (Realtime Screen-Based Engine)

## Overview

Chess Overlay Analyzer adalah aplikasi desktop berbasis overlay transparan yang mampu:

* Mengambil area papan catur langsung dari layar (screen capture)
* Mengenali posisi bidak secara otomatis
* Mengirim posisi ke engine (Stockfish)
* Menampilkan evaluasi secara real-time
* Menampilkan panah rekomendasi langkah langsung di atas papan

Aplikasi berjalan sebagai **single executable (.exe)** tanpa perlu setup manual engine atau dependensi tambahan oleh user.

---

## Core Features

### 1. Transparent Overlay Frame

* Window transparan (hanya border terlihat)
* Bisa di-drag dan resize
* Always-on-top
* Digunakan untuk menentukan area papan catur (ROI)

### 2. Real-time Board Recognition

* Deteksi papan catur dari area yang dipilih
* Identifikasi posisi bidak
* Output dalam format FEN

### 3. Stockfish Integration

* Engine berjalan di background
* Komunikasi via UCI protocol
* Analisis posisi secara real-time

### 4. Evaluation Bar (Vertical)

* Terletak di sisi kanan overlay
* Menampilkan keunggulan putih vs hitam
* Update secara dinamis

### 5. Move Arrow Overlay

* Visualisasi langkah terbaik dari engine
* Digambar langsung di atas papan
* Real-time update

### 6. One-Click Execution

* User hanya menjalankan file `.exe`
* Semua sistem (engine + vision + UI) sudah terintegrasi

---

## System Architecture

```
+---------------------------+
|        UI LAYER           |
|---------------------------|
| Overlay Frame             |
| Evaluation Bar            |
| Arrow Renderer            |
+---------------------------+
            |
            v
+---------------------------+
|     APPLICATION CORE      |
|---------------------------|
| State Manager             |
| Input Handler             |
| Thread Controller         |
+---------------------------+
            |
            v
+---------------------------+
|   PROCESSING MODULES      |
|---------------------------|
| Screen Capture            |
| Board Recognition         |
| FEN Generator             |
+---------------------------+
            |
            v
+---------------------------+
|     ENGINE INTERFACE      |
|---------------------------|
| UCI Communication         |
| Stockfish Process         |
| Output Parser             |
+---------------------------+
```

---

## Application Flow

### 1. Startup

* User menjalankan file `.exe`
* Aplikasi membuka overlay transparan

---

### 2. Area Selection

* User:

  * Drag overlay
  * Resize frame
* Tujuan:

  * Menyesuaikan dengan papan catur di layar

---

### 3. Start Analysis

* User klik tombol **Start Analyze**
* Sistem:

  * Menginisialisasi Stockfish
  * Mulai loop processing

---

### 4. Realtime Loop

Loop berjalan terus selama mode aktif:

#### Step 1: Capture

* Ambil gambar dari area overlay (ROI)

#### Step 2: Recognition

* Deteksi grid 8x8
* Identifikasi bidak
* Generate FEN

#### Step 3: Engine Analysis

* Kirim ke engine:

  ```
  position fen <FEN>
  go depth 15
  ```

#### Step 4: Parse Output

* Ambil:

  * Score (cp/mate)
  * Best move

#### Step 5: Render Update

* Update:

  * Evaluation bar
  * Arrow overlay

---

## UI Layout

```
+--------------------------------------------------+
|                                                  |
|   [ CHESS BOARD FRAME ]   [ EVAL BAR ]            |
|                                                  |
|                                                  |
|                                                  |
|                          [Start Analyze Button]   |
+--------------------------------------------------+
```

---

## Evaluation Bar Logic

* Range evaluasi:

  ```
  -10.0 (hitam menang) → +10.0 (putih menang)
  ```

* Normalisasi:

  ```
  normalized = (eval + 10) / 20
  ```

* Rendering:

  * Atas: putih
  * Bawah: hitam
  * Tengah: seimbang

---

## Arrow Rendering

### Input:

* Best move (contoh: e2e4)

### Konversi:

* Board → pixel mapping:

  ```
  cell_width  = width / 8
  cell_height = height / 8
  ```

### Output:

* Gambar panah:

  * Start: tengah kotak asal
  * End: tengah kotak tujuan

---

## Threading Model

### Thread 1: UI Thread

* Rendering overlay
* Input handling

### Thread 2: Vision Thread

* Screen capture
* Board recognition

### Thread 3: Engine Thread

* Komunikasi Stockfish
* Parsing output

---

## Engine Communication (UCI)

### Init:

```
uci
isready
ucinewgame
```

### Analisis:

```
position fen <FEN>
go depth 15
```

### Output:

```
info depth 15 score cp 34 pv e2e4
bestmove e2e4
```

---

## Project Structure (Suggested)

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
├── /assets
│
├── /models (optional for CV)
│
└── README.md
```

---

## Technology Stack (Recommended)

### Core

* C++ (performance critical)

### UI

* WinAPI / Qt

### Vision

* OpenCV

### Engine

* Stockfish (bundled binary)

---

## Performance Considerations

### Latency

* Gunakan ROI capture (bukan full screen)
* Batasi FPS (10–20 cukup)

### Engine Load

* Gunakan delay:

  * 200–500 ms per analisis

### Rendering

* Gunakan double buffering
* Hindari redraw berlebihan

---

## Packaging

### Goal:

Single executable distribution

### Include:

* app.exe
* stockfish.exe (embedded / bundled)

### Behavior:

* Engine dijalankan otomatis oleh aplikasi
* Tidak perlu install tambahan

---

## Future Improvements

* Multi-line analysis (Top 3 moves)
* Heatmap square
* Auto-detect chessboard
* GPU acceleration (vision)

---

## Summary

Aplikasi ini adalah sistem terintegrasi yang:

* Menggunakan overlay untuk menentukan area papan
* Melakukan computer vision untuk membaca posisi
* Menggunakan engine untuk analisis
* Menampilkan hasil secara visual real-time

Semua berjalan dalam satu aplikasi tanpa intervensi user selain setup awal area.

---
