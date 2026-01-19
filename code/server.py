from flask import Flask, request, render_template
import time
app = Flask(__name__)
@app.route('/',methods=['GET', 'POST'])
def handleimage():
    if request.method == 'POST':
        image = request.get_data()
        with open("image.jpeg", 'wb') as myfile:
            myfile.write(image)
        return "recieved"
    return render_template('serverpage.html')
if __name__ == '__main__':
    app.run(debug=True, host="0.0.0.0",port=5000)
    