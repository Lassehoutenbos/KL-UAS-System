import cv2
import time

# Initialize video capture (0 = default internal webcam)
cap = cv2.VideoCapture(0)
if not cap.isOpened():
    print("❌ Cannot open camera")
    exit()

# Read first frame
ret, frame = cap.read()
if not ret:
    print("❌ Cannot read frame")
    cap.release()
    exit()

# Let user select the object to track
bbox = cv2.selectROI("Select Object to Track", frame, False)
cv2.destroyWindow("Select Object to Track")

# Initialize tracker (CSRT is accurate but slower; use KCF for faster)
tracker = cv2.TrackerCSRT_create()
tracker.init(frame, bbox)

# Start tracking loop
while True:
    ret, frame = cap.read()
    if not ret:
        break

    start = time.time()
    success, bbox = tracker.update(frame)
    end = time.time()

    if success:
        x, y, w, h = [int(i) for i in bbox]
        cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
        cv2.putText(frame, "Tracking", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 0), 2)
    else:
        cv2.putText(frame, "Lost", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 0, 255), 2)

    fps = 1 / (end - start)
    cv2.putText(frame, f"FPS: {int(fps)}", (20, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

    cv2.imshow("Object Tracking", frame)
    if cv2.waitKey(1) & 0xFF == 27:  # ESC to quit
        break

cap.release()
cv2.destroyAllWindows()
