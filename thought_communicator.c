/* 
<Thought Communicator: Head Gaze+Gesture Communication System>
   Copyright (C) <2011>  <Vishwa>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
<thought_communicator.c>  Copyright (C) <2011>  <Vishwa>
*/


#include <stdio.h>
#include <stdlib.h>
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#include <math.h>

CvHaarClassifierCascade *cascade_f;
CvMemStorage *storage;

//functions
void detectFace(IplImage *img);
void load_GUI(void);

#define sum_value 10
#define n_value 15
#define	DRAW_TEXT(f, t, d, use_bg)								\
if (d)															\
{																\
	CvSize _size;												\
	cvGetTextSize(t, &font, &_size, NULL);						\
	if (use_bg)													\
	{															\
		cvRectangle(f, cvPoint(0, f->height), 					\
					cvPoint(_size.width + 5, 					\
						    f->height - _size.height * 2),		\
					CV_RGB(255, 0, 0), CV_FILLED, 8, 0);		\
	}															\
	cvPutText(f, t, cvPoint(2, f->height - _size.height / 2),	\
			  &font, CV_RGB(255,255,0));						\
	d--;														\
}
#define DRAW_RECTS(f, d, rw, ro)								\
do {															\
	cvRectangle(f, POINTS(rw), CV_RGB(255, 0, 0), 1, 8, 0);		\
	cvRectangle(f, POINTS(ro), CV_RGB(0, 255, 0), 1, 8, 0);		\
	cvRectangle(d, POINTS(rw), cvScalarAll(255),  1, 8, 0);		\
	cvRectangle(d, POINTS(ro), cvScalarAll(255),  1, 8, 0);		\
} while(0)

#define FRAME_WIDTH		240
#define FRAME_HEIGHT	180
#define TPL_WIDTH 		16
#define TPL_HEIGHT 		12
#define WIN_WIDTH		TPL_WIDTH * 2
#define WIN_HEIGHT		TPL_HEIGHT * 2
#define TM_THRESHOLD	0.4
#define STAGE_INIT		1
#define STAGE_TRACKING	2

#define POINT_TL(r)		cvPoint(r.x, r.y)
#define POINT_BR(r)		cvPoint(r.x + r.width, r.y + r.height)
#define POINTS(r)		POINT_TL(r), POINT_BR(r)
//global variables
int training = 1;
int threshold_x=0;
int threshold_y=0;
int iterator=0;
int array_fixation_x[sum_value]={0};
int array_fixation_y[sum_value]={0};
int centre_x;
int centre_y;
int current_group=2;
int group_value=0;	
int condition=0;	
char alpha[5][8]={"abcde","fghij","klmno","pqrst","uvwx*"};
char sentence[1000] = "1";
int where=0;
int prev_group=0;
int prev_group1=0;
int x=0;
int group=0;
CvFont			font;
char *GUI_path = "Image_orig.jpg";
IplImage * GUI_img = NULL;
int main(int argc, char ** argv)
{
	//declaration block
	IplImage * face_img1 = NULL;
	IplImage * face_img2 = NULL;
	IplImage * face_img3 = NULL;

	int end;  end = 0; int key; 
	CvCapture * face_capture = NULL; 
	IplImage * face_frame = NULL; 
	face_capture = cvCaptureFromCAM(0); 
	char *file1 = "haarcascade_frontalface_alt.xml";
	//char *file1 = "Eyes.xml";

	/* load the face classifier */
	cascade_f = (CvHaarClassifierCascade*)cvLoad(file1, 0, 0, 0);
	 
	/* setup memory storage, needed by the object detector */
	storage = cvCreateMemStorage(0);
	printf("\nYou are in the training mode.\nLook straight so that your centre of line of sight is captured for relative calculations\n");
	while(!end) 
	{	 
		cvGrabFrame (face_capture); 
		face_frame = cvRetrieveFrame(face_capture,0); 

		if(face_frame)
		{
			
			
			GUI_img = cvLoadImage(GUI_path, 1);	
		
			load_GUI();
			cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, .6, .6, 0, 1, 8);
//			DRAW_TEXT(face_frame,sentence,1,0);
			cvPutText(GUI_img, sentence, cvPoint(2,25),&font, CV_RGB(0,0,0));
			cvNamedWindow("face_frame",0 );
			cvNamedWindow("GUI",0 );
			
			/* always check */
			assert(cascade_f && storage && face_frame);

			/* detect face and do the required calculations */
			detectFace(face_frame);

			cvShowImage("face_frame",face_frame);
			cvShowImage("GUI",GUI_img);
		}
		key = cvWaitKey (50);
		if(key == 'q')
		end = 1;
		//deallocation block
		cvReleaseImage(&face_img1);
		cvReleaseImage(&face_img3);
		cvReleaseImage(&face_img2);
	}
	cvReleaseCapture(&face_capture);
	return 0;
 } //closing main()

void detectFace(IplImage *img)
{

	/* detect faces */
	CvSeq *faces = cvHaarDetectObjects(
		img, cascade_f, storage,
		1.1, 3, 0, cvSize( 40, 40 ) );

	/* return if not found */
	if (faces->total == 0) return;

	/* draw a rectangle */
	CvRect *r = (CvRect*)cvGetSeqElem(faces, 0);
	cvRectangle(img,
		cvPoint(r->x, r->y),
		cvPoint(r->x + r->width, r->y + r->height),
		CV_RGB(255, 0, 0), 1, 8, 0);

	int x1,y1,width1,height1;
	x1 = r->x;
	width1 = r->width;
	height1 = r->height;
	y1 = r->y;
	x1= (x1+width1)/2;
	y1=(y1+height1)/2;
//	printf("%d,%d\n",width1,height1);
	if (threshold_x==0)
	{
		threshold_x = width1/15;
		threshold_y = height1/15;
	}
	if (training==1)
	{

		iterator = iterator+1;
		array_fixation_x[iterator%sum_value]=x1;
		array_fixation_y[iterator%sum_value]=y1;
		int sum_x=0,sum_y=0;
		int i;
		for(i=0;i<sum_value;i++)
		{
			sum_x=array_fixation_x[i]+sum_x;
			sum_y=array_fixation_y[i]+sum_y;
		}
		int mean_x, mean_y, std_x, std_y;
		mean_x = sum_x/sum_value;
		mean_y = sum_y/sum_value;
		sum_x=0;
		sum_y=0;
		for(i=0;i<sum_value;i++)
		{
			sum_x=((mean_x - array_fixation_x[i])*(mean_x - array_fixation_x[i]))+sum_x;
			sum_y=((mean_y-array_fixation_y[i])*(mean_y-array_fixation_y[i]))+sum_y;
		}	
		printf("%d, %d, %d, %d \n", threshold_x, threshold_y, sum_x, sum_y);
		if (sum_x < threshold_x && sum_y<threshold_y)
		{
			printf("\nfixation achieved\n");


			training=0;
			centre_x = mean_x;
			centre_y = mean_y;      
			printf("x = %d, y = %d \n",centre_x,centre_y);
			iterator = 0;
		}
			
	}
	else
	{
		// calculate current group
		int x_diff;
		int y_diff;
		x_diff = x1 - centre_x;
		y_diff = y1 - centre_y;
//		threshold_x = threshold_x/2;
//		threshold_y = threshold_y/2;
		int neg_threshold_x = 0-threshold_x;
		int neg_threshold_y = 0-threshold_y;

		
		
		if(x_diff < neg_threshold_x && y_diff >= neg_threshold_y && y_diff <= threshold_y)
		{
			group = 6;
			group_value = 3;
			
		}
		
		if(x_diff >= neg_threshold_x && x_diff <=threshold_x && y_diff < neg_threshold_y)
		{
			group = 2;
			group_value = 0;
		}
		if(x_diff >= neg_threshold_x && x_diff <=threshold_x && y_diff >= neg_threshold_y && y_diff <= threshold_y)
		{
			group = 5;
			group_value = 2;
		}
		if(x_diff >= neg_threshold_x && x_diff <=threshold_x && y_diff > threshold_y)
		{
			group = 8;
			group_value = 4;
		}	
		if(x_diff > threshold_x && y_diff >= neg_threshold_y && y_diff <= threshold_y)
		{
			group = 4;
			group_value = 1;
		}
		if(group_value==current_group)
		{
			iterator = (iterator+1);
		}
		else
		{
			
			iterator = 0;
			current_group = group_value;
		}
		if(iterator>=15)
		{
			
			printf("-----------------------\nfixation achieved\n-----------------------\n");
			if(condition == 0)
			{
			condition=1;
			prev_group = current_group;
			prev_group1 = group;
			iterator=0;
			
			
			}
			else
			{
			sentence[where] = alpha[prev_group][current_group];
			where = where+1;
			printf("\n\n\n%s\n\n\n",sentence);

			condition=0;
			iterator=0;
			}
		}

		printf("%d\n",group);

		if(condition==1)
		{
			x=prev_group1;
		}
		else
			x=group;
		
		switch(x)
		{
			case 2: printf("-a-\nbcd\n-e-\n\n\n");break;
			case 4: printf("-f-\nghi\n-j-\n\n\n");break;
			case 5: printf("-k-\nlmn\n-o-\n\n\n");break;
			case 6: printf("-p-\nqrs\n-t-\n\n\n");break;
			case 8: printf("-u-\nvwx\n-*-\n\n\n");break;
			default: break;
		}
		
		
		
	}
	
}
void load_GUI()
{
	if(condition == 0)
	{
		if(group == 2)
			GUI_img = cvLoadImage("Image_2.jpg", 1);
		if(group == 4)
			GUI_img = cvLoadImage("Image_4.jpg", 1);
		if(group == 6)
			GUI_img = cvLoadImage("Image_6.jpg", 1);
		if(group == 8)
			GUI_img = cvLoadImage("Image_8.jpg", 1);
		if(group == 5)
			GUI_img = cvLoadImage("Image_5.jpg", 1);
	}

	else
	{
		if(prev_group1 == 2 && group == 2)
			GUI_img = cvLoadImage("Image_22.jpg", 1);
		if(prev_group1 == 2 && group == 4)
			GUI_img = cvLoadImage("Image_24.jpg", 1);
		if(prev_group1 == 2 && group == 5)
			GUI_img = cvLoadImage("Image_25.jpg", 1);
		if(prev_group1 == 2 && group == 6)
			GUI_img = cvLoadImage("Image_26.jpg", 1);
		if(prev_group1 == 2 && group == 8)
			GUI_img = cvLoadImage("Image_28.jpg", 1);
		if(prev_group1 == 4 && group == 2)
			GUI_img = cvLoadImage("Image_42.jpg", 1);
		if(prev_group1 == 4 && group == 4)
			GUI_img = cvLoadImage("Image_44.jpg", 1);
		if(prev_group1 == 4 && group == 5)
			GUI_img = cvLoadImage("Image_45.jpg", 1);
		if(prev_group1 == 4 && group == 6)
			GUI_img = cvLoadImage("Image_46.jpg", 1);
		if(prev_group1 == 4 && group == 8)
			GUI_img = cvLoadImage("Image_48.jpg", 1);
		if(prev_group1 == 5 && group == 2)
			GUI_img = cvLoadImage("Image_52.jpg", 1);
		if(prev_group1 == 5 && group == 4)
			GUI_img = cvLoadImage("Image_54.jpg", 1);
		if(prev_group1 == 5 && group == 5)
			GUI_img = cvLoadImage("Image_55.jpg", 1);
		if(prev_group1 == 5 && group == 6)
			GUI_img = cvLoadImage("Image_56.jpg", 1);
		if(prev_group1 == 5 && group == 8)
			GUI_img = cvLoadImage("Image_58.jpg", 1);
		if(prev_group1 == 6 && group == 2)
			GUI_img = cvLoadImage("Image_62.jpg", 1);
		if(prev_group1 == 6 && group == 4)
			GUI_img = cvLoadImage("Image_64.jpg", 1);
		if(prev_group1 == 6 && group == 5)
			GUI_img = cvLoadImage("Image_65.jpg", 1);
		if(prev_group1 == 6 && group == 6)
			GUI_img = cvLoadImage("Image_66.jpg", 1);
		if(prev_group1 == 6 && group == 8)
			GUI_img = cvLoadImage("Image_68.jpg", 1);
		if(prev_group1 == 8 && group == 2)
			GUI_img = cvLoadImage("Image_82.jpg", 1);
		if(prev_group1 == 8 && group == 4)
			GUI_img = cvLoadImage("Image_84.jpg", 1);
		if(prev_group1 == 8 && group == 5)
			GUI_img = cvLoadImage("Image_85.jpg", 1);
		if(prev_group1 == 8 && group == 6)
			GUI_img = cvLoadImage("Image_86.jpg", 1);
		if(prev_group1 == 8 && group == 8)
			GUI_img = cvLoadImage("Image_88.jpg", 1);

	}

}
