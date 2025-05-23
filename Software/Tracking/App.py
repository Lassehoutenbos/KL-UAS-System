# drone_ui_tracker.py

import sys
import cv2
import torch
import numpy as np
import serial
import serial.tools.list_ports
import re
import csv
import folium
import io
import os
from PyQt5.QtWidgets import (QApplication, QMainWindow, QLabel, QVBoxLayout, QWidget,
                             QTabWidget, QPushButton, QComboBox, QTextEdit, QLineEdit, QMessageBox, QSizePolicy)
from PyQt5.QtGui import QPixmap, QImage, QFont
from PyQt5.QtCore import QTimer, Qt
from PyQt5.QtWebEngineWidgets import QWebEngineView
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

def parse_gps_from_serial(line):
    match = re.match(r"GPS:([-\d.]+),([-\d.]+)", line)
    if match:
        lat, lon = float(match.group(1)), float(match.group(2))
        return lat, lon
    return None

class DroneUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Drone Control Station")
        self.showFullScreen()
        self.setStyleSheet("font-size: 18px;")

        self.tabs = QTabWidget()
        self.tabs.setStyleSheet("QTabBar::tab { height: 50px; width: 150px; font-size: 20px; }")
        self.setCentralWidget(self.tabs)

        self.camera_tab = QWidget()
        self.settings_tab = QWidget()
        self.status_tab = QWidget()
        self.map_tab = QWidget()

        self.tabs.addTab(self.camera_tab, "📷 Camera")
        self.tabs.addTab(self.settings_tab, "⚙️ Settings")
        self.tabs.addTab(self.status_tab, "📡 Drone Log")
        self.tabs.addTab(self.map_tab, "🗺️ Map")

        self.scale_factor = 1.0
        self.object_bbox = None
        self.tracking = False
        self.serial_connection = None
        self.armed = False
        self.gps_coords = []

        self.setup_camera_tab()
        self.setup_settings_tab()
        self.setup_status_tab()
        self.setup_map_tab()

        self.cap = cv2.VideoCapture(0)
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_frame)
        self.timer.start(30)

        self.serial_timer = QTimer()
        self.serial_timer.timeout.connect(self.read_serial_data)
        self.serial_timer.start(200)

    def setup_camera_tab(self):
        layout = QVBoxLayout()
        self.camera_label = QLabel("Live camera feed")
        self.camera_label.setAlignment(Qt.AlignCenter)
        self.camera_label.setStyleSheet("border: 1px solid black")
        self.camera_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.camera_label.setScaledContents(True)

        self.select_btn = QPushButton("🎯 Select Object to Track")
        self.select_btn.setMinimumHeight(40)
        self.select_btn.clicked.connect(self.select_object)

        self.arm_btn = QPushButton("🔐 Arm/Disarm")
        self.arm_btn.setMinimumHeight(40)
        self.arm_btn.clicked.connect(self.toggle_arming)

        layout.addWidget(self.camera_label)
        layout.addWidget(self.select_btn)
        layout.addWidget(self.arm_btn)
        self.camera_tab.setLayout(layout)

    def setup_settings_tab(self):
        layout = QVBoxLayout()

        self.serial_selector = QComboBox()
        ports = serial.tools.list_ports.comports()
        for port in ports:
            self.serial_selector.addItem(port.device)
        layout.addWidget(QLabel("Select Serial Port:"))
        layout.addWidget(self.serial_selector)

        self.baud_selector = QComboBox()
        self.baud_selector.addItems(["9600", "19200", "38400", "57600", "115200"])
        layout.addWidget(QLabel("Select Baud Rate:"))
        layout.addWidget(self.baud_selector)

        self.connect_btn = QPushButton("Connect Serial")
        self.connect_btn.clicked.connect(self.connect_serial)
        layout.addWidget(self.connect_btn)

        self.settings_tab.setLayout(layout)

    def setup_status_tab(self):
        layout = QVBoxLayout()
        self.status_text = QTextEdit()
        self.status_text.setReadOnly(True)
        layout.addWidget(self.status_text)
        self.status_tab.setLayout(layout)

    def setup_map_tab(self):
        layout = QVBoxLayout()
        self.map_view = QWebEngineView()
        self.map_log = QTextEdit()
        self.map_log.setReadOnly(True)
        self.save_map_btn = QPushButton("💾 Save Map for Offline Use")
        self.save_map_btn.setMinimumHeight(40)
        self.save_map_btn.clicked.connect(self.save_map_offline)
        layout.addWidget(self.map_view)
        layout.addWidget(self.map_log)
        layout.addWidget(self.save_map_btn)
        self.map_tab.setLayout(layout)
        self.update_map()

    def select_object(self):
        ret, frame = self.cap.read()
        if not ret:
            return
        bbox = cv2.selectROI("Select Object", frame, False)
        if bbox:
            self.object_bbox = bbox
            self.tracking = True

    def toggle_arming(self):
        if self.serial_connection and self.serial_connection.is_open:
            if not self.armed:
                self.serial_connection.write(b"ARM\n")
                self.arm_btn.setText("🔓 Disarm")
                self.armed = True
            else:
                self.serial_connection.write(b"DISARM\n")
                self.arm_btn.setText("🔐 Arm")
                self.armed = False

    def connect_serial(self):
        port = self.serial_selector.currentText()
        baud = int(self.baud_selector.currentText())
        try:
            self.serial_connection = serial.Serial(port, baud, timeout=1)
            QMessageBox.information(self, "Serial Connected", f"Connected to {port} at {baud} baud.")
        except serial.SerialException as e:
            QMessageBox.critical(self, "Serial Error", str(e))

    def update_map(self):
        if not self.gps_coords:
            return
        lat, lon = self.gps_coords[-1]
        m = folium.Map(location=[lat, lon], zoom_start=18)
        for point in self.gps_coords:
            folium.CircleMarker(location=point, radius=6, color='blue').add_to(m)
        data = io.BytesIO()
        m.save(data, close_file=False)
        self.map_html = data.getvalue().decode()
        self.map_view.setHtml(self.map_html)

    def save_map_offline(self):
        try:
            os.makedirs("offline_maps", exist_ok=True)
            with open("offline_maps/gps_map.html", "w", encoding="utf-8") as f:
                f.write(self.map_html)
            QMessageBox.information(self, "Map Saved", "Offline map saved to offline_maps/gps_map.html")
        except Exception as e:
            QMessageBox.warning(self, "Save Error", str(e))

    def read_serial_data(self):
        if self.serial_connection and self.serial_connection.in_waiting:
            try:
                line = self.serial_connection.readline().decode('utf-8').strip()
                gps = parse_gps_from_serial(line)
                if gps:
                    self.gps_coords.append(gps)
                    self.map_log.append(f"Lat: {gps[0]:.6f}, Lon: {gps[1]:.6f}")
                    self.update_map()
                    with open("gps_log.csv", "a", newline="") as f:
                        csv.writer(f).writerow(gps)
            except Exception:
                pass

    def update_frame(self):
        ret, frame = self.cap.read()
        if not ret:
            return
        rgb_image = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        h, w, ch = rgb_image.shape
        bytes_per_line = ch * w
        qt_image = QImage(rgb_image.data, w, h, bytes_per_line, QImage.Format_RGB888)
        self.camera_label.setPixmap(QPixmap.fromImage(qt_image))

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = DroneUI()
    window.show()
    sys.exit(app.exec_())
