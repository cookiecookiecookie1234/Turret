from flask import Flask, request, render_template_string, send_file
import numpy as np
import cv2 as cv
import socket
import threading
import io
from time import sleep
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
    model_asset_path="code/pose_landmarker_lite.task"
)

options = vision.PoseLandmarkerOptions(
    base_options=base_options,
    running_mode=vision.RunningMode.IMAGE,
    num_poses=1
)

pose_landmarker = vision.PoseLandmarker.create_from_options(options)

# ----------------------------


def BodyDetectFromBytes():
    camera = cv.VideoCapture(0)
    rect, frame = camera.read()
    camera.release()
    frame = cv.cvtColor(frame, cv.COLOR_BGR2RGB)
    h, w, _ = frame.shape
    center = vec2((w/2, h/2))
    mpimg = mp.Image(mp.ImageFormat.SRGB, frame.copy())
    result = pose_landmarker.detect(mpimg)
    
    
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
    


    cv.circle(frame, center.ti(), 100 * shoot, (160, 200, 255), -1)

    cv.line(frame, center.ti(), (center + (follow * vec2((10, 10)))).ti(), (0, 0, 255), thickness = 5)
    
    

    convx = (follow * vec2((10, 10))).ti()[0]
    convy = (follow * vec2((10, 10))).ti()[1]

    cv.imshow("iamgoinginsane",frame)
    cv.waitKey(1)
    sleep(10)
    return f"e{convx}e{convy}e{shoot}"
BodyDetectFromBytes()