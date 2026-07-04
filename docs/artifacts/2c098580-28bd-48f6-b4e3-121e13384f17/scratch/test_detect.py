import os
import sys
import cv2
import glob
import shutil

# Add the project root and tools/MuseTalk to path
project_root = r"c:\just-dub-it2"
sys.path.insert(0, project_root)
sys.path.insert(0, os.path.join(project_root, "tools", "MuseTalk"))

# Setup DLL directories for Windows CUDA
try:
    _base = os.path.join(project_root, "env", "Lib", "site-packages", "nvidia")
    _runtime_bin = os.path.abspath(os.path.join(_base, "cuda_runtime", "bin"))
    _cublas_bin = os.path.abspath(os.path.join(_base, "cublas", "bin"))
    _cudnn_bin = os.path.abspath(os.path.join(_base, "cudnn", "bin"))
    
    for _bin_path in [_runtime_bin, _cublas_bin, _cudnn_bin]:
        if os.path.exists(_bin_path):
            os.add_dll_directory(_bin_path)
            os.environ["PATH"] = _bin_path + os.pathsep + os.environ.get("PATH", "")
            print(f"Added DLL path: {_bin_path}")
except Exception as e:
    print(f"Error setting up DLL path: {e}")

from musetalk.utils.preprocessing import get_landmark_and_bbox

def main():
    video_path = os.path.join(project_root, "test_dummy_spoken.mp4")
    temp_dir = os.path.join(project_root, "workspace", "scratch_detect_tmp")
    os.makedirs(temp_dir, exist_ok=True)
    
    # Extract frames
    print("Extracting frames...")
    cmd = f"ffmpeg -y -v fatal -i {video_path} -start_number 0 {temp_dir}/%08d.png"
    os.system(cmd)
    
    img_list = sorted(glob.glob(os.path.join(temp_dir, '*.png')))
    print(f"Extracted {len(img_list)} frames.")
    
    if not img_list:
        print("No frames extracted!")
        return

    try:
        print("Running get_landmark_and_bbox...")
        coords = get_landmark_and_bbox(img_list, 0)
        print("Resulting coords:")
        for idx, coord in enumerate(coords):
            print(f"  Frame {idx}: {coord}")
    except Exception as e:
        import traceback
        traceback.print_exc()
    finally:
        # Cleanup
        shutil.rmtree(temp_dir)

if __name__ == "__main__":
    main()
