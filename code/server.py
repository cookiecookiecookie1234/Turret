from flask import Flask, request, render_template
from time import sleep
app = Flask(__name__)
image = ""
@app.route('/',methods=['GET', 'POST'])
def  handleimage():
    if request.method == 'POST':
        image = request.get_data()
        print(image)
        return "recieved"
    
    return render_template('serverpage.html')
if __name__ == '__main__':
    app.run(host="0.0.0.0",port=5000)
    with open("image.txt") as file:
        sleep(5)
        file.write(image)
        print("test")