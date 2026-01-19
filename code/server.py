from flask import Flask, request, render_template
import time
import os
import cv2 as cv
import sys
import PIL as pil
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
        image = request.get_data()
        with open("image.jpeg", 'wb') as myfile:
            myfile.write(image)
        return faceDetect("image.jpeg")
    return render_template('serverpage.html')
if __name__ == '__main__':
    app.run(debug=True, host="0.0.0.0",port=5000)
    