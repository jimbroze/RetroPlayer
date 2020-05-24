import serial
import json

ser = serial.Serial("/dev/ttyS0", 9600, timeout=1)
ser.flush()
while True:
    line = ser.readline().decode("utf-8".rstrip())
    data = json.loads(line)
    print(line)
    # data = json.loads(ser.readline())
    print(data["key1"])
