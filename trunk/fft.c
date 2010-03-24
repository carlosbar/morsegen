/**
 * Module Description: fast fourier transform analysis of a signal
 *
 * Copyright © 2010 Carlos Barcellos
 *
 * $Id$
 * $Source$
 * $Revision$
 * $Date$
 * $Author$
 */
static const char rcsid [] = "@(#) $Id$ $RCSfile$$Revision$";

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI	3.14159265358979323846264338327

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
	int				samples=0,x,samplerate=44100,osamples;
	char			buf[512];
	double			*r=NULL,*i=NULL,amp,phase;
	int				nr=0,ni=0;

	if(argc < 2) {
		printf("fft <filename>\n");
		return(1);
	}
	f=fopen(argv[1],"r");
	if(!f) {
		printf("error opening file\n");
		return(1);
	}
	osamples=0;
	while(fgets(buf,sizeof(buf),f)) {
		if(*buf == ';') continue;
		r=(!osamples) ? malloc(sizeof(double)) : realloc(r,sizeof(double)*(osamples+1));
		i=(!osamples) ? malloc(sizeof(double)) : realloc(i,sizeof(double)*(osamples+1));
		r[osamples]=atof(buf);
		i[osamples]=0;
		osamples++;
	}
	fclose(f);
	samples=(int) pow(2,ceil(log(osamples)/log(2)));
	if(samples > osamples) {
		r=realloc(r,sizeof(double)*samples);
		i=realloc(i,sizeof(double)*samples);
		memset(&r[osamples],0,(samples-osamples)*sizeof(double));
		memset(&i[osamples],0,(samples-osamples)*sizeof(double));
	}
	FFT(1,(long) ceil(log(samples)/log(2)),r,i);
	for(x=0;x < samples/2;x++) {
		/* magnitude (positive amplitude) is calculate from abs(complex) == sqrt(real^2 + imag^2)*2 */
		amp=sqrt(r[x]*r[x]+i[x]*i[x])*2;
		/* phase (in radians) is calculate from atan(imag/real) */
		phase=(i[x]) ? 180/PI*atan(r[x]/i[x]) : 90;
		r[x]=amp;
		i[x]=phase;
	}
	f=fopen("fft.csv","w");
	if(f) fprintf(f,"freq (hz);amp;phase (degree)\n");
	for(x=0;f && x < samples/2;x++) {
		fprintf(f,"%d;%.2f;%.0f\n",(samplerate*x/samples),r[x],i[x]);
	}
	if(f) fclose(f);
	return(0);
}
