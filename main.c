// +-----------------------------------------------------------------------+
// |                                                                       |
// | This program estimate the block motion of video and calculate the     |
// | PSNR ratio. The original code is from Jet in our institute.           |
// |                                                                       |
// + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |                                                                       |
// | Author: Yong-Sheng Chen (yschen@iis.sinica.edu.tw)    1999. 5.17      |
// |         Institute of Information Science,                             |
// |         Academia Sinica, Taiwan.                                      |
// |                                                                       |
// +-----------------------------------------------------------------------+

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cvutil.h"

typedef unsigned char un_char;

char videoSource[1000];
char videoDestination[1000];
char videoSequence[80];
char videoOutput[80];
int  videoFrom;
int  videoTo;
int  countType;              // 0: .1 ;  1: .001
int  videoWidth;             // image size
int  videoHeight;
char videoInputName[1000];
char videoOutputName[1000];
int  blockSize;              // matching block size is blockSize^2
int  searchRange;            // search range is (2*searchRange+1)^2
double operationCount;       // number of operations
un_char **recons;            // reconstructed images with motion compensation
FILE *mvfp;                  // file storing motion vectors


void initialize();
void preprocessing();
void swap_image();
void motionEstimation();
double calculateMSE();


double motionCompensation(){
   int hRange, wRange, r, c, vr, vc, pcount=1;
   double mserr;

#ifdef ANALYSIS
   mserr = 0.0;
   pcount = 0;
#endif

   hRange = videoHeight-blockSize+1;
   wRange = videoWidth -blockSize+1;

   for(r=0; r<hRange; r+=blockSize)
   for(c=0; c<wRange; c+=blockSize){
      motionEstimation(r,c,&vr,&vc);
#ifdef ANALYSIS
      fprintf(mvfp, "%d %d\n",vc,vr);
      pcount += blockSize*blockSize;
      mserr += calculateMSE(r,c,vr,vc);
#endif
      } // for r,c
   return(mserr/pcount);
   }


void readVideorc(char *rcfname){
   FILE *fp;
   char temp[200];
  
   fp = fopen(rcfname,"rt");
  
   while(!feof(fp)){
      if(!fgets(temp,200,fp) || temp[0]=='\0' || temp[0]=='\n' || temp[0]==' ')
         continue;
      temp[strlen(temp)-1] = '\0';
      if (strncmp("source", temp, 6) == 0){
         strcpy(videoSource,temp+7);
         printf("video source: %s\n",videoSource);
         }
      else if(strncmp("destination", temp, 11) == 0){
         strcpy(videoDestination,temp+12);
         printf("video destination: %s\n",videoDestination);
         }
      else if(strncmp("sequence", temp, 8) == 0){
         strcpy(videoSequence,temp+9);
         printf("video sequence: %s\n",videoSequence);
         }
      else if(strncmp("output", temp, 6) == 0){
         strcpy(videoOutput,temp+7);
         printf("video output: %s\n",videoOutput);
         }
      else if(strncmp("from", temp, 4) == 0){
         videoFrom = atoi(temp+5);
         printf("video from: %d\n",videoFrom);
         }
      else if(strncmp("to", temp, 2) == 0){
        videoTo = atoi(temp+3);
        printf("video to: %d\n",videoTo);
        }
      else if(strncmp("type", temp, 4) == 0){
         countType = atoi(temp+5);
         printf("count type: %d\n",countType);
         }
      else if(strncmp("width", temp, 5) == 0){
         videoWidth = atoi(temp+6);
         printf("video width: %d\n",videoWidth);
         }
      else if(strncmp("height", temp, 6) == 0){
         videoHeight = atoi(temp+7);
         printf("video height: %d\n",videoHeight);
         }
      else if(strncmp("block", temp, 5) == 0){
         blockSize = atoi(temp+6);
         printf("block size: %d\n",blockSize);
         }
      else if(strncmp("range", temp, 5) == 0){
         searchRange = atoi(temp+6);
         printf("search range: %d\n", searchRange);
         }
      }
   fclose(fp);
   }


void getVideoName(int videoCurrent){
   if(countType == 0)  {
      sprintf(videoInputName,"%s%s.%d",videoSource,videoSequence,videoCurrent);
      sprintf(videoOutputName,"%s%s.%d.ppm",videoDestination,videoOutput,
	      videoCurrent);    
      }
   else{
      sprintf(videoInputName,"%s%s.%03d",videoSource,videoSequence,
	      videoCurrent);
      sprintf(videoOutputName,"%s%s.%03d.ppm",videoDestination,videoOutput,
	      videoCurrent);    
      }
   }


void readInputVideo(un_char **im){
   FILE *fp;
  
   fp = fopen(videoInputName, "rb");
   fread(im[0], videoHeight * videoWidth, 1, fp);
   fclose(fp);
   }


int main(int argc, char *argv[]){
   char rcfname[100];
   un_char **im;
   int videoCurrent, fcount;
   double nerror, terror = 0.0, psnr, time_cost0, time_cost1;
  
   if (argc != 2){
      fprintf(stderr, "Usage: %s videorc\n", argv[0]);
      exit(1);
      }
   sprintf(rcfname,"videorc.%s", argv[1]);
   readVideorc(rcfname);

#ifdef ANALYSIS
   mvfp = fopen("mvfile", "wb");
   operationCount=0.0;
   recons = create_image(videoWidth, videoHeight);
#endif
   im     = create_image(videoWidth, videoHeight);

   initialize();

#ifdef TIMER
   // start timers */
   my_timer(0,START);
   my_timer(0,PAUSE);
   my_timer(1,START);
   my_timer(1,PAUSE);
#endif

   getVideoName(videoFrom);
   readInputVideo(im);

#ifdef TIMER
   my_timer(0,PAUSE);
#endif

   preprocessing(im);

#ifdef TIMER
   my_timer(0,PAUSE);
#endif

  
   fcount = 0;
   for (videoCurrent=videoFrom+1; videoCurrent <= videoTo; videoCurrent++){
      swap_image();
      getVideoName(videoCurrent);
      printf("\nProcessing video fname: %s\n", videoOutputName);
      readInputVideo(im);
      fcount++;

#ifdef TIMER
      my_timer(0,PAUSE);
#endif

      preprocessing(im);

#ifdef TIMER
      time_cost0=my_timer(0,PAUSE);
      printf("Average preprocessing time:%f\n", time_cost0/fcount);

      my_timer(1,PAUSE);
#endif

      nerror = motionCompensation();

#ifdef TIMER
      time_cost1=my_timer(1,PAUSE);
      printf("Average motion estimation time:%f\n", time_cost1/fcount);
#endif

#ifdef ANALYSIS
      write_ppm_image(videoOutputName, recons, videoWidth, videoHeight, 0, 0);
      terror += nerror;
      psnr = 10.0*log10(255.0*255.0/nerror);
      printf("MSEError: %lg, PSNR: %lgdB\n", nerror, psnr);
      psnr = 10*log10(255.0*255.0*fcount / terror);
      printf("Average Compare Count:%d  PSNR: %lgdB\n", 
	     (int)(operationCount/fcount), psnr);
#endif
      }

#ifdef ANALYSIS
      fclose(mvfp);
#endif

#ifdef TIMER
   printf("Average total time:%f\n", (time_cost0+time_cost1)/fcount);
#endif
   }
