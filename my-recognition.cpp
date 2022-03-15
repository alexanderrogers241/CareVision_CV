#include <jetson-inference/poseNet.h>
#include <jetson-utils/videoSource.h>
#include <jetson-utils/videoOutput.h>
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

int main( int argc, char** argv )
{
	float4 color_red_test{252,0,0,252};
	float4 color_green_test{0,252,0,252};
	float4 color_yellow_test{252,252,0,252};
	// Serial object
    serialib serial;
	void *x_buff = malloc(sizeof(char) * 32);
    unsigned char *x_buff_type;
    void *y_buff = malloc(sizeof(char) *1);
    unsigned char handshake_start {'w'};
    void *z_buff = malloc(sizeof(char) *1);
    unsigned char *z_buff_type;
    // pressure sensor data
    std::array<unsigned int, 32> data {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,
										19,20,21,22,23,24,25,26,27,28,29,30,31};
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
    // if (errorOpening!=1) 
	// {
	// 	std::cout << "error\n";
	// 	return errorOpening;
	// }
	// else
	// {
    // 	printf ("Successful connection to %s\n",SERIAL_PORT);
	// }
    
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
	// posenet --input-width=640 --input-height=480 --input-flip=counterclockwise csi://0
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
	videoOutput* output = videoOutput::Create(cmdLine, ARG_POSITION(1));
	
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
    	// serial.writeBytes(y_buff,1);
		// serial.readBytes(x_buff,32,10);
		// x_buff_type = (unsigned char*) x_buff;
		puts("\n Buffer dump:");
		// convert to a vector
    	for(int y=0; y<(int)sizeof(char)*32; y++ )
    	{
			// data[y] = ( *(x_buff_type+y));
			// printf(" %02X",*(x_buff_type+y));
			
			// if(y < 17)
			// {
			// 	switch (data[y])
			// 	{
			// 	case 0:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 1:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 2:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 3:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 4:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 5:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 6:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 7:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 8:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 9:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			// 	case 10:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 11:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 12:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 13:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 14:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 15:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 16:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 17:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 18:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 19:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 20:
			// 		net->SetKeypointColor(y, color_yellow_test);
			// 	break;
			// 	case 21:
			// 		net->SetKeypointColor(y, color_green_test);
			// 	break;
			// 	default:
			// 		net->SetKeypointColor(y, color_red_test);
			// 	break;
			//	}
			//}
			
			
    	}
		puts("\n Coord dump:");
		if (poses.size()>0)
		{
			float coord_x = poses[0].Keypoints[0].x;
			float coord_y = poses[0].Keypoints[0].y;
			printf("Nose Coord: %4.1f,%4.1f",coord_x,coord_y);

		}

		puts("\n Array dump:");
		for(int y=0; y<(int)sizeof(char)*32; y++ )
		{
			printf(" %02u",data[y]);
		}
		for(int y=0; y<(int)sizeof(char)*32; y++ )
		{
			if (y <18)
			{	
				if (data[y] > 20)
				{
				net->SetKeypointColor(y, color_red_test);
				}
				else if (data[y] < 20 && data[y] >= 10)
				{
				net->SetKeypointColor(y, color_yellow_test);
				}
				else
				{
				net->SetKeypointColor(y, color_green_test);	
				}
			}
			
		}
			
		
		// render outputs
		if( output != NULL )
		{
			
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
