import os
import sys
import json
import cv2
import time

os.environ["OPENCV_LOG_LEVEL"] = "FATAL"
sys.path.append(os.path.abspath("chess-vision-main"))
from chess_vision.inference.pipeline import ChessRecognitionPipeline

IMG_PATH = "frame.png"
BOARD_JSON = "board.json"
CLASSIFIER_MODEL = "chess-vision-main/checkpoints/best_classifier.pt" 

def main():
    print("[INFO] Memuat Model AI ke RAM... Mohon tunggu sekitar 3 detik...")
    # Load model 1 KALI SAJA di awal
    pipeline = ChessRecognitionPipeline(
        classifier_weights=CLASSIFIER_MODEL,
        yolo_model_path=None, 
        confidence_threshold=0.6,
        device="cpu"
    )
    print("[INFO] Model siap! Standby menatap file frame.png...")

    last_mtime = 0
    
    # Loop abadi untuk memantau perubahan gambar
    while True:
        if not os.path.exists(IMG_PATH):
            time.sleep(0.1)
            continue
            
        # Cek apakah file frame.png baru saja diperbarui oleh C++
        current_mtime = os.path.getmtime(IMG_PATH)
        
        if current_mtime != last_mtime:
            last_mtime = current_mtime # Update waktu terakhir
            try:
                img = cv2.imread(IMG_PATH)
                if img is None:
                    continue
                    
                # Analisa langsung, ini sangat cepat karena model sudah di RAM!
                result = pipeline.recognize(img)
                fen_string = result.fen 
                
                flat_board = []
                for row in fen_string.split('/'):
                    for ch in row:
                        if ch.isdigit():
                            flat_board.extend(["."] * int(ch))
                        else:
                            flat_board.append(ch)
                            
                with open(BOARD_JSON, "w") as f:
                    json.dump(flat_board, f)
                    
            except Exception as e:
                pass
                
        # Jeda 0.1 detik agar CPU tidak dipaksa kerja 100%
        time.sleep(0.1)

if __name__ == "__main__":
    main()