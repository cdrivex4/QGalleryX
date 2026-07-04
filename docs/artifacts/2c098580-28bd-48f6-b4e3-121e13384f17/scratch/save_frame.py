import cv2
import os

project_root = r"c:\just-dub-it2"
video_path = os.path.join(project_root, "test_dummy_spoken.mp4")
output_path = r"C:\Users\curtis\.gemini\antigravity-ide\brain\2c098580-28bd-48f6-b4e3-121e13384f17\first_frame.png"

def main():
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print("Failed to open video")
        return
        
    ret, frame = cap.read()
    if ret:
        cv2.imwrite(output_path, frame)
        print(f"Saved first frame to {output_path}")
    else:
        print("Failed to read first frame")
    cap.release()

if __name__ == "__main__":
    main()
