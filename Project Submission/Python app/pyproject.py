import serial
import time
import tkinter as tk

window = tk.Tk()
window.configure(background = "gray14")
window.title("Sysc 3310 - MSP432 Course Project")

msp432 = serial.Serial('COM4', 9600)

def control() :
    def next( ):

        msp432.write(b'f')
        time.sleep(2)
        state = msp432.read(1).decode()
        print(">Toggled LED forward to the next state< State: " + str(state))

    def prev():

        msp432.write(b'b')
        time.sleep(2)
        state = msp432.read(1).decode()
        print(">Toggled LED back to the previous state< State: " + str(state))

    def quit() :
        print("\nawhhh you quit... Program has successfully Ended!")
        window.destroy()


    b1 = tk.Button(window, text = "Next", command = next, bg = "firebrick2", fg = "ghost white", font = ("Comic Sans MS", 20 ))
    b2 = tk.Button(window, text = "Prev", command = prev, bg = "dodger blue", fg = "ghost white", font = ("Comic Sans MS", 20 ))
    b3 = tk.Button(window, text = "Quit", command = quit, bg = "gold", fg = "ghost white", font = ("Comic Sans MS", 20 ))

    b1.grid(row = 1, column = 1, padx= 7, pady = 7)
    b2.grid(row = 1, column = 2, padx= 7, pady = 7)
    b3.grid(row = 1, column = 3, padx= 7, pady = 7)

    window.mainloop()

time.sleep(2)
control()