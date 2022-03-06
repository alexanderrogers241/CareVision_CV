import numpy as np
import cv2

TOPL = [[200, 25]]
BOTR = [[460, 470]]
WIDTH =  BOTR[0][0] - TOPL[0][0]
HEIGHT = BOTR[0][1] - TOPL[0][1]
#
#       Pressure sensor array
#                        4X
#               TL *  -  -  -  -  |
#                  |  31 30 29 28 |
#                  |  27 26 25 24 |
#                  |  23 22 21 20 |
#                  |  19 18 17 16 |
#              8X  |  15 14 13 12 |
#                  |  11 10 9  8  |
#                  |  7  6  5  4  |
#                  |  3  2  1  0  |
#                  |  _  _  _  _  * BR
#
HEIGHT_SPACE = int(HEIGHT / 8)
WIDTH_SPACE = int(WIDTH / 4)
# Capturing video through webcam
webcam = cv2.VideoCapture(0)
while (1):

    # Reading the video from the
    # webcam in image frames
    _, imageFrame = webcam.read()

    cv2.rectangle(imageFrame, (TOPL[0][0], TOPL[0][1]), (BOTR[0][0], BOTR[0][1]), (0, 0, 255), thickness=2)
    # create boxes
    boxes = [None]
    # BOX_TOPL = (int(TOPL[0] + WIDTH_SPACE * (j - 1)), int(TOPL[1]))
    # BOX_BOTR = (int(TOPL[0] + WIDTH_SPACE * j), int(TOPL[1] + J_HEIGHT_SPACE))
    BOX_TOPL = [[TOPL[0][0], TOPL[0][1]]]
    BOX_BOTR = [[int(TOPL[0][0] + WIDTH_SPACE), int(TOPL[0][1] + HEIGHT_SPACE)]]

    for i in range(1, 9):
        for j in range(1, 5):
            
            BOX = [[BOX_TOPL[0][0], BOX_TOPL[0][1]], [BOX_BOTR[0][0], BOX_BOTR[0][1]]]
            cv2.rectangle(imageFrame, (BOX_TOPL[0][0], BOX_TOPL[0][1]), (BOX_BOTR[0][0], BOX_BOTR[0][1]),
                          (255, 0, 0), thickness=2)
            boxes.append(BOX)
            # X coord moves ---->
            BOX_TOPL[0][0] = BOX_TOPL[0][0] + WIDTH_SPACE
            BOX_BOTR[0][0] = BOX_BOTR[0][0] + WIDTH_SPACE

        # Y coord moves down
        BOX_BOTR[0][1] = BOX_BOTR[0][1] + HEIGHT_SPACE
        BOX_TOPL[0][1] = BOX_TOPL[0][1] + HEIGHT_SPACE
        # X coord are reset
        BOX_TOPL[0][0] = BOX_TOPL[0][0] - (4*WIDTH_SPACE)
        BOX_BOTR[0][0] = BOX_BOTR[0][0] - (4*WIDTH_SPACE)



    cv2.imshow("Multiple Color Detection in Real-TIme", imageFrame)

    if cv2.waitKey(10) & 0xFF == ord('q'):
        cv2.destroyAllWindows()
        break
