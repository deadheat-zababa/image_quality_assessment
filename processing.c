#include<math.h>
#include<getopt.h>
#include<stdio.h>
#include<stdlib.h>
#include<tiff.h>
#include<tiffio.h>
#include <string.h>
#include<stdint.h>
#include "quality_evaluation.h"
#include "processing.h"

int tiff_psnr_ssim(optinfo *info){
	pthread_mutex_lock(&info->mutex);
 unsigned int before_length = 0;
 unsigned int before_width = 0;
 unsigned int after_length = 0;
 unsigned int after_width = 0;
 unsigned int bitpersample_before = 0;
 unsigned int bitpersample_after = 0;
 unsigned int ch = 0;
 unsigned int *rgbaData_before = NULL;
 unsigned char *rgbData_before = NULL;
 unsigned int *rgbaData_after = NULL;
 unsigned char *rgbData_after = NULL;
 int i,j,c;
 int count = 0;
 double psnr=0;
 double sum_psnr = 0;
 double average_psnr = 0;
 double ssim=0;
 double sum_ssim=0;
 double average_ssim=0;
 float **y_before;
 float **y_after;
 TIFF *image1;
 TIFF *image2;
 uint16_t orientation1;
 uint16_t orientation2;
 int stride = 0;
 unsigned char *openfile1;
 unsigned char *openfile2;
 
 pthread_mutex_unlock(&info->mutex);
 pthread_t tid;
 
 tid = pthread_self();
 stride = (int)tid-1;
 printf("tid:%d\n",tid);
 fflush(stdout);

 openfile1 = (unsigned char *)malloc(sizeof(unsigned char)*(strlen(info->input1_name)+sizeof(int)+5));
 openfile2 = (unsigned char *)malloc(sizeof(unsigned char)*(strlen(info->input2_name)+sizeof(int)+5)); 

 for(count = info->start_number+stride; count<(info->frame_number+info->start_number); count+=info->thread_number){ 
  sprintf(openfile1,"%s%05d.tiff",info->input1_name,count);
  sprintf(openfile2,"%s%05d.tiff",info->input2_name,count);
  //printf("openfile%s\n",openfile1);
  fflush(stdout);
  image1 = TIFFOpen(openfile1,"r");
  image2 = TIFFOpen(openfile2,"r");

  if(image1==NULL||image2==NULL){
   printf("death\n");
   fflush(stdout);
   exit(1);
  }
		
 if(count==info->start_number+stride){
  //tiff画像の高さが同じか確認
  if((!TIFFGetField(image1,TIFFTAG_IMAGELENGTH,&before_length))||(!TIFFGetField(image2,TIFFTAG_IMAGELENGTH,&after_length))||(before_length!=after_length)){
   printf("TIFFGetField length error\n");
   TIFFClose(image1);
   TIFFClose(image2);
   exit(1);
 }
	if((!TIFFGetField(image1,TIFFTAG_IMAGEWIDTH,&before_width))||(!TIFFGetField(image2,TIFFTAG_IMAGEWIDTH,&after_width))||(before_width!=after_width)){
		printf("TIFFGetField width error\n");
		TIFFClose(image1);
		TIFFClose(image2);
		exit(1);
	}

	if((!TIFFGetField(image1,TIFFTAG_BITSPERSAMPLE,&bitpersample_before))||(!TIFFGetField(image2,TIFFTAG_BITSPERSAMPLE,&bitpersample_after))){
		printf("TIFFGetField bitpersample error\n");
		TIFFClose(image1);
		TIFFClose(image2);
		exit(1);
	}
			 

	rgbaData_before = (unsigned int*)malloc(sizeof(unsigned int)*before_width*before_length);
	rgbaData_after = (unsigned int*)malloc(sizeof(unsigned int)*after_width*after_length);

	if((rgbaData_before == NULL)||(rgbaData_after == NULL)){
	 printf("malloc rgbaData error\n");
	 free(rgbaData_before);
	 free(rgbaData_after);
	 TIFFClose(image1);
	 TIFFClose(image2);
	 exit(1);
	}
	rgbData_before = (unsigned char*)malloc(sizeof(unsigned char)*before_width*before_length*3);
	rgbData_after = (unsigned char*)malloc(sizeof(unsigned char)*after_width*after_length*3);

	if((rgbData_before==NULL)||(rgbData_after==NULL)){
	 printf("malloc rgbData error\n");
	 free(rgbData_before);
	 free(rgbData_after);
	 exit(1);
	}

			y_before = (float**)malloc(sizeof(float*)*before_length);
			y_after = (float**)malloc(sizeof(float*)*before_length);
    		for(i=0;i<before_length;i++){
				y_before[i] = (float*)malloc(sizeof(float)*before_width);
				y_after[i] = (float*)malloc(sizeof(float)*before_width);
			}

		}

		//printf("-------------------------------------------------------\n");
		if(!TIFFGetField(image1,TIFFTAG_ORIENTATION,&orientation1)){
			printf("image1 eeeor\n");
		}

		
		if(!TIFFGetField(image2,TIFFTAG_ORIENTATION,&orientation2)){
			printf("image2 eeeor\n");
		}

        if(info->show_flag==1){
		 printf("image1 orientation:%d\n",orientation1);
		 printf("image2 orientation:%d\n",orientation2);
		 fflush(stdout);
		}

		//init
		memset(rgbaData_after,0,sizeof(unsigned int)*before_length*before_width);
		memset(rgbaData_before,0,sizeof(unsigned int)*before_length*before_width);
		memset(rgbData_after,0,sizeof(unsigned char)*before_length*before_width*3);
		memset(rgbData_before,0,sizeof(unsigned char)*before_length*before_width*3);
		
		if((!TIFFReadRGBAImage(image1,before_width,before_length,rgbaData_before,0))||(!TIFFReadRGBAImage(image2,after_width,after_length,rgbaData_after,0))){
			printf("TIFFReadRGBAImage error\n");
			free(rgbaData_before);
			free(rgbaData_after);
			TIFFClose(image1);
			TIFFClose(image2);
			exit(1);
		}

        if(info->show_flag==1){
		 printf("height:%d,width:%d\n",before_length,before_width);
		 fflush(stdout);
		}

	for(j=0;j<before_length;j++){
	 for(i=0;i<before_width;i++){
	  y_before[j][i] = 0.299*(TIFFGetR(rgbaData_before[i + j * before_width]))+
						0.587*(TIFFGetG(rgbaData_before[i + j * before_width]))+
						0.114*(TIFFGetB(rgbaData_before[i + j * before_width]));
				

	  y_after[j][i] = 0.299*(TIFFGetR(rgbaData_after[i + j * before_width]))+
						0.587*(TIFFGetG(rgbaData_after[i + j * before_width]))+
						0.114*(TIFFGetB(rgbaData_after[i + j * before_width]));
	 }
	}
  
	if(info->mode == 1){
	 psnr = cal_psnr(y_before,y_after,before_length,before_width);
	 sum_psnr += psnr;
	 if(info->show_flag==1) printf("psnr = %lf\n",psnr);
     
	  pthread_mutex_lock(&info->mutex);
      info->psnr_value[count-info->start_number] = psnr;
      pthread_mutex_unlock(&info->mutex);
	}

	else if(info->mode ==2){
	 ssim = cal_ssim(y_before,y_after,before_length,before_width);
	 sum_ssim += ssim;
     if(info->show_flag==1) printf("ssim = %lf\n",ssim);
	 pthread_mutex_lock(&info->mutex);
     info->ssim_value[count-info->start_number] = ssim;
     pthread_mutex_unlock(&info->mutex);
    }
    else if(info->mode ==3){
     psnr = cal_psnr(y_before,y_after,before_length,before_width);
	 sum_psnr += psnr;
     ssim = cal_ssim(y_before,y_after,before_length,before_width);
	 sum_ssim += ssim;
	 if(info->show_flag==1){
      printf("psnr = %lf\n",psnr);
      printf("ssim = %lf\n",ssim);
     }
     pthread_mutex_lock(&info->mutex);
     info->psnr_value[count-info->start_number] = psnr;
     info->ssim_value[count-info->start_number] = ssim;
     pthread_mutex_unlock(&info->mutex);
    }

/*
	if(info->mode == 1) fprintf(info->outfile,"%d FRAME,%lf,\n",count,psnr);
    if(info->mode == 2) fprintf(info->outfile,"%d FRAME,%lf,\n",count,ssim);
	if(info->mode == 3) fprintf(info->outfile,"%d FRAME,%lf,%lf,\n",count,psnr,ssim);
 */
	TIFFClose(image1);
	TIFFClose(image2);
	fflush(stdout);
 }
 /*
    if(info->mode == 1 ||info->mode ==3) average_psnr = (sum_psnr/info->frame_number);
	if(info->mode == 2 ||info->mode ==3) average_ssim = (sum_ssim/info->frame_number);
    
	fprintf(info->outfile,"\n");
	if(info->mode == 1 ||info->mode ==3){
	 fprintf(info->outfile,"SUM(PSNR),%lf,\n",sum_psnr);
	 fprintf(info->outfile,"AVE(PSNR),%lf,\n",average_psnr);
	}


    if(info->mode == 2 ||info->mode ==3){
	 fprintf(info->outfile,"SUM(SSIM),%lf,\n",sum_ssim);
	 fprintf(info->outfile,"AVE(SSIM),%lf,\n",average_ssim);
}*/
if(info->frame_number>=60){
 if(info->mode ==1){
  pthread_mutex_lock(&info->mutex);
  info->psnr_value[info->frame_number] += sum_psnr;
  pthread_mutex_unlock(&info->mutex);
 }
 else if(info->mode==2){
  pthread_mutex_lock(&info->mutex);
  info->ssim_value[info->frame_number] += sum_ssim;
  pthread_mutex_unlock(&info->mutex);
 }
 else if(info->mode==3){
  pthread_mutex_lock(&info->mutex);
  info->psnr_value[info->frame_number] += sum_psnr;
  info->ssim_value[info->frame_number] += sum_ssim;
  //info->frame_number+info->start_number
  pthread_mutex_unlock(&info->mutex);
 }
}
 //-------------------TEST----------------------
 //test=stride;
 //printf("thread:%d--TEST_VALUE:%d",stride,test);
 //---------------------------------------------

 free(rgbaData_before);
 free(rgbaData_after);
 free(rgbData_before);
 free(rgbData_after);
 return 0;
}