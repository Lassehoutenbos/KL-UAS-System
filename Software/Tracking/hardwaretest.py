# drone_ui_tracker.py

import sys
import cv2
import torch
import numpy as np
from PyQt5.QtWidgets import (QApplication, QMainWindow, QLabel, QVBoxLayout, QWidget,
                             QTabWidget, QPushButton, QHBoxLayout, QComboBox, QTextEdit)
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtCore import QTimer
from torchvision.transforms import Compose, Resize, ToTensor, Normalize
from PIL import Image

# Load MiDaS
midas = torch.hub.load("intel-isl/MiDaS", "MiDaS_small", trust_repo=True)
midas.eval()

transform = Compose([
    Resize((256, 256)),
    ToTensor(),
    Normalize(mean=[0.485, 0.456, 0.406],
              std=[0.229, 0.224, 0.225])
])

class DroneUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Drone Control Station")
        self.setGeometry(100, 100, 1000, 600)

        self.tabs = QTabWidget()
        self.setCentralWidget(self.tabs)

        self.camera_tab = QWidget()
        self.settings_tab = QWidget()
        self.status_tab = QWidget()

        self.tabs.addTab(self.camera_tab, "Camera Feed")
        self.tabs.addTab(self.settings_tab, "Settings")
        self.tabs.addTab(self.status_tab, "Drone Info")

        self.setup_camera_tab()
        self.setup_settings_tab()
        self.setup_status_tab()

        self.cap = cv2.VideoCapture(0)
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_frame)
        self.timer.start(30)

    def setup_camera_tab(self):
        layout = QVBoxLayout()
        self.camera_label = QLabel("Camera feed will appear here")
        layout.addWidget(self.camera_label)
        self.camera_tab.setLayout(layout)

    def setup_settings_tab(self):
        layout = QVBoxLayout()
        self.input_selector = QComboBox()
        self.input_selector.addItems(["Camera 0", "Camera 1", "Camera 2"])
        layout.addWidget(QLabel("Select Camera Input:"))
        layout.addWidget(self.input_selector)

        self.start_button = QPushButton("Start Tracking")
        layout.addWidget(self.start_button)

        self.settings_tab.setLayout(layout)

    def setup_status_tab(self):
        layout = QVBoxLayout()
        self.status_text = QTextEdit()
        self.status_text.setReadOnly(True)
        layout.addWidget(QLabel("Info Sent to Drone:"))
        layout.addWidget(self.status_text)
        self.status_tab.setLayout(layout)

    def update_frame(self):
        ret, frame = self.cap.read()
        if not ret:
            return

        # MiDaS depth estimation
        img_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        img_pil = Image.fromarray(img_rgb)
        input_tensor = transform(img_pil).unsqueeze(0)

        with torch.no_grad():
            prediction = midas(input_tensor)
            depth_map = prediction.squeeze().cpu().numpy()

        # Visual overlay (e.g., circle center)
        h, w, _ = frame.shape
        cx, cy = w // 2, h // 2
        depth_resized = cv2.resize(depth_map, (w, h))
        depth_at_center = depth_resized[cy, cx]
        cv2.circle(frame, (cx, cy), 10, (0, 255, 0), 2)
        cv2.putText(frame, f"Depth: {depth_at_center:.2f}", (cx + 10, cy),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

        # Send info to status tab
        self.status_text.setPlainText(f"Center depth: {depth_at_center:.2f} (relative units)")

        # Show frame
        rgb_image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgb_image.shape
        bytes_per_line = ch * w
        qt_image = QImage(rgb_image.data, w, h, bytes_per_line, QImage.Format_RGB888)
        pixmap = QPixmap.fromImage(qt_image)
        self.camera_label.setPixmap(pixmap)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = DroneUI()
    window.show()
    sys.exit(app.exec_())
# drone_ui_tracker.py

import sys
import cv2
import torch
import numpy as np
from PyQt5.QtWidgets import (QApplication, QMainWindow, QLabel, QVBoxLayout, QWidget,
                             QTabWidget, QPushButton, QHBoxLayout, QComboBox, QTextEdit)
from PyQt5.QtGui import QPixmap, QImage
from PyQt5.QtCore import QTimer
from torchvision.transforms import Compose, Resize, ToTensor, Normalize
from PIL import Image

# Load MiDaS
midas = torch.hub.load("intel-isl/MiDaS", "MiDaS_small", trust_repo=True)
midas.eval()

transform = Compose([
    Resize((256, 256)),
    ToTensor(),
    Normalize(mean=[0.485, 0.456, 0.406],
              std=[0.229, 0.224, 0.225])
])

class DroneUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Drone Control Station")
        self.setGeometry(100, 100, 1000, 600)

        self.tabs = QTabWidget()
        self.setCentralWidget(self.tabs)

        self.camera_tab = QWidget()
        self.settings_tab = QWidget()
        self.status_tab = QWidget()

        self.tabs.addTab(self.camera_tab, "Camera Feed")
        self.tabs.addTab(self.settings_tab, "Settings")
        self.tabs.addTab(self.status_tab, "Drone Info")

        self.setup_camera_tab()
        self.setup_settings_tab()
        self.setup_status_tab()

        self.cap = cv2.VideoCapture(0)
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_frame)
        self.timer.start(30)

    def setup_camera_tab(self):
        layout = QVBoxLayout()
        self.camera_label = QLabel("Camera feed will appear here")
        layout.addWidget(self.camera_label)
        self.camera_tab.setLayout(layout)

    def setup_settings_tab(self):
        layout = QVBoxLayout()
        self.input_selector = QComboBox()
        self.input_selector.addItems(["Camera 0", "Camera 1", "Camera 2"])
        layout.addWidget(QLabel("Select Camera Input:"))
        layout.addWidget(self.input_selector)

        self.start_button = QPushButton("Start Tracking")
        layout.addWidget(self.start_button)

        self.settings_tab.setLayout(layout)

    def setup_status_tab(self):
        layout = QVBoxLayout()
        self.status_text = QTextEdit()
        self.status_text.setReadOnly(True)
        layout.addWidget(QLabel("Info Sent to Drone:"))
        layout.addWidget(self.status_text)
        self.status_tab.setLayout(layout)

    def update_frame(self):
        ret, frame = self.cap.read()
        if not ret:
            return

        # MiDaS depth estimation
        img_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        img_pil = Image.fromarray(img_rgb)
        input_tensor = transform(img_pil).unsqueeze(0)

        with torch.no_grad():
            prediction = midas(input_tensor)
            depth_map = prediction.squeeze().cpu().numpy()

        # Visual overlay (e.g., circle center)
        h, w, _ = frame.shape
        cx, cy = w // 2, h // 2
        depth_resized = cv2.resize(depth_map, (w, h))
        depth_at_center = depth_resized[cy, cx]
        cv2.circle(frame, (cx, cy), 10, (0, 255, 0), 2)
        cv2.putText(frame, f"Depth: {depth_at_center:.2f}", (cx + 10, cy),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

        # Send info to status tab
        self.status_text.setPlainText(f"Center depth: {depth_at_center:.2f} (relative units)")

        # Show frame
        rgb_image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgb_image.shape
        bytes_per_line = ch * w
        qt_image = QImage(rgb_image.data, w, h, bytes_per_line, QImage.Format_RGB888)
        pixmap = QPixmap.fromImage(qt_image)
        self.camera_label.setPixmap(pixmap)

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = DroneUI()
    window.show()
    sys.exit(app.exec_())
