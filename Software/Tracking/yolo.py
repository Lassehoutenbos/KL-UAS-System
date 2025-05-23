import cv2
import numpy as np

# --- Init webcam ---
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("❌ Cannot open webcam")
    exit()

# --- Grab initial frame for selection ---
ret, frame = cap.read()
if not ret:
    print("❌ Failed to read from webcam")
    cap.release()
    exit()

# --- Let user select object ---
init_bbox = cv2.selectROI("Select Object to Track", frame, False)
cv2.destroyWindow("Select Object to Track")

# --- Initialize tracker ---
tracker = cv2.TrackerCSRT_create()
tracker.init(frame, init_bbox)
tracking = True
last_bbox = init_bbox

# --- Frame center ---
frame_h, frame_w = frame.shape[:2]
center_x, center_y = frame_w // 2, frame_h // 2

# --- RC simulation settings ---
rc_yaw = 1500
rc_pitch = 1500
gain_x = 0.3
gain_y = 0.3

while True:
    ret, frame = cap.read()
    if not ret:
        break

    cx, cy = center_x, center_y  # fallback if tracking fails

    if tracking:
        success, bbox = tracker.update(frame)
        if success:
            x, y, w, h = [int(v) for v in bbox]
            last_bbox = (x, y, w, h)
            cx = x + w // 2
            cy = y + h // 2
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
            cv2.putText(frame, "Tracking", (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
        else:
            tracking = False
            cv2.putText(frame, "Tracking Lost — Press R to reselect", (20, 40),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 0, 255), 2)

    # Simulated drone RC commands
    offset_x = cx - center_x
    offset_y = cy - center_y

    rc_yaw_cmd = int(rc_yaw + offset_x * gain_x)
    rc_pitch_cmd = int(rc_pitch - offset_y * gain_y)

    rc_yaw_cmd = max(1000, min(2000, rc_yaw_cmd))
    rc_pitch_cmd = max(1000, min(2000, rc_pitch_cmd))

    # Display simulated control
    cv2.putText(frame, f"Yaw RC:   {rc_yaw_cmd}", (20, frame_h - 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
    cv2.putText(frame, f"Pitch RC: {rc_pitch_cmd}", (20, frame_h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)

    cv2.imshow("Tracking + RC Simulation", frame)
    key = cv2.waitKey(1) & 0xFF

    if key == 27:  # ESC
        break
    elif key == ord('r'):
        cv2.putText(frame, "Select new object", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
        cv2.imshow("Tracking + RC Simulation", frame)
        new_bbox = cv2.selectROI("Re-select Object", frame, False)
        tracker = cv2.TrackerCSRT_create()
        tracker.init(frame, new_bbox)
        tracking = True

cap.release()
cv2.destroyAllWindows()
