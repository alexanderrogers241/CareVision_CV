#include <jetson-inference/poseNet.h>
#include <jetson-utils/videoSource.h>
#include <jetson-utils/videoOutput.h>
#include <jetson-utils/cudaDraw.h>
#include <jetson-utils/imageIO.h>
#include <signal.h>
#include <cstdint>
// Serial library
#include "serialib.h"
#include <unistd.h>
#include <stdio.h>
// sudo chmod 666 /dev/ttyACM0


#if defined (_WIN32) || defined(_WIN64)
    //for serial ports above "COM9", we must use this extended syntax of "\\.\COMx".
    //also works for COM0 to COM9.
    //https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea?redirectedfrom=MSDN#communications-resources
    #define SERIAL_PORT "\\\\.\\COM1"
#endif
#if defined (__linux__) || defined(__APPLE__)
    #define SERIAL_PORT "/dev/ttyACM0"
#endif
/* 
  # print the pose results
    # print("detected {:d} objects in image".format(len(poses)))
    # [TRT]    poseNet -- keypoint 00 'nose'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 01 'left_eye'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 02 'right_eye'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 03 'left_ear'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 04 'right_ear'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 05 'left_shoulder'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 06 'right_shoulder'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 07 'left_elbow'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 08 'right_elbow'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 09 'left_wrist'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 10 'right_wrist'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 11 'left_hip'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 12 'right_hip'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 13 'left_knee'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 14 'right_knee'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 15 'left_ankle'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 16 'right_ankle'  color 0 255 0 255
    # [TRT]    poseNet -- keypoint 17 'neck'  color 0 255 0 255
    # [TRT]    poseNet -- loaded 18 class colors 
    # index 6 is right shoulder
    # Code below works fine
    # net.SetKeypointAlpha(alpha=20.0,keypoint=6) 

 */


// below array is a temprary test replaced with data[]

std::array<std::array<int, 2>, 1> TOPL{ {136,12} };
std::array<std::array<int, 2>, 1> BOTR{ {451,635} };
std::array<std::array<int, 2>, 2> PSA{ { { {136, 12} }, { { 451, 635} } } };
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
std::vector<two_pts> PSA_boxes;
float4 color_red_test{252,0,0,252};
float4 color_green_test{0,252,0,252};
float4 color_yellow_test{252,252,0,252};

void drawx(pt& point, uchar3* frame)
{
    int spaceing{ 10 };
	CUDA(cudaDrawLine(frame, 480, 640, point[0][0] + spaceing, point[0][1],  point[0][0] - spaceing, point[0][1], make_float4(255,0,200,200), 10));
	CUDA(cudaDrawLine(frame, 480, 640, point[0][0], point[0][1] + spaceing,  point[0][0], point[0][1] + spaceing, make_float4(255,0,200,200), 10));
}

void drawPSA(uchar3* frame)
{
	int i{0};

	for(const auto& box : PSA_boxes) 
	{  
		if(i%3==0)
		{
		CUDA(cudaDrawRect(frame, 480, 640, box[0][0], box[0][1], box[1][0], box[1][1], make_float4(0,0,255,20))); 
		}
		else if(i%3 == 1)
		{
		CUDA(cudaDrawRect(frame, 480, 640, box[0][0], box[0][1], box[1][0], box[1][1], make_float4(0,255,255,20)));	
		}
		else
		{
		CUDA(cudaDrawRect(frame, 480, 640, box[0][0], box[0][1], box[1][0], box[1][1], make_float4(255,255,255,20)));
		}
       

	   i++;
    }
}

bool InsidePSA(std::array<std::array<int, 2>, 1>& point)
{
    // points start on the top right corner and traverse clockwise
    pt p1 = { PSA[0] };
    pt p2 = { PSA[0][0] + 4 * WIDTH_SPACE, PSA[0][1] };
    pt p3 = { PSA[1] };
    pt p4 = { PSA[1][0] - 4 * WIDTH_SPACE, PSA[1][1] };

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
	//check to make sure point is on pressure sensor array
    if (InsidePSA(point) == false) return false;
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

bool PointToPressBox(pt& point, int& out_index)
{

    for (int i = 0; i < PSA_boxes.size(); i++) {
        if (InsideBox(PSA_boxes[i], point))
        {
            // returns index 0 - 31 to represent which pressure sensor the point is over
            out_index = i;
            return true;
        }
        
    }
    // return 101 value which means point is on edge of PSA box for a respective pressure sensor. Some edges while inside the PSA box are technically by edge case
    // outside of the smaller box for each pressure sensor
    return false;
}

bool signal_recieved = false;

void sig_handler(int signo)
{
	if( signo == SIGINT )
	{
		LogVerbose("received SIGINT\n");
		signal_recieved = true;
	}
}

int usage()
{
	printf("usage: posenet [--help] [--network=NETWORK] ...\n");
	printf("                input_URI [output_URI]\n\n");
	printf("Run pose estimation DNN on a video/image stream.\n");
	printf("See below for additional arguments that may not be shown above.\n\n");	
	printf("positional arguments:\n");
	printf("    input_URI       resource URI of input stream  (see videoSource below)\n");
	printf("    output_URI      resource URI of output stream (see videoOutput below)\n\n");

	printf("%s", poseNet::Usage());
	printf("%s", videoSource::Usage());
	printf("%s", videoOutput::Usage());
	printf("%s", Log::Usage());

	return 0;
}

void setup()
{
	std::array<std::array<int, 2>, 1> BOX_TOPL = { {int(BOTR[0][0] - WIDTH_SPACE), int(BOTR[0][1] - HEIGHT_SPACE) } };
	std::array<std::array<int, 2>, 1> BOX_BOTR = { {BOTR[0][0], BOTR[0][1] } };
    // build PSA_boxes based on parameters generated by python calibration scripts  
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
				
                // // draw box
                // cv::rectangle(frame,
                //     cv::Point(BOX_TOPL[0][0], BOX_TOPL[0][1]),
                //     cv::Point(BOX_BOTR[0][0], BOX_BOTR[0][1]),
                //     cv::Scalar(0, 0, 255),
                //     2,
                //     cv::LINE_8);

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




}

int main( int argc, char** argv )
{
	




	setup();

	// Serial object
    serialib serial;
	void *x_buff = malloc(sizeof(char) * 32);
    unsigned char *x_buff_type;
    void *y_buff = malloc(sizeof(char) *1);
    unsigned char handshake_start {'w'};
    void *z_buff = malloc(sizeof(char) *1);
    unsigned char *z_buff_type;
    // pressure sensor data
    std::array<unsigned int, 32> data;

    // place 'w' into the y_buff and same for z_buff
    memcpy(y_buff,&handshake_start,sizeof(unsigned char));
   
	// MEMORY ERROR CHECKING
    if( x_buff==NULL | y_buff==NULL |z_buff==NULL)
    {
        fprintf(stderr," Unable to allocate memory\n");
        exit(1);
    }
	// Connection to serial port
    char errorOpening = serial.openDevice(SERIAL_PORT, 115200);
    // If connection fails, return the error code otherwise, display a success message
    if (errorOpening!=1) 
	{
		std::cout << "error\n";
		return errorOpening;
	}
	else
	{
    	printf ("Successful connection to %s\n",SERIAL_PORT);
	}
    
	/*
	 * parse command line
	 */
	commandLine cmdLine(argc, argv);

	if( cmdLine.GetFlag("help") )
		return usage();


	/*
	 * attach signal handler
	 */
	if( signal(SIGINT, sig_handler) == SIG_ERR )
		LogError("can't catch SIGINT\n");


	/*
	 * create input stream
	 */
	//posenet --input-width=640 --input-height=480 --input-flip=counterclockwise csi://0

	
	videoOptions camsettings{};
	camsettings.flipMethod = videoOptions::FlipMethod::FLIP_COUNTERCLOCKWISE;
	camsettings.height = 480; // for some reason this is the width
	camsettings.width = 640;
	const char *str1 = "csi://0";
	videoSource* input = videoSource::Create(str1,camsettings);

	if( !input )
	{
		LogError("posenet: failed to create input stream\n");
		return 0;
	}


	/*
	 * create output stream
	 */
	videoOutput* output = videoOutput::Create(cmdLine,ARG_POSITION(1));
	
	if( !output )
		LogError("posenet: failed to create output stream\n");	
	

	/*
	 * create recognition network
	 */
	poseNet* net = poseNet::Create(cmdLine);
	
	if( !net )
	{
		LogError("posenet: failed to initialize poseNet\n");
		return 0;
	}

	// parse overlay flags
	const uint32_t overlayFlags = poseNet::OverlayFlagsFromStr(cmdLine.GetString("overlay", "links,keypoints"));
	


	bool key17InPSA{false};
	bool key6InPSA{false};
	bool key5InPSA{false};
	
	/*
	 * processing loop
	 */
	while( !signal_recieved )
	{
		// capture next image image
		uchar3* image = NULL;

		if( !input->Capture(&image, 1000) )
		{
			// check for EOS
			if( !input->IsStreaming() )
				break;

			LogError("posenet: failed to capture next frame\n");
			continue;
		}

		// run pose estimation
		std::vector<poseNet::ObjectPose> poses;
		
		if( !net->Process(image, input->GetWidth(), input->GetHeight(), poses, overlayFlags) )
		{
			LogError("posenet: failed to process frame\n");
			continue;
		}
		
		LogInfo("\n posenet: detected %zu %s(s)\n", poses.size(), net->GetCategory());

		// gather pressure sensor data
		//handshake is the char w = ascii = 0x77
    	serial.writeBytes(y_buff,1);
		serial.readBytes(x_buff,32,10);
		x_buff_type = (unsigned char*) x_buff;

		

		// transfer pressure sensor serial datat to a to an array
    	for(int y=0; y<(int)sizeof(char)*32; y++ )
    	{
			data[y] = ( *(x_buff_type+y));
			//printf(" %02X",*(x_buff_type+y));
    	}


		// puts("\n Coord dump:");
		// if (poses.size()>0)
		// {
		// 	float coord_x = poses[0].Keypoints[0].x;
		// 	float coord_y = poses[0].Keypoints[0].y;
		// 	printf("Nose Coord: %4.1f,%4.1f",coord_x,coord_y);
		// }

		puts("\n Array dump:\n");
		for(int y=0; y<(int)sizeof(char)*32; y++ )
		{
			printf(" %02u",data[y]);
		}
		puts("\n");
		// paint each keypoint a color
		// color represents pressure
		// copy to try to avoid a memory fault
		if(poses.empty() || poses[0].Keypoints.empty())
		{
			
		} else {
			
			

			for (auto it =poses[0].Keypoints.begin(); it!= poses[0].Keypoints.end(); ++it) {
				// out index
				int i{0};
				pt temp_pt {it->x,it->y};
				// i returns index to pressure sensor data 0-21 lbs
				if(it->ID == 17)
				{
					std::array<std::array<int,2>,1> temp_array{{it->x,it->y}};
					
					if(InsidePSA(temp_array))
					{
						key17InPSA = true;
					}
					else
					{
						key17InPSA = false;
					}
						
				}

				if(it->ID == 6)
				{
					std::array<std::array<int,2>,1> temp_array{{it->x,it->y}};
					
					if(InsidePSA(temp_array))
					{
						key6InPSA = true;
					}
					else
					{
						key6InPSA = false;
					}
						
				}

				if(it->ID == 5)
				{
					std::array<std::array<int,2>,1> temp_array{{it->x,it->y}};
					
					if(InsidePSA(temp_array))
					{
						key5InPSA = true;
					}
					else
					{
						key5InPSA = false;
					}
						
				}

				if(PointToPressBox(temp_pt,i))
				{
					if (data[i] > 15)
					{
						// j returns the coresponding keypoint
						
					net->SetKeypointColor(it->ID, color_red_test);
					//std::cout << "RED Key: " << j <<" Sense#: " << i <<" Press: " << data[i] <<"\n";
					}
					else if (data[i] < 20 && data[i] >= 10)
					{
					net->SetKeypointColor(it->ID, color_yellow_test);
					}
					else
					{
					net->SetKeypointColor(it->ID, color_green_test);	
					}
				}

				if(key17InPSA&&key5InPSA&&key6InPSA)
				{
					std::cout<<"\nOVERSIGHT WARNING: Patient is in bed\n";
				}
				else
				{
					std::cout<<"\nOVERSIGHT WARNING: Patient has left bed\n";
				}
			}
		}

		// render outputs
		if( output != NULL )
		{
			CUDA(cudaDrawRect(image, 480, 640, TOPL[0][0], TOPL[0][1], BOTR[0][0],BOTR[0][1], make_float4(0,0,255,20)));
			drawPSA(image);
			output->Render(image, input->GetWidth(), input->GetHeight());

			// update status bar
			char str[256];
			sprintf(str, "TensorRT %i.%i.%i | %s | Network %.0f FPS", NV_TENSORRT_MAJOR, NV_TENSORRT_MINOR, NV_TENSORRT_PATCH, precisionTypeToStr(net->GetPrecision()), net->GetNetworkFPS());
			output->SetStatus(str);	

			// check if the user quit
			if( !output->IsStreaming() )
				signal_recieved = true;
		}

		// print out timing info
		//net->PrintProfilerTimes();
	}
	
	
	/*
	 * destroy resources
	 */
	LogVerbose("posenet: shutting down...\n");
	
	SAFE_DELETE(input);
	SAFE_DELETE(output);
	SAFE_DELETE(net);
	
	LogVerbose("posenet: shutdown complete.\n");
	return 0;
}
