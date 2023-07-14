from threading import Thread
import socket
from time import sleep
from kivy.app import App
from kivy.uix.boxlayout import BoxLayout
# from kivy.core.window import Window
# Window.fullscreen = True


localaddr = ('', 2390)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# AF_INET = internet family addresses (IP), SOCK_DGRAM = udp datagram packets
sock.bind(localaddr)

rover_address = ['127.0.0.1', 2390]  # set to loopback address by default

controlBuffer = ["0", "0", "0", "0", "0", "0"]
controlCode = "ZDB"  # start sequence to verify that it is coming from the correct host
# Control array, corresponds to the 5 possible motor states and a sensor flag:
# sensor, forward, backward, left, right, sweep (in that order)


class ControlInterface(BoxLayout):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    @staticmethod
    def toggle_data():
        if controlBuffer[0] == "0":
            controlBuffer[0] = "1"
        else:
            controlBuffer[0] = "0"

    @staticmethod
    def move_forward():
        controlBuffer[2] = "0"
        controlBuffer[1] = "1"

    @staticmethod
    def move_backward():
        controlBuffer[1] = "0"
        controlBuffer[2] = "1"

    @staticmethod
    def move_left():
        controlBuffer[4] = "0"
        controlBuffer[3] = "1"

    @staticmethod
    def move_right():
        controlBuffer[3] = "0"
        controlBuffer[4] = "1"

    @staticmethod
    def forward_release():
        controlBuffer[1] = "0"

    @staticmethod
    def backward_release():
        controlBuffer[2] = "0"

    @staticmethod
    def left_release():
        controlBuffer[3] = "0"

    @staticmethod
    def right_release():
        controlBuffer[4] = "0"

    @staticmethod
    def toggle_sweep():
        if controlBuffer[5] == "0":
            controlBuffer[5] = "1"
        else:
            controlBuffer[5] = "0"

    def update_address(self):
        global rover_address
        address = self.ids["addressinput"].text
        port = self.ids["portinput"].text
        rover_address = [address, int(port)]


class rover(App):
    pass


def send():
    while True:
        try:
            msg = (controlCode + ''.join(controlBuffer)).encode(encoding='utf-8')
            sent = sock.sendto(msg, tuple(rover_address))  # address must be a tuple
        except Exception:
            continue
        if sent:
            print(msg)  # used for logging purposes
        sleep(0.1)  # note that this suspends execution of only this thread


def recv():
    while True:
        try:
            data, server = sock.recvfrom(1518)
            print("received: " + data.decode(encoding='utf-8'))
        except Exception:
            print("\nExit...\n")
            break
        sleep(0.1)


if __name__ == "__main__":
    sendThread = Thread(target=send)
    sendThread.setDaemon(True)

    recvThread = Thread(target=recv)
    recvThread.setDaemon(True)

    sendThread.start()
    recvThread.start()  # two threads, running concurrently.

    rover().run()
