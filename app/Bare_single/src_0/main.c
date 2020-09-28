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
void graySDF(uint8_t* img, int dim){
    int i = 0, j = 0, t;
    for (; i < dim; i++){
        t = img[j++] * 5;
        t+= img[j++] * 9;
        t+= img[j++] * 2;
        img[i] = t >> 4;
    }
}


void resizeSDF(uint8_t* img, int dimX, int dimY){
    int i, j;
    for (i = 0; i < dimY/2; i++){
        for (j = 0; j < dimX/2; j++){
            img[i*dimX/2 + j] = (img[i*2*dimX+j*2]+img[i*2*dimX+j*2+1]+img[(i*2+1)*dimX+j*2]+img[(i*2+1)*dimX+j*2+1])/4;
        }
    }
}

void brightnessSig(uint8_t* img, int dim, uint8_t* hmaxmin){
    int i;
    hmaxmin[0] = 0;
    hmaxmin[1] = 255;
    for (i = 0; i < dim; i++){
        if (img[i] > hmaxmin[0]) 
			hmaxmin[0] = img[i];
        if (img[i] < hmaxmin[1]) 
			hmaxmin[1] = img[i];
    }
}


void correctSDF( uint8_t* img, int dim, uint8_t* hmaxmin){
    
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
}

void sobelSDF(const uint8_t* img, uint8_t* result, int dimX, int dimY){
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
        }
    }
}

int main(){

    int average = 0, sum = 0;
	
    int i, dimX, dimY;
    for (i = 0; i < sequence_length; i++){
        D(printf("\n\n\n\e[0;42mimage #%d:\e[0m\n", i));
        uint8_t* img = image_sequence[i];
        dimX = img[0];
        dimY = img[1];
        img += 3;

		PERF_RESET(PERFORMANCE_COUNTER_0_BASE);
		PERF_START_MEASURING (PERFORMANCE_COUNTER_0_BASE);
		PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, Timer);
  
        graySDF(img, dimX * dimY);

        resizeSDF(img, dimX, dimY);

        uint8_t hmaxmin[2];
        brightnessSig(img, dimX * dimY >> 2, hmaxmin);

		unsigned char brt[3]={255,255,255};

        if( average < 128){
           
            D(printf("\e[0;32maverage=%d, ENABLE correntSDF\e[0m\n", average));
            correctSDF(img, dimX * dimY >> 2, hmaxmin);
       
        }else{
            D(printf("\e[0;31maverage=%d, DISABLE correntSDF\e[0m\n", average));
        }
		brt[sequence_length % 3]=hmaxmin[0]-hmaxmin[1];
		sum = (brt[0]+brt[1]+brt[2]);
		average = sum/3;
      //  sum  += (hmaxmin[0] - hmaxmin[1]);
       // average = sum/(i+1);

        uint8_t* result = (uint8_t*)malloc((dimX/2-2)*(dimY/2-2));
        sobelSDF(img, result, dimX/2, dimY/2);
        printAscii(result, dimX/2-2, dimY/2-2);

		PERF_END(PERFORMANCE_COUNTER_0_BASE, Timer);  

				/* Print report */
		perf_print_formatted_report
		(PERFORMANCE_COUNTER_0_BASE,            
		ALT_CPU_FREQ,        // defined in "system.h"
		1,                   // How many sections to print
		"Running time"        // Display-name of section(s).
		);   
    }
    

    return 0;
}
