from flask import Flask, request, render_template
import time
import numpy as np
import cv2 as cv
face_classifier = cv.CascadeClassifier(
        cv.data.haarcascades + "haarcascade_frontalface_default.xml"
    )
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
        print(str([int(face[0,0]+(face[0,2]/2)),int(face[0,1]+(face[0,3]/2))]))
        return str([int(face[0,0]+(face[0,2]/2)),int(face[0,1]+(face[0,3]/2))])
    else:
        return "none"
    


@app.route('/',methods=['GET', 'POST'])
def handleimage():
    if request.method == 'POST':
        print("skibidi")
        image = request.get_data()
        #with open("image.jpeg", 'wb') as myfile:
            #myfile.write(image)
        #return faceDetect("image.jpeg")
        return faceDetectandDecode(image)
    return render_template('serverpage.html')
if __name__ == '__main__':
    app.run(host="0.0.0.0",port=5000,threaded=True)
    