#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_HARMONICS	40

struct st_max {
	int		i;
	int		d;
};

/*
   This computes an in-place complex-to-complex FFT 
   x and y are the real and imaginary arrays of 2^m points.
   dir =  1 gives forward transform
   dir = -1 gives reverse transform 
*/
short FFT(short int dir,long m,double *x,double *y)
{
   long n,i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;

   /* Calculate the number of points */
   n = 1;
   for (i=0;i<m;i++) 
      n *= 2;

   /* Do the bit reversal */
   i2 = n >> 1;
   j = 0;
   for (i=0;i<n-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0; 
   c2 = 0.0;
   l2 = 1;
   for (l=0;l<m;l++) {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0; 
      u2 = 0.0;
      for (j=0;j<l1;j++) {
         for (i=j;i<n;i+=l2) {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1; 
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }
   return(1);
}

int main(int argc,char *argv[])
{
	FILE			*f;
	int				samples=0,x,y,z,samplerate=44100;
	char			buf[512];
	double			r[1 << 15],i[1 << 15];
	struct st_max	max[MAX_HARMONICS];

	if(argc < 2) {
		printf("fft <filename>\n");
		return(1);
	}
	f=fopen(argv[1],"r");
	if(!f) {
		printf("error opening file\n");
		return(1);
	}
	while(fgets(buf,sizeof(buf),f)) {
		if(*buf == ';') continue;
		r[samples]=atof(buf);
		i[samples]=0;
		if(++samples >= 1 << 15) break;
	}
	fclose(f);
	x=15;
	while(samples < 1 << x) x--;
	memset(max,0,sizeof(max));
	FFT(1,x,r,i);
	for(samples=0;samples < (1 << x)/2;samples++) {
		r[samples]=sqrt(r[samples]*r[samples]+i[samples]*i[samples]);
		for(y=0;y < MAX_HARMONICS;y++) {
			if(abs((int) r[samples]) > max[y].d) {
				for(z=MAX_HARMONICS-1;z > y;z--) {
					max[z].d=max[z-1].d;
					max[z].i=max[z-1].i;
				}
				max[y].d=abs((int) r[samples]);
				max[y].i=samples;
				break;
			}
		}
	}
	f=fopen("fft.csv","w");
	if(f) fprintf(f,"freq;amp\n");
	for(y=0;f && y < MAX_HARMONICS;y++) {
		fprintf(f,"%d;%d\n",max[y].i*samplerate/(1 << x),max[y].d);
	}
	if(f)	fclose(f);
	return(0);
}