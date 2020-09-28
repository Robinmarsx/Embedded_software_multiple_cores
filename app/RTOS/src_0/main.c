 #include <stdio.h>
#include "images.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ascii_gray.h"
#include <system.h>
#include <sys/alt_timestamp.h>
#include "altera_avalon_performance_counter.h"
#define abs(x) (((x)<0)?(-(x)):(x))
#ifdef NOINFO
#define D(x)
#else
#define D(x) x 
#endif
#define Timer 1

#define grayP 1
#define resizedP 2
#define brightnessP 3
#define correctP 4
#define sobelP 5
#define mooreP 6

OS_EVENT *aSemaphore;
OS_EVENT *bSemaphore;
OS_EVENT *cSemaphore;
OS_EVENT *dSemaphore;
OS_EVENT *eSemaphore;
OS_EVENT *fSemaphore;




#define   TASK_STACKSIZE       8192
OS_STK    task1_stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    task3_stk[TASK_STACKSIZE];
OS_STK    task4_stk[TASK_STACKSIZE];
OS_STK    task5_stk[TASK_STACKSIZE];
OS_STK    task6_stk[TASK_STACKSIZE];

void graySDF(uint8_t* img, int dim){

	INT8U err_aSemaphore;


	while(1){

	OSSemPend(aSemaphore,0,&err_aSemaphore);
    int i = 0, j = 0, t;
    for (; i < dim; i++){
        t = img[j++] * 5;
        t+= img[j++] * 9;
        t+= img[j++] * 2;
        img[i] = t >> 4;
    }
 	OSSemPost(bSemaphore);
	}
}


void resizeSDF(uint8_t* img, int dimX, int dimY){

	INT8U err_bSemaphore;
	
	while(1){

	OSSemPend(bSemaphore,0,&err_bSemaphore);
    int i, j;
    for (i = 0; i < dimY/2; i++){
        for (j = 0; j < dimX/2; j++){
            img[i*dimX/2 + j] = (img[i*2*dimX+j*2]+img[i*2*dimX+j*2+1]+img[(i*2+1)*dimX+j*2]+img[(i*2+1)*dimX+j*2+1])/4;
        }
    }
	OSSempost(cSemaphore);
}
}

void brightnessSig(uint8_t* img, int dim2, uint8_t* hmaxmin){

	INT8U err_cSemaphore;

	while(1){

	OSSemPend(cSemaphore,0,&err_cSemaphore);
    int i;
    hmaxmin[0] = 0;
    hmaxmin[1] = 255;
    for (i = 0; i < dim2; i++){
        if (img[i] > hmaxmin[0]) 
			hmaxmin[0] = img[i];
        if (img[i] < hmaxmin[1]) 
			hmaxmin[1] = img[i];
    }
	OSSempost(dSemaphore);
}
}



void mooreSDF ( int average, sum, uint8_t*hmaxmin){


	INT8U err_dSemaphore;

	while(1){
		 OSSemPend(dSemaphore,0,&err_dSemaphore);
			if( average < 128){
           
            D(printf("\e[0;32maverage=%d, ENABLE correntSDF\e[0m\n", average));
            correctSDF(img, dimX * dimY >> 2, hmaxmin);
        	}
			else{
            D(printf("\e[0;31maverage=%d, DISABLE correntSDF\e[0m\n", average));
        	}
		brt[sequence_image %3] = hmaxmin[0]-hmaxmin[1];
	
        sum = (brt[0]+brt[1]+brt[2]);

        average = sum/3;
		OSSempost(eSemaphore);
}
				
}
void correctSDF( uint8_t* img, int dim, uint8_t* hmaxmin){

	INT8U err_eSemaphore;

	while(1){

	OSSemPend(eSemaphore,0,&err_eSemaphore);
	
   
    int i, rescale;
    
    rescale = hmaxmin[0] - hmaxmin[1];
    
    for (i=0; i<dim ; i++){
        if (rescale > 127)
            return;
         if (rescale > 63)
            img[i] = (img[i] - hmaxmin[1])<<1;
        else if (rescale > 31)
             img[i] = (img[i] - hmaxmin[1])<<2;
        else if (rescale >15)
              img[i] = (img[i] - hmaxmin[1])<<3;
        else
             img[i] = (img[i] - hmaxmin[1])<<4;
    }
	OSSempost(fSemaphore);
}
}

void sobelSDF(const uint8_t* img, uint8_t* result, int dimX2, int dimY2){


	INT8U err_fSemaphore;

	while(1){
			OSSemPend(fSemaphore,0,&err_fSemaphore);
    int gx = 0, gy = 0, res = 0, cur = dimX - 2;
    int i, j;
    for (i = 1; i < dimY - 1; i ++){
        cur += 2;
        for (j = 1; j < dimX - 1; j++){
            cur ++;
            gx = 0;
            gx -=   img[ cur - dimX - 1 ];
            gx +=   img[ cur - dimX + 1 ];
            gx -=   img[ cur        - 1 ]<<1;
            gx +=   img[ cur        + 1 ]<<1;
            gx -=   img[ cur + dimX - 1 ];
            gx +=   img[ cur + dimX + 1 ];
            gx=abs(gx)-1;
            gy = 0;
            gy -=   img[ cur - dimX - 1 ];
            gy -=   img[ cur - dimX     ]<<1;
            gy -=   img[ cur - dimX + 1 ];
            gy +=   img[ cur + dimX - 1 ];
            gy +=   img[ cur + dimX     ]<<1;
            gy +=   img[ cur + dimX + 1 ];
            
            res = sqrt(gx*gx + gy*gy);
            result[(i-1)*(dimX-2) + j-1] = res;
			printAscii(result, dimX, dimY);

			PERF_END(PERFORMANCE_COUNTER_0_BASE, Timer);  

        }
    }
	OSSempost(aSemaphore);
}
}

int main(){

    int average = 0, sum = 0;
	unsigned char brt[3]={255,255,255};
    int i, dimX, dimY,dim,dim2,dimX2,dimY2;
    for (i = 0; i < sequence_length; i++){
        D(printf("\n\n\n\e[0;42mimage #%d:\e[0m\n", i));
        uint8_t* img = image_sequence[i];
        dimX = img[0];
		dimX2 = dimX/2;
        dimY = img[1];
		dimY2 = dimY/2;
		dim = dimX * dimY;
		dim2 =dimX * dimY >> 2;
		
        img += 3;
		uint8_t hmaxmin[2];
 		uint8_t* result = (uint8_t*)malloc((dimX/2-2)*(dimY/2-2));
		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, Timer);

		}
	OSTaskCreateExt(graySDF,
				  NULL,
				  (void *)&task1_stk[TASK_STACKSIZE-1],
				  grayP,
				  grayP,
				  task1_stk,
				  TASK_STACKSIZE,
				  NULL,
				  0);
	OSTaskCreateExt(resizedSDF,
				  NULL,
				  (void *)&task2_stk[TASK_STACKSIZE-1],
				  resizedP,
				  resizedP,
				  task1_stk,
				  TASK_STACKSIZE,
				  NULL,
				  0);
	OSTaskCreateExt(brightnessSig,
				  NULL,
				  (void *)&task3_stk[TASK_STACKSIZE-1],
				  brightnessP,
				  brightnessP,
				  task1_stk,
				  TASK_STACKSIZE,
				  NULL,
				  0);
	OSTaskCreateExt(mooreSDF,
				  NULL,
				  (void *)&task4_stk[TASK_STACKSIZE-1],
				  mooreP,
				  mooreP,
				  task1_stk,
				  TASK_STACKSIZE,
				  NULL,
				  0);
	OSTaskCreateExt(correctSDF,
				  NULL,
				  (void *)&task5_stk[TASK_STACKSIZE-1],
				  correctP,
				  correctP,
				  task1_stk,
				  TASK_STACKSIZE,
				  NULL,
				  0);
	OSTaskCreateExt(sobelSDF,
				  NULL,
				  (void *)&task6_stk[TASK_STACKSIZE-1],
				  sobelP,
				  sobelP,
				  task1_stk,
				  TASK_STACKSIZE,
				  NULL,
				  0);
					
  
       // graySDF(img, dimX * dimY);

       // resizeSDF(img, dimX, dimY);

       
      //  brightnessSig(img, dimX * dimY >> 2, hmaxmin);
	
        /*if( average < 128){
           
            D(printf("\e[0;32maverage=%d, ENABLE correntSDF\e[0m\n", average));
            correctSDF(img, dimX * dimY >> 2, hmaxmin);
       
        }else{
            D(printf("\e[0;31maverage=%d, DISABLE correntSDF\e[0m\n", average));
        }
        sum  += (hmaxmin[0] - hmaxmin[1]);
        average = sum/(i+1);
*/

		//mooreSDF(average,sum,hmaxmin);
        //uint8_t* result = (uint8_t*)malloc((dimX/2-2)*(dimY/2-2));
      //  sobelSDF(img, result, dimX/2, dimY/2);
       // printAscii(result, dimX/2-2, dimY/2-2);

	//	PERF_END(PERFORMANCE_COUNTER_0_BASE, Timer);  

		



			/* Print report */
	/*	perf_print_formatted_report
		(PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,        // defined in "system.h"
		1,                   // How many sections to print
		"Running time"        // Display-name of section(s).
		);    */

   // }
    aSemaphore = OSSemCreate(1);
	bSemaphore = OSSemCreate(0);
	cSemaphore = OSSemCreate(0);
	dSemaphore = OSSemCreate(0);
	eSemaphore = OSSemCreate(0);
	fSemaphore = OSSemCreate(0);

	OSStart();

    return 0;
}
