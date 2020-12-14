import random
import time
import sys
import os

from PyQt5.QtWidgets import QApplication
from PyQt5.QtWidgets import QLabel, QWidget, QHBoxLayout, QVBoxLayout, QTextEdit, QPushButton
from PyQt5.QtCore import pyqtSignal, QObject, Qt

from paho.mqtt import client as mqtt_client

import json
# 读取配置信息
with open("./setting.json", "r") as f:
    setting = json.load(f)
    broker = setting['broker']
    port = setting['port']
    username = setting['client']
    password = setting['passwd']
    topic = setting['topic']

# 随机生成Client ID
client_id = f'ternimal-mqtt-{random.randint(0, 100)}'


class MQTT(QObject):
    """MQTT通信"""

    signal = pyqtSignal(str, str)

    def __init__(self, parent=None):
        super().__init__(parent)

    def _connect_mqtt(self) -> mqtt_client:
        def on_connect(client, userdata, flags, rc):
            if rc == 0:
                print("Connected to MQTT Broker!")
            else:
                print("Failed to connect, return code %d\n", rc)

        client = mqtt_client.Client(client_id)
        client.username_pw_set(username=username, password=password)
        client.on_connect = on_connect
        client.connect(broker, port)
        return client

    def _subscribe(self, client: mqtt_client):
        def on_message(client, userdata, msg):
            print(f"Received `{msg.payload.decode()}` from `{msg.topic}` topic")
            self.signal.emit(msg.topic, msg.payload.decode())

        client.subscribe(topic)
        client.on_message = on_message

    def mqtt_run(self):

        client = self._connect_mqtt()
        self._subscribe(client)
        client.loop_start()  # 开启新线程处理MQTT数据


class Window(QWidget):
    """窗口界面"""

    def __init__(self, parent=None):
        super().__init__(parent)
        self._ui_init()

    def _ui_init(self):
        """绘制UI"""
        self.setWindowTitle("MQTT")
        self.setFixedWidth(900)
        self.setFixedHeight(450)

        self.topic_wgt = QTextEdit()
        self.data_wgt = QTextEdit()

        self.clear_btn = QPushButton("clear")

        layout_h1 = QHBoxLayout()
        layout_h2 = QHBoxLayout()
        layoutV = QVBoxLayout()
        layout_h1.addWidget(QLabel("Topic:"))
        layout_h1.addWidget(QLabel("Payload:"))
        layout_h2.addWidget(self.topic_wgt)
        layout_h2.addWidget(self.data_wgt)
        layoutV.addLayout(layout_h1)
        layoutV.addLayout(layout_h2)
        layoutV.addWidget(self.clear_btn)

        self.clear_btn.clicked.connect(self.clear_data)
        self.setLayout(layoutV)

    def show_data(self, topic, payload):
        self.topic_wgt.append(topic)
        self.data_wgt.append(payload)
        pass

    def clear_data(self):
        self.topic_wgt.clear()
        self.data_wgt.clear()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    # 创建MQTT客户端
    client = MQTT()
    client.mqtt_run()
    # 显示界面
    Win = Window()
    Win.show()

    client.signal.connect(Win.show_data)  # 将MQTT数据接收信号和UI刷新槽关联
    sys.exit(app.exec())
