import argparse
import time
import cv2
import queue
import threading
import torch
from ultralytics import YOLO

torch.set_num_threads(1)


class ParallelYOLO:

    def __init__(self, model_path="yolov8s-pose.pt", workers=4):
        self.model_path = model_path
        self.num_workers = workers

        self.input_queue = queue.Queue(maxsize=30)

        self.output_lock = threading.Lock()
        self.latest_result = None

        self.stop_event = threading.Event()
        self.threads = []

        self.processed_frames = 0

    def add_frame(self, frame_id, frame):
        if not self.input_queue.full():
            self.input_queue.put((frame_id, frame))

    def worker(self, worker_id):
        torch.set_num_threads(1)

        model = YOLO(self.model_path)

        while not self.stop_event.is_set():
            try:
                frame_id, frame = self.input_queue.get(timeout=0.1)

            except queue.Empty:
                continue
            results = model(frame)

            with self.output_lock:
                self.latest_result = results
                self.processed_frames += 1

            self.input_queue.task_done()

    def start(self):
        for i in range(self.num_workers):
            t = threading.Thread(
                target=self.worker,
                args=(i,),
                daemon=True,
            )

            t.start()
            self.threads.append(t)

    def get_latest_result(self):
        with self.output_lock:
            return self.latest_result

    def get_processed_frames(self):
        with self.output_lock:
            return self.processed_frames

    def stop(self):
        self.stop_event.set()

        for t in self.threads:
            t.join()


def single_thread_mode(model_path="yolov8s-pose.pt"):
    torch.set_num_threads(torch.get_num_threads())

    model = YOLO(model_path)

    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        print("Ошибка: камера не открылась")
        return

    processed_frames = 0

    start_time = time.time()

    while True:
        ret, frame = cap.read()

        if not ret:
            break
        results = model(frame)

        processed_frames += 1

        annotated = results[0].plot()

        cv2.imshow("Single-thread", annotated)
        elapsed = time.time() - start_time

        if elapsed >= 1.0:
            fps = processed_frames / elapsed

            print(f"[Single] Inference FPS: {fps:.2f}")

            processed_frames = 0
            start_time = time.time()

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


def multi_thread_mode(model_path="yolov8s-pose.pt", workers=4):
    proc = ParallelYOLO(model_path, workers)
    proc.start()

    cap = cv2.VideoCapture(0)

    if not cap.isOpened():
        print("Ошибка: камера не открылась")
        return

    frame_id = 0

    last_processed = 0
    start_time = time.time()

    while True:
        ret, frame = cap.read()

        if not ret:
            break

        proc.add_frame(frame_id, frame)

        latest = proc.get_latest_result()

        if latest is not None:
            annotated = latest[0].plot()
            cv2.imshow("Multi-thread", annotated)

        else:
            cv2.imshow("Multi-thread", frame)

        elapsed = time.time() - start_time

        if elapsed >= 1.0:
            current_processed = proc.get_processed_frames()

            fps = (current_processed - last_processed) / elapsed

            print(f"[Multi] Workers={workers} | " f"Inference FPS: {fps:.2f}")

            last_processed = current_processed
            start_time = time.time()

        frame_id += 1

        if cv2.waitKey(1) & 0xFF == ord("q"):
            break

    proc.stop()

    cap.release()
    cv2.destroyAllWindows()


def main():
    parser = argparse.ArgumentParser(
        description="YOLOv8 CPU benchmark: single vs multi-thread"
    )

    parser.add_argument(
        "--mode",
        choices=["single", "multi"],
        default="single",
        help="Режим работы",
    )

    parser.add_argument(
        "--workers",
        type=int,
        default=4,
        help="Количество worker threads",
    )

    parser.add_argument(
        "--model",
        type=str,
        default="yolov8s-pose.pt",
        help="Путь к модели",
    )

    args = parser.parse_args()

    print(f"PyTorch threads: {torch.get_num_threads()}")

    if args.mode == "single":
        print("Запуск SINGLE THREAD режима")
        single_thread_mode(args.model)

    else:
        print(f"Запуск MULTI THREAD режима " f"(workers={args.workers})")

        multi_thread_mode(
            model_path=args.model,
            workers=args.workers,
        )


if __name__ == "__main__":
    main()
