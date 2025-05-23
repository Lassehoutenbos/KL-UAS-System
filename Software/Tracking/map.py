import cv2
import torch
import numpy as np
from torchvision.transforms import Compose, Resize, ToTensor, Normalize
from PIL import Image
import math
import csv
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# Load MiDaS model
midas = torch.hub.load("intel-isl/MiDaS", "MiDaS_small", trust_repo=True)
midas.eval()

transform = Compose([
    Resize((256, 256)),
    ToTensor(),
    Normalize(mean=[0.485, 0.456, 0.406],
              std=[0.229, 0.224, 0.225])
])

# Webcam setup
cap = cv2.VideoCapture(0)
ret, frame = cap.read()
init_bbox = cv2.selectROI("Select Object to Track", frame, False)
cv2.destroyWindow("Select Object to Track")

tracker = cv2.TrackerCSRT_create()
tracker.init(frame, init_bbox)

# Drone simulation config
desired_offset_xy = np.array([-100, 0])
desired_offset_z = 0.2
drone_pos = None
drone_vel = np.array([0.0, 0.0, 0.0])
last_pos = None
trail = []
log = []

follow_gain = 0.1

# 3D plot setup
fig = plt.figure(figsize=(8, 6))
ax = fig.add_subplot(111, projection='3d')
plt.ion()

def update_3d_view(position, velocity, target, trail):
    ax.clear()

    # Compute bounds dynamically
    margin = 150
    center_x, center_y = position[0], position[1]
    ax.set_xlim(center_x - margin, center_x + margin)
    ax.set_ylim(center_y - margin, center_y + margin)

    z_values = [p[2] for p in trail] + [position[2], target[2]]
    min_z = min(z_values)
    max_z = max(z_values)
    ax.set_zlim(min_z - 0.1, max_z + 0.1)

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Depth")

    # Drone
    ax.scatter(position[0], position[1], position[2], color='blue', label='Drone')

    # Object
    ax.scatter(target[0], target[1], target[2], color='green', label='Object')

    # Velocity vector
    ax.quiver(position[0], position[1], position[2],
              velocity[0], velocity[1], velocity[2],
              length=50, color='cyan', label='Velocity')

    # Facing vector (to object)
    direction = target - position
    norm = np.linalg.norm(direction)
    if norm > 0:
        direction = direction / norm
    ax.quiver(position[0], position[1], position[2],
              direction[0], direction[1], direction[2],
              length=75, color='yellow', label='Facing')

    # Trail
    if len(trail) > 1:
        xs = [p[0] for p in trail]
        ys = [p[1] for p in trail]
        zs = [p[2] for p in trail]
        ax.plot(xs, ys, zs, color='magenta', linewidth=1, label='Trail')

    ax.legend()
    plt.draw()
    plt.pause(0.001)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    success, bbox = tracker.update(frame)
    if not success:
        cv2.putText(frame, "Tracking Lost", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        cv2.imshow("Tracking", frame)
        if cv2.waitKey(1) & 0xFF == 27:
            break
        continue

    x, y, w, h = [int(v) for v in bbox]
    cx = x + w // 2
    cy = y + h // 2

    # Depth estimation
    img_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    img_pil = Image.fromarray(img_rgb)
    input_tensor = transform(img_pil).unsqueeze(0)
    with torch.no_grad():
        prediction = midas(input_tensor)
        depth_map = prediction.squeeze().cpu().numpy()
    depth_resized = cv2.resize(depth_map, (frame.shape[1], frame.shape[0]))
    object_depth = depth_resized[cy, cx]
    object_pos = np.array([cx, cy, object_depth], dtype=np.float32)

    if drone_pos is None:
        drone_pos = object_pos.copy()
        drone_pos[0:2] += desired_offset_xy
        drone_pos[2] += desired_offset_z

    target_pos = object_pos.copy()
    target_pos[0:2] += desired_offset_xy
    target_pos[2] += desired_offset_z

    # Velocity estimation
    if last_pos is not None:
        drone_vel = (drone_pos - last_pos) * 10
    last_pos = drone_pos.copy()

    error = target_pos - drone_pos
    drone_pos += error * follow_gain

    trail.append(drone_pos.copy())
    log.append(drone_pos.copy())

    update_3d_view(drone_pos, drone_vel, object_pos, trail)

    # 2D feedback
    cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
    cv2.circle(frame, (cx, cy), 4, (0, 255, 255), -1)
    cv2.putText(frame, f"Depth: {object_depth:.2f}", (cx + 10, cy - 10),
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
    cv2.imshow("Tracking", frame)

    if cv2.waitKey(1) & 0xFF == 27:
        break

cap.release()
cv2.destroyAllWindows()
plt.ioff()
plt.show()

# Save log
with open("drone_3d_orientation_log.csv", "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["x", "y", "z"])
    for p in log:
        writer.writerow([round(float(p[0]), 2), round(float(p[1]), 2), round(float(p[2]), 3)])
