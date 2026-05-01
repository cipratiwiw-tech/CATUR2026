import cv2
import json
import numpy as np
import os

IMG_PATH = "frame.png"
TEMPLATE_CACHE = "templates_cache.npz"
MAPPING_PATH = "template_mapping.json"
DEBUG_DIR = "templates_debug"
BOARD_JSON = "board.json"

if not os.path.exists(DEBUG_DIR):
    os.makedirs(DEBUG_DIR)

def get_cell(img, row, col):
    h, w = img.shape[:2]
    cell_w = w // 8
    cell_h = h // 8
    x1 = col * cell_w
    y1 = row * cell_h
    return img[y1:y1 + cell_h, x1:x1 + cell_w]

def preprocess(cell):
    # 1. Ambil 1 kotak PENUH
    gray = cv2.cvtColor(cell, cv2.COLOR_BGR2GRAY)
    
    # 2. CROP PENGAMAN EKSTRA (5 piksel dari tiap sisi)
    # Membuang sisa-sisa garis papan yang mungkin masih bocor dari C++
    h, w = gray.shape
    gray_safe = gray[5:h-5, 5:w-5]
    
    # 3. Baru distandardisasi ukurannya ke 64x64
    gray_std = cv2.resize(gray_safe, (64, 64))
    
    # 4. Hapus noise huruf/angka (cukup 12x12 karena sudah dikikis di awal)
    median_bg = np.median(gray_std)
    gray_std[0:12, 0:12] = median_bg   # Pojok Kiri Atas
    gray_std[-12:, -12:] = median_bg   # Pojok Kanan Bawah
    
    return gray_std

def is_empty(gray_std):
    # Sel kosong warnanya solid, standar deviasinya pasti sangat rendah (< 18)
    # Jika ada bidak, warnanya pasti bervariasi tajam (std > 30)
    return np.std(gray_std) < 18

def build_templates(img):
    if not os.path.exists(MAPPING_PATH):
        return None

    with open(MAPPING_PATH, "r") as f:
        mapping = json.load(f)

    # Load cache lama agar saat edit sepotong-sepotong template lama tidak hilang tereset
    templates_dict = load_templates()
    if templates_dict is None:
        templates_dict = {}

    for i in range(64):
        label = mapping[i]
        if label != "." and label != "E":
            r, c = i // 8, i % 8
            cell = get_cell(img, r, c)
            gray_std = preprocess(cell)
            
            # Hanya simpan atau timpa(overwrite) jika bidak itu di atas kotak yang TIDAK KOSONG
            if not is_empty(gray_std):
                templates_dict[label] = gray_std
                cv2.imwrite(os.path.join(DEBUG_DIR, f"{label}.png"), gray_std)

    np.savez(TEMPLATE_CACHE, **templates_dict)
    return templates_dict

def load_templates():
    if os.path.exists(TEMPLATE_CACHE):
        data = np.load(TEMPLATE_CACHE, allow_pickle=True)
        return {key: data[key] for key in data.files}
    return None

def match(gray_std, templates_dict):
    best_score = -1
    best_shape = "."

    # Langsung tes ke semua 12 template bidak yang tersimpan
    for label, t_img in templates_dict.items():
        res = cv2.matchTemplate(gray_std, t_img, cv2.TM_CCOEFF_NORMED)
        score = res[0][0]

        if score > best_score:
            best_score = score
            best_shape = label

    return best_shape, best_score

# =========================
# MAIN EXECUTION
# =========================
img = cv2.imread(IMG_PATH)
if img is None: exit()

templates = load_templates()

if templates is None or (os.path.exists(MAPPING_PATH) and os.path.getmtime(MAPPING_PATH) > os.path.getmtime(TEMPLATE_CACHE)):
    templates = build_templates(img)

board = []

for r in range(8):
    for c in range(8):
        cell = get_cell(img, r, c)
        gray_std = preprocess(cell)

        if is_empty(gray_std):
            board.append(".")
            continue

        shape, score = match(gray_std, templates)

        # CCOEFF_NORMED punya skala skor -1.0 sampai 1.0. Batas 0.45 sangat wajar.
        if score < 0.45: 
            board.append(".")
        else:
            board.append(shape) # Langsung masukkan hurufnya, tidak perlu cek warna lagi!

with open(BOARD_JSON, "w") as f:
    json.dump(board, f)

print("[INFO] Detection done")