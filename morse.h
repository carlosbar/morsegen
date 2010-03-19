#ifndef MORSE_H
#define MORSE_H

#define TARGET_FILE	"morse.wav"

typedef enum en_infoMode {
	im_start,
	im_data,
	im_end,
} infoMode;

typedef void (*NEWPOINT)(infoMode mode,int harmonic,int freq,int x,double y,int rate,int bps,int totalsamples,void *usrptr);
typedef void (*WAVEDATA)(infoMode mode,char *buf,int size,void *usrptr);

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

#define LETTER_PAUSE	"-3"
#define	WORD_PAUSE		"-7"

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