# Python code for Multiple Color Detection

# color detecting masks code coppied from https://aishack.in/tutorials/tracking-colored-objects-opencv/
# QR detecting code directly copied from https://learnopencv.com/opencv-qr-code-scanner-c-and-python/
import numpy as np
import cv2


# Display barcode and QR code location
def display(im, bbox):
    # hardcoded with help of the debugger. Changed from the guys code referenced above
    # Is a tuple array. Acts like a 3 arrays.
    cv2.rectangle(im, (bbox[0][0][0], bbox[0][0][1]), (bbox[0][2][0], bbox[0][2][1]), (0, 0, 255), thickness=2)
    # Display results

    cv2.imshow("Results", im)


# Capturing video through webcam
webcam = cv2.VideoCapture(0)

# Start a while loop
while (1):

    # Reading the video from the
    # webcam in image frames
    _, imageFrame = webcam.read()
    _, imageFrame_qr = webcam.read()

    # Convert the imageFrame in
    # BGR(RGB color space) to
    # HSV(hue-saturation-value)
    # color space
    hsvFrame = cv2.cvtColor(imageFrame, cv2.COLOR_BGR2HSV)

    # Set range for yellow color. In CV Hue range is 0-179. Online the range is usually 0 -360 degrees. Note conversion
    # define mask Hue 40 - 60 degrees
    red_lower = np.array([20, 100, 100], np.uint8)
    red_upper = np.array([30, 255, 255], np.uint8)
    yel_mask = cv2.inRange(hsvFrame, red_lower, red_upper)

    # Morphological Transform, Dilation
    # for each color and bitwise_and operator
    # between imageFrame and mask determines
    # to detect only that particular color
    kernal = np.ones((5, 5), "uint8")

    # For yellow color
    yel_mask = cv2.dilate(yel_mask, kernal)
    res_red = cv2.bitwise_and(imageFrame, imageFrame, mask=yel_mask)

    # Creating contour to track yellow color
    contours, hierarchy = cv2.findContours(yel_mask,
                                           cv2.RETR_TREE,
                                           cv2.CHAIN_APPROX_SIMPLE)
    i = 1;

    for pic, contour in enumerate(contours):
        area = cv2.contourArea(contour)
        if (area > 300):
            # returns top corner then bottom right corner

            x, y, w, h = cv2.boundingRect(contour)
            print(f"BB{i} = {x},{y}::{x+w},{y+h}\n")
            
            imageFrame = cv2.rectangle(imageFrame, (x, y),
                                       (x + w, y + h),
                                       (0, 0, 255), 2)

            cv2.putText(imageFrame, "yellow Colour", (x, y),
                        cv2.FONT_HERSHEY_SIMPLEX, 1.0,
                        (0, 0, 255))
            i = i + 1 # count areas

    # now the QR part

    # qrDecoder = cv2.QRCodeDetector()
    #
    # # Detect and decode the qrcode
    # data, bbox, rectifiedImage = qrDecoder.detectAndDecode(imageFrame_qr)
    # if len(data) > 0:
    #     print("Decoded Data : {}".format(data))
    #
    #     display(imageFrame_qr, bbox)
    #     rectifiedImage = np.uint8(rectifiedImage);
    #     cv2.imshow("Rectified QRCode", rectifiedImage);
    # else:
    #     print("QR Code not detected")
    #     cv2.imshow("Results", imageFrame_qr)
    # Program Termination
    cv2.imshow("Multiple Color Detection in Real-TIme", imageFrame)
    if cv2.waitKey(10) & 0xFF == ord('q'):
        cv2.destroyAllWindows()
        break