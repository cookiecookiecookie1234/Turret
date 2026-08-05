from flask import Flask, request, render_template_string, send_file
import numpy as np
import cv2 as cv
import socket
import threading
import io

import mediapipe as mp
from mediapipe.tasks import python as mppython
from mediapipe.tasks.python import vision



class vec2:
    def __init__(self, t):
        self.x = t[0]
        self.y = t[1]
        
    def __add__(self, other):
        return vec2((self.x + other.x, self.y + other.y))
    
    def __sub__(self, other):
        return vec2((self.x - other.x, self.y - other.y))


    def __mul__(self, other):
        return vec2((self.x * other.x, self.y * other.y))
    
    def t(self):
        return (self.x, self.y)
    
    def ti(self):
        return (int(self.x), int(self.y))
        
    
    def magnetude(self):
        return np.sqrt(self.x * self.x + self.y * self.y)





POSE_LANDMARKS = {
    0:  "nose",
    1:  "left_eye_inner",
    2:  "left_eye",
    3:  "left_eye_outer",
    4:  "right_eye_inner",
    5:  "right_eye",
    6:  "right_eye_outer",
    7:  "left_ear",
    8:  "right_ear",
    9:  "mouth_left",
    10: "mouth_right",

    11: "left_shoulder",
    12: "right_shoulder",

    13: "left_elbow",
    14: "right_elbow",

    15: "left_wrist",
    16: "right_wrist",

    17: "left_pinky",
    18: "right_pinky",

    19: "left_index",
    20: "right_index",

    21: "left_thumb",
    22: "right_thumb",

    23: "left_hip",
    24: "right_hip",

    25: "left_knee",
    26: "right_knee",

    27: "left_ankle",
    28: "right_ankle",

    29: "left_heel",
    30: "right_heel",

    31: "left_foot_index",
    32: "right_foot_index",
}

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


lastT = None


def BodyDetectFromBytes(img_bytes, frame):
    global lastT
    
    torgb = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
    mp_image = mp.Image(
        image_format=mp.ImageFormat.SRGB,
        data=torgb
    )

    h, w, _ = frame.shape
    center = vec2((w/2, h/2))
    
    result = pose_landmarker.detect(mp_image)
    
    
    if not result.pose_landmarks:
        return "KEEP"

    

    e1 = None
    e2 = None
    m1 = None
    m2 = None
    n = None
    
    for i, lm in enumerate(result.pose_landmarks[0]):
        cc = lm.visibility
        cx = int(lm.x * w)
        cy = int(lm.y * h)
        
    
        if cc > 0.5:
            if i == 11:
                e1 = vec2((cx, cy)) # Elbow 1
                cv.circle(frame, (cx, cy), 4, (255 * cc, 255, 0), -1)
            if i == 12:
                e2 = vec2((cx, cy)) # Elbow 2
                cv.circle(frame, (cx, cy), 4, (255 * cc, 255, 0), -1)
            if i == 0:
                n = vec2((cx, cy)) # Nose
                cv.circle(frame, (cx, cy), 4, (255 * cc, 255, 0), -1)
            if i == 9:
                m1 = vec2((cx, cy)) # Mouth 1
                cv.circle(frame, (cx, cy), 4, (255 * cc, 255, 0), -1)
            if i == 10:
                m2 = vec2((cx, cy)) # Mouth 2
                cv.circle(frame, (cx, cy), 4, (255 * cc, 255, 0), -1)
            
        
            cv.circle(frame, (cx, cy), 2, (255 * cc, 0, 0), -1)
            
    
    
    target = None
    
    
    shootRadius = 160
    follow = vec2((0, 0))
    
    cv.circle(frame, center.ti(), 3, (255, 255, 255), -1)
    cv.circle(frame, center.ti(), shootRadius, (255, 255, 255))

    shoot = 0
    
    if e1 != None and e2 != None:
        target = (e1 + e2) * vec2((0.5, 0.5)) 
        cv.line(frame, e1.t(), e2.ti(), (255, 0, 0), thickness = 5)
        
        cv.circle(frame, target.ti(), 15, (0, 0, 255), -1)
        cv.circle(frame, target.ti(), 10, (255, 255, 255), -1)
        cv.circle(frame, target.ti(), 5, (0, 0, 255), -1)
        
        follow = (target - center) * vec2((0.04, 0.04))
        
        if lastT != None:
            speed = lastT - target
        
        lastT = target
        
        if (target - center).magnetude() < shootRadius:
            shoot = 1

            
        
    else:
        lastT = None


    cv.circle(frame, center.ti(), 100 * shoot, (160, 200, 255), -1)

    cv.line(frame, center.ti(), (center + (follow * vec2((10, 10)))).ti(), (0, 0, 255), thickness = 5)
    
    

    convx = (follow * vec2((10, 10))).ti()[0]
    convy = (follow * vec2((10, 10))).ti()[1]

    return f"e{convx}e{convy}e{shoot}"

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


    
<script>
setInterval(() => {
    document.getElementById("cam").src = "/frame?t=" + Date.now();
}, 50);
</script>
    
<body>
    <h2>Live Camera Feed</h2>
    <img id="cam" width="640">    


</body>
</html>

    """

lastCmd = None
n = 0

@app.route("/", methods=["POST"])
def receive_image():
    global last_frame, lastCmd, n

    data = request.get_data()
    if len(data) < 4:
        return "bad"

    w = (data[0] << 8) | data[1]
    h = (data[2] << 8) | data[3]

    img = np.frombuffer(data[4:], dtype=np.uint8).reshape((h, w))

    # keep grayscale for display
    gray = img.copy()

    # convert ONLY for MediaPipe
    frame = cv.cvtColor(gray, cv.COLOR_GRAY2BGR)

    result = BodyDetectFromBytes(None, frame)

    last_frame = gray

    if result == "KEEP":
        if lastCmd is None or n == 4:
            result = "e128e128e128"
            lastCmd = result
            n = 0
        else:
            n += 1
            result = lastCmd
    else:
        lastCmd = result
        n = 0

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
