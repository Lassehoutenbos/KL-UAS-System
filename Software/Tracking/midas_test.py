import cv2
import torch
import numpy as np
from torchvision.transforms import Compose, Resize, ToTensor, Normalize
from PIL import Image

# Load MiDaS
midas = torch.hub.load("intel-isl/MiDaS", "MiDaS_small", trust_repo=True)
midas.eval()

# Load transform
transform = Compose([
    Resize((256, 256)),
    ToTensor(),
    Normalize(mean=[0.485, 0.456, 0.406],
              std=[0.229, 0.224, 0.225])
])

# Open webcam
cap = cv2.VideoCapture(0)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    img_pil = Image.fromarray(img)
    input_tensor = transform(img_pil).unsqueeze(0)

    with torch.no_grad():
        prediction = midas(input_tensor)
        depth_map = prediction.squeeze().cpu().numpy()

    # Normalize depth map for visualization
    depth_vis = cv2.normalize(depth_map, None, 0, 255, cv2.NORM_MINMAX)
    depth_vis = depth_vis.astype(np.uint8)
    depth_vis = cv2.applyColorMap(depth_vis, cv2.COLORMAP_MAGMA)

    # Show frames
    cv2.imshow("Webcam", frame)
    cv2.imshow("Depth (MiDaS)", depth_vis)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
