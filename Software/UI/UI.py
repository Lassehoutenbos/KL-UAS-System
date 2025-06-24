import sys
import folium
import io

from PyQt6.QtWidgets import (
    QApplication, QWidget, QPushButton, QLabel, QVBoxLayout,
    QHBoxLayout, QComboBox, QTextEdit, QTabWidget
)
from PyQt6.QtWebEngineWidgets import QWebEngineView


class GCS(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Drone GCS")
        self.setFixedSize(600, 400)

        # Example GCS location (latitude, longitude)
        self.home_lat = 52.6324
        self.home_lon = 4.7534

        self.tabs = QTabWidget()
        self.init_status_tab()
        self.init_map_tab()

        layout = QVBoxLayout()
        layout.addWidget(self.tabs)
        self.setLayout(layout)

    def init_status_tab(self):
        status_tab = QWidget()
        layout = QVBoxLayout()

        self.status_label = QLabel("Status: Disarmed")
        self.altitude_label = QLabel("Altitude: ---")
        self.gps_label = QLabel("GPS: ---")

        self.arm_button = QPushButton("Arm")
        self.arm_button.clicked.connect(self.toggle_arm)

        self.mode_selector = QComboBox()
        self.mode_selector.addItems(["Manual", "Altitude Hold", "Loiter"])

        self.log_output = QTextEdit()
        self.log_output.setReadOnly(True)

        hlayout = QHBoxLayout()
        hlayout.addWidget(self.arm_button)
        hlayout.addWidget(self.mode_selector)

        layout.addWidget(self.status_label)
        layout.addWidget(self.altitude_label)
        layout.addWidget(self.gps_label)
        layout.addLayout(hlayout)
        layout.addWidget(self.log_output)

        status_tab.setLayout(layout)
        self.tabs.addTab(status_tab, "Status")

    def init_map_tab(self):
        map_tab = QWidget()
        layout = QVBoxLayout()

        # Create map centered on home location
        m = folium.Map(location=[self.home_lat, self.home_lon], zoom_start=15)
        folium.Marker([self.home_lat, self.home_lon], tooltip="Home").add_to(m)

        # Render map as HTML
        data = io.BytesIO()
        m.save(data, close_file=False)

        # Display map in QWebEngineView
        web_view = QWebEngineView()
        web_view.setHtml(data.getvalue().decode())

        layout.addWidget(web_view)
        map_tab.setLayout(layout)
        self.tabs.addTab(map_tab, "Map")

    def toggle_arm(self):
        if self.status_label.text() == "Status: Disarmed":
            self.status_label.setText("Status: Armed")
            self.log_output.append("Drone armed.")
        else:
            self.status_label.setText("Status: Disarmed")
            self.log_output.append("Drone disarmed.")


if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = GCS()
    window.show()
    sys.exit(app.exec())
