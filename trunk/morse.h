/**
 * Module Description: morse code modulation and related definitions
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

#ifndef MORSE_H
#define MORSE_H

#define TARGET_FILE		"morse.wav"		/* default wave filename */
#define MAX_HARMONICS	12				/* maximum of harmonics to be calculated from the fundamental frequency */
#define LETTER_PAUSE	"-3"			/* time sequences after a letter */
#define	WORD_PAUSE		"-7"			/* time sequences after a word */

/**
 * enumeration for the information mode used by the callback functions
 */
typedef enum en_infoMode {
	im_start,	/* information is about to start */
	im_data,	/* data received */
	im_end,		/* end of information */
} infoMode;

/**
 * callback function executed whenever a new frequency sample is calculated
 * @param mode          info mode (see enum infoMode)
 * @param harmonic      current harmonic (0=average, 1=fundamental, ...)
 * @param freq          harmonic frequency (0=average)
 * @param x             current x position (time)
 * @param y             current y position (amplitude)
 * @param rate          sample rate
 * @param bps           bits per sample
 * @param totalsamples  totalsamples for current modulation
 * @param usrptr        user pointer
 */
typedef void (*NEWPOINT)(infoMode mode,int harmonic,int freq,int x,double y,int rate,int bps,int totalsamples,void *usrptr);

/**
 * callback function executed whenever wave data is to be save inside the wave file
 * @param mode    info mode (see enum infoMode)
 * @param buf     buffer to be saved in the wave file
 * @param size    size of the buffer (in bytes)
 * @param usrptr        user pointer
 */
typedef void (*WAVEDATA)(infoMode mode,char *buf,int size,void *usrptr);

/**
 * morse code translation international table
 * dot  == 1 time
 * dash == 3 times
*/
static struct st_morse {
	char	key;
	char	*snd;
} morse[] = {
	{'A',"1,3"},
	{'B',"3,1,1,1"},
	{'C',"3,1,3,1"},
	{'D',"3,1,1"},
	{'E',"1"},
	{'F',"1,1,3,1"},
	{'G',"3,3,1"},
	{'H',"1,1,1,1"},
	{'I',"1,1"},
	{'J',"1,3,3,3"},
	{'K',"3,1,3"},
	{'L',"1,3,1,1"},
	{'M',"3,3"},
	{'N',"3,1"},
	{'O',"3,3,3"},
	{'P',"1,3,3,1"},
	{'Q',"3,3,1,3"},
	{'R',"1,3,1"},
	{'S',"1,1,1"},
	{'T',"3"},
	{'U',"1,1,3"},
	{'V',"1,1,1,3"},
	{'W',"1,3,3"},
	{'X',"3,1,1,3"},
	{'Y',"3,1,3,3"},
	{'Z',"3,3,1,1"},
	{'1',"1,3,3,3,3"},
	{'2',"1,1,3,3,3"},
	{'3',"1,1,1,3,3"},
	{'4',"1,1,1,1,3"},
	{'5',"1,1,1,1,1"},
	{'6',"3,1,1,1,1"},
	{'7',"3,3,1,1,1"},
	{'8',"3,3,3,1,1"},
	{'9',"3,3,3,3,1"},
	{'0',"3,3,3,3,3"},
	{'.',"1,3,1,3,1,3"},
	{',',"3,3,1,1,3,3"}};

/**
 * translate a key into a morse code value
 * @param key key to be translated
 * @return a pointer to morse code translation string. Notice a dot "." will be returned if the key is not found
 */
static char *getsound(char key)
{
	int	x;

	for(x=0;x < sizeof(morse)/sizeof(morse[0]);x++) {
		if(morse[x].key == toupper(key)) {
			return(morse[x].snd);
		}
	}
	/* return a dot if sound is not found */
	return("1,2,1,2,1,2");
}

/**
 * converts a string into a morse code representation for dots and dashes
 * @param msg message to be converted
 * @param buf buffer to return conversion
 * @param sz  size of buffer to store the converstion
 * @return a pointer to buf
 */
static char	*morsecode(char *msg,char *buf,int sz)
{
	char	*q=msg,*s;

	*buf='\0';
	while(*q == ' ') q++;
	for(;*q && sz > 1;q++) {
		if(*q == ' ') {
			if(*buf) strcat(buf,",");
			strcat(buf,WORD_PAUSE);
			sz--;
			continue;
		}
		s=getsound(*q);
		sz-=strlen(s)-1;
		if(sz > 1) {
			if(*buf) strcat(buf,",");
			strcat(buf,s);
			if(*buf) strcat(buf,",");
			strcat(buf,LETTER_PAUSE);
		}
	}
	return(buf);
}

#endif /* MORSE_H */