from flask import Flask, request, render_template
import time
import os
import cv2 as cv
import sys
import PIL as pil
import socket
import threading

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
        message = "nah";
        
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
def faceDetect(imgpath):
    img = cv.imread(imgpath)
    grey_image = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
    face_classifier = cv.CascadeClassifier(
        cv.data.haarcascades + "haarcascade_frontalface_default.xml"
    )
    face = face_classifier.detectMultiScale(
        grey_image, scaleFactor=1.01, minNeighbors=5, minSize=(40, 40)
    )
    if face:
        return (face[0]+(face[2]/2),face[1]+(face[3]/2))
    else:
        return False
    
    
@app.route('/',methods=['GET', 'POST'])

def handleimage():
    if request.method == 'POST':
        print("image Receive!")
        image = request.get_data()
        with open("image.jpeg", 'wb') as myfile:
            myfile.write(image)
        return faceDetect("image.jpeg")
    return render_template('serverpage.html')


if __name__ == '__main__':
    
    
    t = threading.Thread(target=udp_discovery, daemon=True)
    t.start()
    app.run(debug=True, use_reloader=False  , host="0.0.0.0",port=HTTP_PORT)
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    