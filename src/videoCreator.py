import cv2
import os
import sys

def create_video_from_images(folder_path, output_file="output.avi", fps=30):
    # List image files with common image extensions.
    valid_extensions = ('.png', '.jpg', '.jpeg', '.bmp', '.tiff')
    images = [img for img in os.listdir(folder_path) if img.lower().endswith(valid_extensions)]
    
    if not images:
        print("No images found in provided folder.")
        return

    # Sort images alphabetically to keep a consistent order
    images.sort()

    # Read the first image to determine the video frame dimensions
    first_image_path = os.path.join(folder_path, images[0])
    frame = cv2.imread(first_image_path)
    if frame is None:
        print(f"Error reading the first image: {first_image_path}")
        return

    height, width, layers = frame.shape
    size = (width, height)
    print(f"Video frame size: {size}, Number of frames: {len(images)}")

    # Define the codec and create VideoWriter object.
    # The codec 'MJPG' is widely compatible; you can also try 'XVID' or others.
    fourcc = cv2.VideoWriter_fourcc(*'MJPG')
    out = cv2.VideoWriter(output_file, fourcc, fps, size)

    for image in images:
        img_path = os.path.join(folder_path, image)
        frame = cv2.imread(img_path)
        if frame is None:
            print(f"Warning: Could not read image {img_path}. Skipping.")
            continue

        # In case the image does not match the first image size,
        # resize it to ensure consistency.
        if frame.shape[1] != width or frame.shape[0] != height:
            frame = cv2.resize(frame, size)

        out.write(frame)

    out.release()
    print(f"Video saved as '{output_file}'.")

if __name__ == "__main__":
    # Command-line usage: python videoCreator.py <folder_path> [output_file] [fps]
    if len(sys.argv) < 2:
        print("Usage: python videoCreator.py <folder_path> [output_file] [fps]")
    else:
        folder_path = sys.argv[1]
        output_file = sys.argv[2] if len(sys.argv) >= 3 else "output.avi"
        try:
            fps = int(sys.argv[3]) if len(sys.argv) >= 4 else 30
        except ValueError:
            print("FPS must be an integer. Using default FPS=30.")
            fps = 30
        create_video_from_images(folder_path, output_file, fps)
