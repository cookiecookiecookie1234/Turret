from flask import Flask, request, render_template
import time
import numpy as np
import cv2 as cv
import sys
import socket
import threading
from mediapipe.tasks import python as mppython
from mediapipe.tasks.python import vision
import mediapipe as mp
face_classifier = cv.CascadeClassifier(
            cv.data.haarcascades + "haarcascade_frontalface_default.xml"
    )

base_options = mppython.BaseOptions(
    model_asset_path="pose_landmarker_lite.task"
)

options = vision.PoseLandmarkerOptions(
    base_options=base_options,
    running_mode=vision.RunningMode.IMAGE,
    num_poses=1
)

pose_landmarker = vision.PoseLandmarker.create_from_options(options)

DISCOVERY_PORT = 4210
HTTP_PORT = 5000
MAGIC = b"COOKIEMEOW"

def udp_discovery():
    while True:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("", DISCOVERY_PORT))

        print("UDP discovery listening...")
    
        waiting = True
        message = "nah"
        
        while waiting:
            data, addr = sock.recvfrom(256)
        
            if MAGIC in data :
                message = data.decode().split("/")[1]
            
                reply = f"TURRET_SERVER:{HTTP_PORT} Nyay!".encode()
                sock.sendto(reply, addr)
                waiting = False
    
        print("Turret connected!")
        print(f"Received: {message}")



app = Flask(__name__)
def faceDetectandDecode(img):
    np_img = np.frombuffer(img, np.uint8)
    grey_image = cv.imdecode(np_img, cv.IMREAD_GRAYSCALE)
    #image = cv.imread(imgpath)
    #grey_image = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
    
    face = face_classifier.detectMultiScale(
        grey_image, scaleFactor=1.03, minNeighbors=5, minSize=(40, 40)
    )
    if len(face) > 0:
        return "e0e0e1"
    else:
        return "e20e0e0"
    
def BodyDetectFromBytes(img):
    np_img = np.frombuffer(img, np.uint8)
    frame = cv.imdecode(np_img,cv.IMREAD_COLOR)
    torgb = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB,data=torgb)
    result = pose_landmarker.detect(mp_image)
    if not result.pose_landmarks:
        return "e0e0e0"
    lefthip = result.pose_landmarks[0][23]
    righthip = result.pose_landmarks[0][24]
    convx = str(int((lefthip.x + righthip.x)/2*frame.shape[1]))
    convy = str(int((lefthip.y + righthip.y) / 2 * frame.shape[0]))
    print("e"+convx+"e"+convy+"e1")
    return ("e"+convx+"e"+convy+"e1")
@app.route('/',methods=['GET', 'POST'])
def handleimage():
    if request.method == 'POST':
        print("skibidi")
        image = request.get_data()
        #with open("image.jpeg", 'wb') as myfile:
            #myfile.write(image)
        #return faceDetect("image.jpeg")
        return BodyDetectFromBytes(image)
    return render_template('serverpage.html')


if __name__ == '__main__':
    t = threading.Thread(target=udp_discovery, daemon=True)
    t.start()
    app.run(debug=True, use_reloader=False  , host="0.0.0.0",port=HTTP_PORT)