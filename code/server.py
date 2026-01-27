from flask import Flask, request, render_template_string, send_file
import numpy as np
import cv2 as cv
import socket
import threading
import io

import mediapipe as mp
from mediapipe.tasks import python as mppython
from mediapipe.tasks.python import vision

# -----------------------------
# MediaPipe setup
# -----------------------------
base_options = mppython.BaseOptions(
    model_asset_path="pose_landmarker_lite.task"
)

options = vision.PoseLandmarkerOptions(
    base_options=base_options,
    running_mode=vision.RunningMode.IMAGE,
    num_poses=1
)

pose_landmarker = vision.PoseLandmarker.create_from_options(options)

# -----------------------------
# Networking constants
# -----------------------------
DISCOVERY_PORT = 4210
HTTP_PORT = 5000
MAGIC = b"COOKIEMEOW"

# -----------------------------
# Globals
# -----------------------------
last_frame = None

# -----------------------------
# UDP discovery
# -----------------------------
def udp_discovery():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(("", DISCOVERY_PORT))

    print("UDP discovery listening...")

    while True:
        data, addr = sock.recvfrom(256)
        if MAGIC in data:
            reply = f"TURRET_SERVER:{HTTP_PORT}".encode()
            sock.sendto(reply, addr)
            print("Turret connected from", addr)

# -----------------------------
# Flask app
# -----------------------------
app = Flask(__name__)

def BodyDetectFromBytes(img_bytes, frame):
    torgb = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
    mp_image = mp.Image(
        image_format=mp.ImageFormat.SRGB,
        data=torgb
    )

    result = pose_landmarker.detect(mp_image)

    if not result.pose_landmarks:
        return "e0e0e0"
    lefthip = result.pose_landmarks[0][23]
    righthip = result.pose_landmarks[0][24]

    convx = int((lefthip.x + righthip.x) / 2 * w)
    convy = int((lefthip.y + righthip.y) / 2 * h)

    return f"e{convx}e{convy}e1"

# -----------------------------
# Routes
# -----------------------------
@app.route("/", methods=["GET"])
def index():
    return """
<!DOCTYPE html>
<html>
<head>
    <title>Turret Vision</title>
</head>


    
    
<body>
    <h2>Live Camera Feed</h2>
    <img id="cam" width="640">    


</body>
</html>

    """

@app.route("/", methods=["POST"])
def receive_image():
    global last_frame

    img_bytes = request.get_data()
    np_img = np.frombuffer(img_bytes, np.uint8)
    frame = cv.imdecode(np_img, cv.IMREAD_COLOR)

    if frame is None:
        return "decode_error"

    result = BodyDetectFromBytes(img_bytes, frame)

    last_frame = frame
    return result

@app.route("/frame")
def frame():
    global last_frame
    if last_frame is None:
        return "no frame", 404

    _, jpeg = cv.imencode(".jpg", last_frame)

    response = send_file(
        io.BytesIO(jpeg.tobytes()),
        mimetype="image/jpeg"
    )

    response.headers["Cache-Control"] = "no-store"
    return response



# -----------------------------
# Main
# -----------------------------
if __name__ == "__main__":
    t = threading.Thread(target=udp_discovery, daemon=True)
    t.start()

    app.run(
        host="0.0.0.0",
        port=HTTP_PORT,
        debug=False,
        use_reloader=False
    )
