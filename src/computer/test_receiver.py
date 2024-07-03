import socket

import pygame

server_addr = ('127.0.0.1', 12030)

pygame.init()


class Client:
    def __init__(self):
        self.client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client.connect(server_addr)
        self.client.setblocking(False)

    def getVal(self):
        recv = self.recv()
        if not recv:
            return 0
        data = recv.split(b'\n')[0]
        val = int(data)
        return val

    def recv(self) -> bytes:
        try:
            get = self.client.recv(1024)
            print(get)
            return get
        except BlockingIOError:
            return b""


screen = pygame.display.set_mode((640, 480))
cli = Client()

while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            exit()
    val = cli.getVal()
    if val != 0:
        screen.fill((val, val, val))
    pygame.display.update()
