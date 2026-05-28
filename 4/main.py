import threading
import queue
import time
import cv2
import logging
import os
import random

if not os.path.exists("log"):
    os.makedirs("log")

logging.basicConfig(
    filename="log/sensor_system.log",
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    encoding="utf-8",
)


class Sensor:
    def get(self):
        raise NotImplementedError("Subclasses must implement method get()")


class SensorX(Sensor):
    def __init__(self, delay: float):
        self.delay = delay
        self.data = 0

    def get(self) -> int:
        time.sleep(self.delay)
        self.data = random.randint(0, 100)
        return self.data


class SensorCam(Sensor):
    def __init__(self, device_name, resolution):
        self.cap = cv2.VideoCapture(device_name)
        if not self.cap.isOpened():
            logging.error(f"Не удалось открыть камеру: {device_name}")
            raise RuntimeError("Camera not found")

        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])
        logging.info(f"Камера {device_name} инициализирована успешно.")

    def get(self):
        ret, frame = self.cap.read()
        if not ret:
            logging.error("Ошибка чтения кадра.")
            return None
        return frame

    def __del__(self):
        if hasattr(self, "cap") and self.cap.isOpened():
            self.cap.release()
            logging.info("Ресурс камеры освобожден (деструктор).")


class WindowImage:
    def __init__(self, title, fps):
        self.title = title
        self.delay = int(1000 / fps)
        cv2.namedWindow(self.title)
        logging.info(f"Окно {title} создано.")

    def show(self, img):
        if img is not None:
            cv2.imshow(self.title, img)
            if cv2.waitKey(self.delay) & 0xFF == ord("q"):
                return False
        return True

    def __del__(self):
        cv2.destroyWindow(self.title)
        logging.info(f"Окно {self.title} закрыто (деструктор).")


def sensor_worker(sensor_obj, data_queue):
    while True:
        data = sensor_obj.get()
        if data is not None:
            if not data_queue.empty():
                try:
                    data_queue.get_nowait()
                except queue.Empty:
                    pass
            data_queue.put(data)


def main():
    try:
        cam = SensorCam(0, (1280, 720))
        s1 = SensorX(0.1)
        s2 = SensorX(0.2)
        s3 = SensorX(0.05)

        q_cam = queue.Queue(maxsize=1)
        q_s1 = queue.Queue(maxsize=1)
        q_s2 = queue.Queue(maxsize=1)
        q_s3 = queue.Queue(maxsize=1)

        threads = [
            threading.Thread(target=sensor_worker, args=(cam, q_cam), daemon=True),
            threading.Thread(target=sensor_worker, args=(s1, q_s1), daemon=True),
            threading.Thread(target=sensor_worker, args=(s2, q_s2), daemon=True),
            threading.Thread(target=sensor_worker, args=(s3, q_s3), daemon=True),
        ]
        for t in threads:
            t.start()

        window = WindowImage("Control System", 30)

        last_vals = {"s1": 0, "s2": 0, "s3": 0}

        while True:
            if not q_cam.empty():
                frame = q_cam.get()

                if not q_s1.empty():
                    last_vals["s1"] = q_s1.get()
                if not q_s2.empty():
                    last_vals["s2"] = q_s2.get()
                if not q_s3.empty():
                    last_vals["s3"] = q_s3.get()

                info = (
                    f"S1: {last_vals['s1']} S2: {last_vals['s2']} S3: {last_vals['s3']}"
                )
                cv2.putText(
                    frame, info, (30, 50), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2
                )

                if not window.show(frame):
                    break
            else:
                time.sleep(0.01)

    except Exception as e:
        logging.critical(f"Критический сбой: {e}")
    finally:
        logging.info("Программа завершена.")


if __name__ == "__main__":
    main()
