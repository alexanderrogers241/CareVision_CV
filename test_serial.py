import serial
# sudo chmod 666 /dev/ttyACM0 
# the above command unlocks the serial port.
# using pyserial library
# CAREVISION ARRAY size 32
data  = [0] * 32
with serial.Serial('/dev/ttyACM0', 2000000 , timeout = 10) as ser: # open serial port
    while True:
        ser.readinto(data)
        print(data)

