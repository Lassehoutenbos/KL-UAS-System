import cv2
import time

# Initialize video
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("❌ Cannot open webcam")
    exit()

# Read first frame
ret, frame = cap.read()
if not ret:
    print("❌ Cannot read frame")
    cap.release()
    exit()

frame_h, frame_w = frame.shape[:2]
center_x, center_y = frame_w // 2, frame_h // 2

# Select ROI to track
bbox = cv2.selectROI("Select Object to Track", frame, False)
cv2.destroyWindow("Select Object to Track")

# Initialize tracker
tracker = cv2.TrackerCSRT_create()
tracker.init(frame, bbox)

# PID-like control settings (emulated)
gain_x = 0.3  # For yaw
gain_y = 0.3  # For pitch

# Neutral RC values
rc_yaw = 1500
rc_pitch = 1500

while True:
    ret, frame = cap.read()
    if not ret:
        break

    success, bbox = tracker.update(frame)

    if success:
        x, y, w, h = [int(i) for i in bbox]
        cx, cy = x + w // 2, y + h // 2

        # Draw tracking box
        cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
        cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)

        # Calculate offset
        offset_x = cx - center_x  # horizontal → yaw
        offset_y = cy - center_y  # vertical → pitch

        # Map to RC values (simulate RC override or MSP)
        rc_yaw_cmd = int(rc_yaw + offset_x * gain_x)
        rc_pitch_cmd = int(rc_pitch - offset_y * gain_y)

        # Clamp to RC range
        rc_yaw_cmd = max(1000, min(2000, rc_yaw_cmd))
        rc_pitch_cmd = max(1000, min(2000, rc_pitch_cmd))

        # Draw text
        cv2.putText(frame, f"Yaw Command:   {rc_yaw_cmd}", (20, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, f"Pitch Command: {rc_pitch_cmd}", (20, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
    else:
        cv2.putText(frame, "Tracking lost", (20, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)

    cv2.imshow("Live Tracking with Simulated Drone Commands", frame)

    if cv2.waitKey(1) & 0xFF == 27:  # ESC
        break

cap.release()
cv2.destroyAllWindows()
