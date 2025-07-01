import cv2
import numpy as np
import websocket

def on_message(ws, message):
    # message 是二进制（图像数据），用 numpy 和 OpenCV 解码
    img_array = np.frombuffer(message, dtype=np.uint8)
    img = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    if img is not None:
        cv2.imshow("Received Image", img)
        cv2.waitKey(1)
    else:
        print("图像解码失败")

def on_error(ws, error):
    print("错误:", error)

def on_close(ws, close_status_code, close_msg):
    print("连接关闭")

def on_open(ws):
    print("连接成功，等待图像...")

# 启动 WebSocket 客户端
ws = websocket.WebSocketApp(
    "ws://192.168.1.104:9000",  # ← 替换为你的图像服务器 IP 和端口
    on_open=on_open,
    on_message=on_message,
    on_error=on_error,
    on_close=on_close
)

# 持续接收
ws.run_forever()

