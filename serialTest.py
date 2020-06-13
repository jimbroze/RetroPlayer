import serial
import json

x = {"key6": [1, 7, 5]}
y = json.dumps(x)
y = y + "\n"

ser = serial.Serial("/dev/ttyS0", 9600, timeout=1)
ser.flush()
# print(y)
# ser.write(y.encode("ascii"))
# ser.write("\n".encode("ascii"))
while True:
    line = ser.readline().decode("utf-8".rstrip())
    # data = json.loads(line)
    print(line)
    # data = json.loads(ser.readline())
    # print(data["key1"])
    # ser.write()
