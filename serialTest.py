import serial
import json

x = {"awake": 1}
y = json.dumps(x)
y = y + "\n"

ser = serial.Serial("/dev/ttyAMA0", 9600, timeout=1)
ser.flush()
# print(y)
ser.write(y.encode("ascii"))
# ser.write("\n".encode("ascii"))
while True:
    line1 = ser.readline()
    print(line1)
    # line = line1.decode("utf-8")
    # print(line)
    line2 = line1.decode("utf-8".rstrip())
    print(line2)

    # line = ser.readline().decode("ascii")

    # data = json.loads(line)

    # data = json.loads(ser.readline())
    # print(data["key1"])
    # ser.write()
