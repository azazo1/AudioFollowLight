import socket

import scipy
import scipy.signal as signal
import pyaudiowpatch as pyaudio
import numpy as np
import time


# 音频随动

class Server:
    def __init__(self):
        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.address = ('', 12030)
        self.server.bind(self.address)
        print(f"Bind: {self.address}")
        self.server.listen(1)
        self.server.setblocking(False)
        self.clients = []
        self.ratio = 0

    def accept(self):
        try:
            sock, addr = self.server.accept()
            self.clients.append(sock)
            print('Accepted connection from {}'.format(addr))
        except BlockingIOError:
            pass

    def update(self):
        val = self.ratio * 255
        val = max(val, 0)
        val = min(val, 255)
        i = 0
        while i < len(self.clients):
            client = self.clients[i]
            try:
                client.setblocking(True)
                sent = f"{int(val):03d}\n"
                client.sendall(sent.encode('utf-8'))
                print("\rSend: ", sent.rstrip(), end="")
                client.setblocking(False)
                try:
                    get = client.recv(1024)
                    if not get:
                        raise Exception("Client disconnected")
                except BlockingIOError:
                    pass
            except Exception as e:
                print("Client disconnected")
                self.clients.remove(client)
                client.close()
                i -= 1
            i += 1


def low_pass_filter(data, cutoff_freq, sample_rate):
    """
    对数据应用低通滤波器
    """
    # 将数据转换为频域
    freq_data = scipy.fft.fft(data)

    # 创建频率轴
    freqs = np.fft.fftfreq(len(data), 1 / sample_rate)

    # 过滤高频分量
    freq_data[np.abs(freqs) > cutoff_freq] = 0

    # 转换回时域
    filtered_data = scipy.fft.ifft(freq_data).real

    return filtered_data

def main():
    with pyaudio.PyAudio() as p:
        """
        Create PyAudio instance via context manager.
        Spinner is a helper class, for `pretty` output
        """
        try:
            # Get default WASAPI info
            wasapi_info = p.get_host_api_info_by_type(pyaudio.paWASAPI)
        except OSError:
            print("Looks like WASAPI is not available on the system. Exiting...")
            exit()

        # Get default WASAPI speakers
        default_speakers = p.get_device_info_by_index(wasapi_info["defaultOutputDevice"])

        if not default_speakers["isLoopbackDevice"]:
            for loopback in p.get_loopback_device_info_generator():
                """
                Try to find loopback device with same name(and [Loopback suffix]).
                Unfortunately, this is the most adequate way at the moment.
                """
                if default_speakers["name"] in loopback["name"]:
                    default_speakers = loopback
                    break
            else:
                print(
                    "Default loopback output device not found.\nRun this to check available devices.\nExiting...\n")
                exit()

        print(f"Recording from: ({default_speakers['index']}){default_speakers['name']}")

        # waveFile = wave.open(filename, 'wb')
        # waveFile.setnchannels(default_speakers["maxInputChannels"])
        # waveFile.setsampwidth(pyaudio.get_sample_size(pyaudio.paInt16))
        # waveFile.setframerate(int(default_speakers["defaultSampleRate"]))

        int16max = 2 ** 15 - 1
        server = Server()
        CHUNK = 10
        SAMPLE_RATE = int(default_speakers["defaultSampleRate"])
        B, A = signal.butter(2, 1 / 10 / SAMPLE_RATE, fs=1 / SAMPLE_RATE, output='ba')


        def callback(in_data, frame_count, time_info, status):
            """Write frames and return PA flag"""
            data = np.frombuffer(in_data, dtype=np.int16)
            data = low_pass_filter(data, 1000, SAMPLE_RATE)
            # data = np.abs(signal.filtfilt(B, A, data))
            data = np.abs(data)
            ratio = np.max(data) / int16max
            ratio = ratio * 4
            # print('\r', "-" * int(ratio * 100) + "a", end="")
            # print('\r', str(data).replace('\n', ' '), end="")
            server.ratio = ratio
            return in_data, pyaudio.paContinue


        with p.open(format=pyaudio.paInt16,
                    channels=1,
                    rate=SAMPLE_RATE,
                    frames_per_buffer=CHUNK,  # pyaudio.get_sample_size(pyaudio.paInt16),
                    input=True,
                    input_device_index=default_speakers["index"],
                    stream_callback=callback
                    ) as stream:
            """
            Opena PA stream via context manager.
            After leaving the context, everything will
            be correctly closed(Stream, PyAudio manager)            
            """
            while True:
                time.sleep(0.1)
                server.accept()
                server.update()

if __name__ == "__main__":
    while True:
        try:
            main()
        except KeyboardInterrupt:
            time.sleep(3)