#define LEGAL "\nCarlos Barcellos <carlosbar@gmail.com>\nMay the force be with you!\n"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "morse.h"

#ifdef _WIN32
#include <windows.h>
#define sleep(a) Sleep(a)
#define snprintf _snprintf
int startgui(HINSTANCE instance);
#endif

/**
	WAVE format is always formed by blocks (chunks), identified by a type and the size of remaining bytes of the block, as follows:

	<main wave header>				"RIFF"	- mandatory initial chunk header of a wave file
	<broadcast_audio_extension>		"bext"	- information on the audio sequence
	<fmt-ck>						"fmt"	- Format of the audio signal: PCM/MPEG
	<fact-ck>						"fact"	- Fact chunk is required for MPEG formats only
	<mpeg_audio_extension>			"mpeg"	- Mpeg Audio Extension chunk is required for MPEG formats only
	<wave-data>						"data"	- sound data

*/

/**
	struct st_chunk

	*@brief - common header for all chunk types

	type(4 bytes) - type of the block ("bext,"fmt ","fact","mpeg","data",etc)
	size(4 bytes) - integer little-endian - size of the remaining bytes of the block (excluding this header)
*/
struct st_chunk {
	char			type[4];				/* chunck type */
	unsigned int	size:32;				/* chunck size */
};

/* st_bext - broadcast information chunk */
struct st_bext {
	char			ckID[4];				/* (broadcastextension) ckID=bext */
	int				ckSize:32;				/* size of extension chunk */
	char			Description[256];		/* ASCII : «Description of the sound sequence» */
	char			Originator[32];			/* ASCII : «Name of the originator» */
	char			OriginatorReference[32];/* ASCII : «Reference of the originator» */
	char			OriginationDate[10];	/* ASCII : «yyyy-mm-dd» */
	char			OriginationTime[8];		/* ASCII : «hh-mm-ss» */
	char			TimeReferenceLow;		/* First sample count since midnight low word */
	char			TimeReferenceHigh;		/* First sample count since midnight, high word */
	char			Version;				/* Version of the BWF; unsigned binary number */
	char			UMID_0;					/* Binary byte 0 of SMPTE UMID */
	unsigned char	UMID_63;				/* Binary byte 63 of SMPTE UMID */
	unsigned char	Reserved[190];			/* 190 bytes, reserved for future use, set to NULL */
};

/* fmt chunk - describes the sound data's format */
struct st_fmt {
	struct st_chunk	header;					/* FMT subchunk Header (type == "fmt ") - subchunk describes the sound data's format, size is 16 for PCM. */
	short			audioformat;			/* PCM = 1 (i.e. Linear quantization) Values other than 1 indicate some form of compression.*/
	short			numchannels;			/* Mono = 1, Stereo = 2, etc.*/
	int				samplerate;				/* 8000, 44100, etc.*/
	int				byterate;				/* SampleRate * NumChannels * BitsPerSample/8*/
	short			blockalign;				/* NumChannels * BitsPerSample/8 - The number of bytes for one sample including all channels.*/
	short			bitspersample;			/* number of bits per channel: 8, 16, 32, etc.*/
};
#define SZ_PCM (sizeof(struct st_fmt)-offsetof(struct st_fmt,audioformat))

/* data chunk - contains the raw data sound */
struct st_data {
	struct st_chunk	header;					/* DATA subchunk  (type == "data"), size if the bytes composing the sound */
	char			raw[1];					/* The actual sound data. */
};

/* wav header chunk */
struct st_wav {
	struct st_chunk	header;					/* Main Chunk Header (type == "RIFF"), size contain the size of whole file excluding the header */
	char			format[4];				/* Contains the letters "WAVE" */
};
#define SZ_WAV sizeof(struct st_wav)

/* constants */

#define PHASE				0		/* initial fase (in degrees 0..360) */
#define BITS_IN_BYTE		8
#define SAMPLE_RATE			44100
#define BITS_PER_SAMPLE		16
#define PCM_TYPE			16
#define TIME_BASE			.03	/* seconds */
#define	FREQ_FUNDAMENTAL	3500

/**
	harmonics used for the modulation
*/
struct st_harm {
	int		freq;
	int		vol;
	double	phase;
};

/**
	convert from degree to radian
*/
double radian(double degree)
{
#define PI ((double) 3.14159265)
	return(PI/180*degree);
}

/**
	get sine value for the current degree
*/
double point(double degree,double amp)
{
	return(amp*sin(radian(degree)));
}

/**
	convert dots (1) and dashes (3) to total time needed
*/
double gettotaltime(char *msg,double time_base)
{
	char	buf[512];
	char	*p;
	double	t=0,d;

	snprintf(buf,sizeof(buf),msg);
	p=strtok(buf,",");
	while(p) {
		d=atof(p);
		if(d < 0) d=-d;
		/* key duration + interval after key press */
		t+=d*time_base+time_base;
		p=strtok(NULL,",");
	}
	return(t);
}

/**
	* generates wav file
	* @param msg        plain text message
	* @param freq       fundamental frequency
	* @param phase      initial phase
	* @param time_base  time base used for morse code modulation
	* @param bps        bits per sample
	* @param samplerate sample rate
	* @param np         callback function - called whenever a new wave point is to be draw
	* @param wd         callback function - called whever the wav file needs to be written
	* @param usrptr     user pointer informed to callback functions
	* @return - 0 for success or an error code
*/
int generate(char *msg,int freq,int phase,double time_base,int bps,int samplerate,NEWPOINT np,WAVEDATA wd,void *usrptr)
{
	int				x,y,w;
	struct st_fmt	fmt,tfmt={0};
	struct st_wav	wave;
	struct st_chunk	chunk;
	double			sample,v;
	char			buf[512],*p;
	double			ttime,cycles;
	int				t,mute,interval,rawsize,totaltime;
	int				med,end;
	struct st_harm	harmonics[12];	/* number of harmonics to add to fundamental wave */
	int				max_volume=((1<<(bps-1))-1);

	/* get morse code message */
	morsecode(msg,buf,sizeof(buf));
	ttime=gettotaltime(buf,time_base);
	/* header */
	totaltime=(int) (ttime*samplerate);
	rawsize=totaltime*bps/BITS_IN_BYTE;
	wave.header.size=sizeof(struct st_wav)+sizeof(struct st_fmt)+sizeof(struct st_bext)+rawsize;
	memcpy(wave.header.type,"RIFF",sizeof(wave.header.type));
	memcpy(wave.format,"WAVE",sizeof(wave.format));
	if(wd) wd(im_start,0,0,usrptr);
	if(np) np(im_start,0,0,0,0,samplerate,bps,totaltime,usrptr);
	/* set harmonic values */
	for(x=0;x < sizeof(harmonics)/sizeof(harmonics[0]);x++) {
		harmonics[x].freq=freq*(x+1);
		harmonics[x].phase=phase;
		harmonics[x].vol=((1<<(bps-(x+2)))-1);
		if(np) np(im_start,x+1,harmonics[x].freq,0,0,samplerate,bps,totaltime,usrptr);
	}
	if(wd) wd(im_data,(char *) &wave,sizeof(struct st_wav),usrptr);
	/* sound info */
	memset(&fmt,0,sizeof(struct st_fmt));
	memcpy(tfmt.header.type,"fmt ",sizeof(tfmt.header.type));
	tfmt.header.size=PCM_TYPE;
	tfmt.audioformat=1;
	tfmt.numchannels=1;
	tfmt.samplerate=samplerate;
	tfmt.bitspersample=bps;
	tfmt.byterate=tfmt.samplerate*tfmt.numchannels*tfmt.bitspersample/BITS_IN_BYTE;
	tfmt.blockalign=tfmt.numchannels*tfmt.bitspersample/BITS_IN_BYTE;
	if(wd) wd(im_data,(char *) &tfmt,sizeof(struct st_fmt),usrptr);
	chunk.size=rawsize;
	memcpy(chunk.type,"data",4);
	if(wd) wd(im_data,(char *) &chunk,sizeof(struct st_chunk),usrptr);
	p=strtok(buf,",");
	if(p) {
		/* calculate time for the first pressed key */
		t=(int) (atof(p)*time_base*samplerate);
		mute=(t < 0) ? 1 : 0;
		t=abs(t);
		med=t/3;
		end=t/2;
		interval=0;
	}
	for(x=0;x < totaltime;x++) {
		/* harmonic waves  */
		for(sample=0,y=0;y < sizeof(harmonics)/sizeof(harmonics[0]);y++) {
			cycles=samplerate/(double) harmonics[y].freq;
			/* is this frequency higher than half of sample rate? (nyquist ratio) */
			if(mute || cycles < 2.) {
				/* with this sample rate we cannot digitalize this frequency */
				if(np) np(im_data,y+1,harmonics[y].freq,x,0,samplerate,bps,totaltime,usrptr);
				continue;
			}
			/* calculate composite wave (waves cancel or add to each other) */
			v=point(harmonics[y].phase,1)*harmonics[y].vol;
			if(np) np(im_data,y+1,harmonics[y].freq,x,v,samplerate,bps,totaltime,usrptr);
			sample+=v;
			/* calculate next step */
			harmonics[y].phase+=360./cycles;
			while(harmonics[y].phase >= 360.) harmonics[y].phase-=360.;
		}
		/* save in the wav file the generated sample value */
		rawsize-=bps/BITS_IN_BYTE;
		if(rawsize >= 0) {
			/* verify if maximum amplitude was reached, if so, put it tom maximum allowed amplitude */
			sample=(sample > max_volume)  ? max_volume : (sample < -max_volume) ? -max_volume : sample;
			/*NOTICE this will work only in LITTLE-ENDIAN! For Big endian bytes must be swipped prior copying */
			w=(int) sample;
			if(wd) wd(im_data,(char *) &w,bps/BITS_IN_BYTE,usrptr);
			if(np) np(im_data,0,0,x,sample,samplerate,bps,totaltime,usrptr);
		}
		/* here we do the morse modulation (pwm) */
		if(!p) {
			mute=1;
		} else if(--t == 0) {
			if(!interval && !mute) {
				/* interval between each "pressed" key */
				interval=1;
				mute=1;
				t=(int) (time_base*samplerate);
			} else if((p=strtok(NULL,",")) != NULL) {
				/* calculate duration of the new "pressed" key */
				interval=0;
				t=(int) (atof(p)*time_base*samplerate);
				mute=(t < 0) ? 1 : 0;
				t=abs(t);
				med=t/3;
				end=t/2;
			}
		}
	}
	if(wd) wd(im_end,0,0,usrptr);
	if(np) np(im_end,0,0,x,0,samplerate,bps,totaltime,usrptr);
	return(0);
}

/* execute whenever a new point is set */
static void newpoint(infoMode mode,int harmonic,int freq,int x,double y,int rate,int bps,int totalsamples,void *usrptr)
{
	static FILE	*f=NULL;

	if(freq != 0) return;
	if(mode == im_start) {
		f=fopen("stat.txt","w");
	} else if(mode == im_end) {
		if(f) fclose(f);
		f=NULL;
	} else if(mode == im_data) {
		if(f) fprintf(f,"%d\n",(int) y);
	}
}

/* executed whenever the wave file is being constructed */
static void wavedata(infoMode mode,char *buf,int size,void *usrptr)
{
	static FILE	*f=NULL;

	if(mode == im_start) {
		f=fopen(TARGET_FILE,"wb");
	} else if(mode == im_end) {
		if(f) fclose(f);
		f=NULL;
	} else if(mode == im_data) {
		if(f) fwrite(buf,1,size,f);
	}
}

/* Entry Points */

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hinst,HINSTANCE hprev,char *cmdline,int show)
{
	return(startgui(hinst));
}
#else
int main(int argc,char **argv)
{
	int				freq=FREQ_FUNDAMENTAL;
	int				phase=0;
	int				samplerate=SAMPLE_RATE;
	int				bps=BITS_PER_SAMPLE;

	/* verify received parameters */
	if(argc < 2) {
		printf("\nmorsegen <message> [frequency:3500] [initial phase:0] [sample rate:44100 (8000, 16000, 44100)] [bits per sample:16 (16, 24, 32)] --> Generates a wav file with corresponding morse code\n%s",LEGAL);
		return(1);
	}
	/* bits per sample */
	if(argc > 5) bps=atoi(argv[5]);
	bps=(bps != 16 && bps != 24 && bps != 32) ? BITS_PER_SAMPLE : bps;
	/* sample rate */
	if(argc > 4) samplerate=atoi(argv[4]);
	samplerate=(samplerate != 8000 && samplerate != 16000 && samplerate != 44100) ? SAMPLE_RATE : samplerate;
	/* initial phase */
	if(argc > 3) phase=atoi(argv[3]);
	phase=(phase > 360) ? PHASE : phase;
	/* frequency */
	if(argc > 2) freq=atoi(argv[2]);
	freq=(freq < 20 || freq > samplerate/2) ? FREQ_FUNDAMENTAL : freq;
	/* generate wav file */
	generate(argv[1],freq,phase,TIME_BASE,bps,samplerate,newpoint,wavedata,(void *) NULL);
	return(0);
}
#endif



