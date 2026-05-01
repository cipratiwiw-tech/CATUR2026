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
    gray = cv2.cvtColor(cell, cv2.COLOR_BGR2GRAY)
    gray_std = cv2.resize(gray, (64, 64))
    
    edges = cv2.Canny(gray_std, 40, 120)
    
    # PERBAIKAN: Dilation diturunkan ke 2x2 agar garis tebal tapi tidak menggumpal
    kernel = np.ones((2, 2), np.uint8)
    edges_thick = cv2.dilate(edges, kernel, iterations=1)
    
    # PERBAIKAN: Hapus pojokan cukup 14x14 agar badan bidak tidak terpotong
    edges_thick[0:14, 0:14] = 0   
    edges_thick[-14:, -14:] = 0   
    
    return gray_std, edges_thick 

def is_empty(edges_thick):
    # Jika garis siluetnya sangat sedikit, fix ini kotak kosong
    return cv2.countNonZero(edges_thick) < 20

def get_piece_color(gray_std):
    # PERBAIKAN: Ambil rata-rata kecerahan di pusat badan bidak
    # Jauh lebih akurat daripada menghitung jumlah piksel
    center_area = gray_std[24:40, 24:40]
    avg_brightness = np.mean(center_area)
    
    if avg_brightness > 120:
        return "WHITE"
    else:
        return "BLACK"

def build_templates(img):
    if not os.path.exists(MAPPING_PATH):
        return None

    with open(MAPPING_PATH, "r") as f:
        mapping = json.load(f)

    templates_dict = {}

    for i in range(64):
        label = mapping[i]
        if label != ".":
            r, c = i // 8, i % 8
            cell = get_cell(img, r, c)
            gray, edges = preprocess(cell)
            
            # PERBAIKAN: Jangan di-upper()! Biarkan 'p' dan 'P' tersimpan masing-masing
            # agar mesin punya banyak referensi bentuk.
            if label not in templates_dict:
                templates_dict[label] = edges
                cv2.imwrite(os.path.join(DEBUG_DIR, f"{label}.png"), edges)

    np.savez(TEMPLATE_CACHE, **templates_dict)
    return templates_dict

def load_templates():
    if os.path.exists(TEMPLATE_CACHE):
        data = np.load(TEMPLATE_CACHE, allow_pickle=True)
        return {key: data[key] for key in data.files}
    return None

def match(cell_edges, templates_dict):
    best_score = -1
    best_shape = "."

    for label, t_img in templates_dict.items():
        res = cv2.matchTemplate(cell_edges, t_img, cv2.TM_CCOEFF_NORMED)
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
        gray, edges = preprocess(cell)

        if is_empty(edges):
            board.append(".")
            continue

        shape, score = match(edges, templates)

        # Toleransi dikembalikan ke 0.25
        if score < 0.25: 
            board.append(".")
        else:
            # PERBAIKAN: Kalau kebetulan yang mirip adalah template Empty (E), 
            # paksa jadi titik (.) agar C++ tidak nulis huruf E di FEN.
            if shape.upper() == "E":
                board.append(".")
            else:
                color = get_piece_color(gray)
                if color == "WHITE":
                    board.append(shape.upper())
                else:
                    board.append(shape.lower())

with open(BOARD_JSON, "w") as f:
    json.dump(board, f)

print("[INFO] Detection done")