#include <stdio.h>
#include <opencv2\opencv.hpp>
#include <iostream>
#include <array>


std::array<unsigned int, 32> PSA_DATA{0,1,2,3,
                                      4,5,6,7,
                                      8,9,10,11,
                                     12,13,14,15,
                                     16,17,18,19,
                                     20,21,22,23,
                                     24,25,26,27,
                                     28,29,30,31};

std::array<std::array<int, 2>, 1> TOPL = { {200, 25 } };
std::array<std::array<int, 2>, 1> BOTR = { {460, 470} };
int WIDTH = BOTR[0][0] - TOPL[0][0];
int HEIGHT = BOTR[0][1] - TOPL[0][1];

//#
//#                         Pressure sensor array
//#
//#                        4X                    4X
//#               TL * -------------|  TL * -------------|
//#                  |  31 30 29 28 |     |  31 30 29 28 |
//#                  |  27 26 25 24 |     |  27 26 25 24 |
//#                  |  23 22 21 20 |     |  23 22 21 20 |
//#                  |  19 18 17 16 |     |  19 18 17 16 |
//#               8X |  15 14 13 12 |  8X |  15 14 13 12 |
//#                  |  11 10 9  8  |     |  11 10  9  8 |
//#                  |  7  6  5  4  |     |   7  6  5  4 |
//#                  |  3  2  1  0  |     |   3  2  1  0 |
//#                  |  _  _  _  _  * BR  |  _  _  _  _  * BR 
//#
//#                    PSA layout #       PSA boxes index #

int HEIGHT_SPACE = int(HEIGHT / 8);
int WIDTH_SPACE = int(WIDTH / 4);
typedef std::array<std::array<int, 2>, 1> pt;
typedef std::array<std::array<int, 2>, 2> two_pts;

void drawx(pt& point, cv::Mat& frame)
{
    int spaceing{ 10 };

    cv::rectangle(frame,
        cv::Point(point[0][0] + spaceing, point[0][1]),
        cv::Point(point[0][0] - spaceing, point[0][1]),
        cv::Scalar(255, 0, 0),
        2,
        cv::LINE_8);

    cv::rectangle(frame,
        cv::Point(point[0][0], point[0][1] + spaceing),
        cv::Point(point[0][0], point[0][1] - spaceing),
        cv::Scalar(255, 0, 0),
        2,
        cv::LINE_8);

}

bool InsidePSA(std::array<std::array<int, 2>, 2>& box, std::array<std::array<int, 2>, 1>& point)
{
    // points start on the top right corner and traverse clockwise
    pt p1 = { TOPL[0] };
    pt p2 = { TOPL[0][0] + 4 * WIDTH_SPACE, TOPL[0][1] };
    pt p3 = { BOTR[0] };
    pt p4 = { BOTR[0][0] - 4 * WIDTH_SPACE, BOTR[0][1] };

    // point is plugged into 4 line equations that make up rectangle
    // if the line equation f(x,y) > 0 point is to the right of vector
    // if f(x,y) < 0 point is  to the left of vector
    // if f(x,y) == 0 point is on the line
    // traversing clockwise
    // d1 = p1------------------------------------------>p2
    int d1 = (p1[0][1] - p2[0][1]) * point[0][0] + (p2[0][0] - p1[0][0]) * point[0][1] + p1[0][0] * p2[0][1] - p2[0][0] * p1[0][1];
    // d2 = p2
    //       |
    //       V
    //      p3
    int d2 = (p2[0][1] - p3[0][1]) * point[0][0] + (p3[0][0] - p2[0][0]) * point[0][1] + p2[0][0] * p3[0][1] - p3[0][0] * p2[0][1];
    // d3 = p4<------------------------------------------p3
    int d3 = (p3[0][1] - p4[0][1]) * point[0][0] + (p4[0][0] - p3[0][0]) * point[0][1] + p3[0][0] * p4[0][1] - p4[0][0] * p3[0][1];
    // d4 = p1
    //       ^
    //       |
    //      p4
    int d4 = (p4[0][1] - p1[0][1]) * point[0][0] + (p1[0][0] - p4[0][0]) * point[0][1] + p4[0][0] * p1[0][1] - p1[0][0] * p4[0][1];

    if (d1 < 0)
    {
        // above line d1
        return false;
    }
    if (d2 <= 0)
    {
        // right and including line d2
        return false;
    }
    if (d3 <= 0)
    {
        // below including line d3
        return false;
    }
    if (d4 < 0)
    {
        // left of line d2
        return false;
    }

    // if passes previous tests point is inside box
    return true;
}

bool InsideBox(std::array<std::array<int, 2>, 2> box, std::array<std::array<int,2>,1> point)
{
    if (InsidePSA(box, point) == false) return false;
    // points start on the top right corner and traverse clockwise
    pt p1 = {box[0]};
    pt p2 = {box[0][0] + WIDTH_SPACE, box[0][1]};
    pt p3= {box[1]};
    pt p4 = {box[1][0] - WIDTH_SPACE, box[1][1]};

    // point is plugged into 4 line equations that make up rectangle
    // if the line equation f(x,y) > 0 point is to the right of vector
    // if f(x,y) < 0 point is  to the left of vector
    // if f(x,y) == 0 point is on the line
    // traversing clockwise
    // d1 = p1------------------------------------------>p2
    int d1 = (p1[0][1] - p2[0][1]) * point[0][0] + (p2[0][0] - p1[0][0]) * point[0][1] + p1[0][0] * p2[0][1] - p2[0][0] * p1[0][1];
    // d2 = p2
    //       |
    //       V
    //      p3
    int d2 = (p2[0][1] - p3[0][1]) * point[0][0] + (p3[0][0] - p2[0][0]) * point[0][1] + p2[0][0] * p3[0][1] - p3[0][0] * p2[0][1];
    // d3 = p4<------------------------------------------p3
    int d3 = (p3[0][1] - p4[0][1]) * point[0][0] + (p4[0][0] - p3[0][0]) * point[0][1] + p3[0][0] * p4[0][1] - p4[0][0] * p3[0][1];
    // d4 = p1
    //       ^
    //       |
    //      p4
    int d4 = (p4[0][1] - p1[0][1]) * point[0][0] + (p1[0][0] - p4[0][0]) * point[0][1] + p4[0][0] * p1[0][1] - p1[0][0] * p4[0][1];

    if (d1 < 0)
    {
        // above line d1
        return false;
    }
    if (d2 <= 0)
    {
        // right and including line d2
        return false;
    }
    if (d3 <= 0)
    {
        // below including line d3
        return false;
    }
    if (d4 < 0)
    {
        // left of line d2
        return false;
    }

    // if passes previous tests point is inside box
    return true;
}





bool PointToPressBox(std::vector<two_pts>& PSA_boxes, pt& point, int& out_index)
{

    for (int i = 0; i < PSA_boxes.size(); i++) {
        if (InsideBox(PSA_boxes[i], point))
        {
            // returns index 0 - 31 to represent which pressure sensor the point is over
            // to map to PSA layout # subtract 31
            out_index = i;
            return true;
        }
        
    }
    // return 101 value which means point is on edge of PSA box for a respective pressure sensor. Some edges while inside the PSA box are technically by edge case
    // outside of the smaller box for each pressure sensor
    return false;
}

int main()
{
    cv::Mat frame;
    cv::VideoCapture cap;
    cap.open(0);
    std::vector<two_pts> PSA_boxes;

    if (!cap.isOpened()) {
        std::cout << "ERROR! Unable to open camera\n";
        return -1;
    }

    for (;;)
    {
        // wait for a new frame from camera and store it into 'frame'
        cap.read(frame);
        // check if we succeeded
        if (frame.empty()) {
            std::cout << "ERROR! blank frame grabbed\n";
            break;
        }
        // cv.rectangle(imageFrame, (TOPL[0][0], TOPL[0][1]), (BOTR[0][0], BOTR[0][1]), (0, 0, 255), thickness = 2)
        cv::rectangle(frame,
            cv::Point(TOPL[0][0], TOPL[0][1]),
            cv::Point(BOTR[0][0], BOTR[0][1]),
            cv::Scalar(0, 0, 255),
            2,
            cv::LINE_8);

        //BOX_TOPL = [[TOPL[0][0], TOPL[0][1]]]
        //BOX_BOTR = [[int(TOPL[0][0] + WIDTH_SPACE), int(TOPL[0][1] + HEIGHT_SPACE)]]
        std::array<std::array<int, 2>, 1> BOX_TOPL = { {int(BOTR[0][0] - WIDTH_SPACE), int(BOTR[0][1] - HEIGHT_SPACE) } };
        std::array<std::array<int, 2>, 1> BOX_BOTR = { {BOTR[0][0], BOTR[0][1] } };
        

        /*for i in range(1, 9) :
            for j in range(1, 5) :

                BOX = [[BOX_TOPL[0][0], BOX_TOPL[0][1]], [BOX_BOTR[0][0], BOX_BOTR[0][1]]]
                cv2.rectangle(imageFrame, (BOX_TOPL[0][0], BOX_TOPL[0][1]), (BOX_BOTR[0][0], BOX_BOTR[0][1]),
                    (255, 0, 0), thickness = 2)
                boxes.append(BOX)
                # X coord moves---- >
                BOX_TOPL[0][0] = BOX_TOPL[0][0] + WIDTH_SPACE
                BOX_BOTR[0][0] = BOX_BOTR[0][0] + WIDTH_SPACE

                # Y coord moves down
                BOX_BOTR[0][1] = BOX_BOTR[0][1] + HEIGHT_SPACE
                BOX_TOPL[0][1] = BOX_TOPL[0][1] + HEIGHT_SPACE
                # X coord are reset
                BOX_TOPL[0][0] = BOX_TOPL[0][0] - (4 * WIDTH_SPACE)
                BOX_BOTR[0][0] = BOX_BOTR[0][0] - (4 * WIDTH_SPACE)*/


        for (int i{ 1 }; i < 9; i++)
        {
            for (int j{ 1 }; j < 5; j++)
            {
                // store box coord in list
                std::array<std::array<int, 2>, 2> BOX;
                BOX[0][0] = BOX_TOPL[0][0];
                BOX[0][1] = BOX_TOPL[0][1];
                BOX[1][0] = BOX_BOTR[0][0];
                BOX[1][1] = BOX_BOTR[0][1];
                PSA_boxes.push_back(BOX);
                // draw box
                cv::rectangle(frame,
                    cv::Point(BOX_TOPL[0][0], BOX_TOPL[0][1]),
                    cv::Point(BOX_BOTR[0][0], BOX_BOTR[0][1]),
                    cv::Scalar(0, 0, 255),
                    2,
                    cv::LINE_8);
                // Move Box X <---
                BOX_TOPL[0][0] = BOX_TOPL[0][0] - WIDTH_SPACE;
                BOX_BOTR[0][0] = BOX_BOTR[0][0] - WIDTH_SPACE;
            }
            // Move Box Y up
            BOX_BOTR[0][1] = BOX_BOTR[0][1] - HEIGHT_SPACE;
            BOX_TOPL[0][1] = BOX_TOPL[0][1] - HEIGHT_SPACE;
            // Reset Box X coord
            BOX_TOPL[0][0] = BOX_TOPL[0][0] + (4 * WIDTH_SPACE);
            BOX_BOTR[0][0] = BOX_BOTR[0][0] + (4 * WIDTH_SPACE);
        }
        int PressSenseIndex{ 101 };
        pt point = { 400,450 };
        drawx(point, frame);
        if (PointToPressBox(PSA_boxes, point, PressSenseIndex))
        {
            std::cout << PSA_DATA[PressSenseIndex] << "\n";
        } 

        // show live and wait for a key with timeout long enough to show images
        imshow("Live", frame);
        if (cv::waitKey(5) >= 0)
            break;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor

    return 0;
}