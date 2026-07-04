import cv2
import os

project_root = r"c:\just-dub-it2"
video_path = os.path.join(project_root, "test_dummy_spoken.mp4")

def main():
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print("Failed to open video")
        return
        
    width = cap.get(cv2.CAP_PROP_FRAME_WIDTH)
    height = cap.get(cv2.CAP_PROP_FRAME_HEIGHT)
    fps = cap.get(cv2.CAP_PROP_FPS)
    frame_count = cap.get(cv2.CAP_PROP_FRAME_COUNT)
    
    print(f"Video Properties:")
    print(f"  Resolution: {width}x{height}")
    print(f"  FPS: {fps}")
    print(f"  Frame count: {frame_count}")
    
    ret, frame = cap.read()
    if ret:
        print(f"Frame shape: {frame.shape}")
        # Check standard deviation or average to see if it's blank/solid color
        mean, std = cv2.meanStdDev(frame)
        print(f"Frame Mean: {mean.flatten()}")
        print(f"Frame StdDev: {std.flatten()}")
    else:
        print("Failed to read first frame")
        
    cap.release()

if __name__ == "__main__":
    main()
