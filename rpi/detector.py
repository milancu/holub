import time
import requests
from io import BytesIO
from ultralytics import YOLO
from PIL import Image

ESP32_URL = "http://holub.local"
SNAPSHOT_ENDPOINT = f"{ESP32_URL}/snapshot"
SPRAY_ENDPOINT = f"{ESP32_URL}/spray"

CONFIDENCE_THRESHOLD = 0.5
CHECK_INTERVAL_S = 1.0
BIRD_CLASS_ID = 14  # COCO "bird" class

model = YOLO("yolov8n.pt")


def grab_snapshot():
    resp = requests.get(SNAPSHOT_ENDPOINT, timeout=5)
    resp.raise_for_status()
    return Image.open(BytesIO(resp.content))


def detect_bird(image):
    results = model(image, verbose=False)
    for box in results[0].boxes:
        cls_id = int(box.cls[0])
        conf = float(box.conf[0])
        if cls_id == BIRD_CLASS_ID and conf >= CONFIDENCE_THRESHOLD:
            return True, conf
    return False, 0.0


def spray():
    try:
        resp = requests.post(SPRAY_ENDPOINT, params={"ms": 500}, timeout=5)
        if resp.status_code == 200:
            print("[spray] pumpa aktivovana")
        elif resp.status_code == 429:
            print("[spray] cooldown, preskakuji")
        else:
            print(f"[spray] neocekavany status: {resp.status_code}")
    except requests.RequestException as e:
        print(f"[spray] chyba: {e}")


def main():
    print("[detector] startuji — cekam na ESP32-CAM...")

    # Pockame az bude ESP32 dostupna
    while True:
        try:
            requests.get(f"{ESP32_URL}/status", timeout=3)
            break
        except requests.RequestException:
            time.sleep(2)

    print("[detector] ESP32-CAM dostupna, zacinam detekci")

    while True:
        try:
            image = grab_snapshot()
            found, conf = detect_bird(image)
            if found:
                print(f"[detector] HOLUB detekovan (conf={conf:.2f}) — spoustim spray")
                spray()
        except requests.RequestException as e:
            print(f"[detector] chyba spojeni: {e}")
        except Exception as e:
            print(f"[detector] chyba: {e}")

        time.sleep(CHECK_INTERVAL_S)


if __name__ == "__main__":
    main()
