/**
 * Module Description: windows graphical user interface to morse code generator
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

/* windows only */

#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <math.h>
#include <Mmsystem.h>
#include "resource.h"
#include "morse.h"

/* list base */

/* defines */

#define BGCOLOR		RGB(0,183,235)
#define DOTCOLOR	RGB(0,0,0)
#define MAX_HARM	4

/* globals */

HBRUSH		BgBrush,GraphBrush[MAX_HARM];
HPEN		DashPen,GraphPen[MAX_HARM];
static int	rates[]={8000,16000,44100};
COLORREF	colors[MAX_HARM]={RGB(255,255,128),RGB(255,0,0),RGB(0,255,0),RGB(0,0,255)};

char		*samples[MAX_HARM];
int			samplesize[MAX_HARM];
char		*wav;
int			wavesize;

int generate(char *msg,int freq,int phase,int nharm,double time_base,int bps,int samplerate,NEWPOINT np,WAVEDATA wd,void *usrptr);

/*
	adjust frequency based on current samplerate
*/
void adjustfreq(HWND h,DWORD samplerate)
{
	char	buf[512];

	if(samplerate >= 1 && samplerate <= sizeof(rates)/sizeof(rates[0])) {
		samplerate=rates[samplerate-1];
		GetWindowText(GetDlgItem(h,IDC_FREQ),buf,sizeof(buf));
		if(atoi(buf) > 0 && atoi(buf) >= (int) samplerate/2) {
			sprintf(buf,"%d",samplerate/2-1);
			SetWindowText(GetDlgItem(h,IDC_FREQ),buf);
		}
	}
}

/*
	draw graph
*/
void drawgraph(HWND h,HDC hdc,int zoom,int posit)
{
	int		lastx,lasty=0;
	RECT	rc,rcb;
	POINT	pt;
	int		max_vol=0,x,y,exit=0,rate,bps,totalsample,l,*psample,harm,position,freq;
	double	dif=0,d;
	char	buf[512];

	GetClientRect(h,&rc);
	MoveToEx(hdc,0,rc.bottom/2,&pt);
	SelectObject(hdc,DashPen);
	SetBkColor(hdc,BGCOLOR);
	FillRect(hdc,&rc,BgBrush);
	LineTo(hdc,pt.x+rc.right,rc.bottom/2);
	for(harm=sizeof(samples)/sizeof(samples[0])-1;harm >=0;harm--) {
		/* read samples */
		if(samplesize[harm] < sizeof(int)*3) {
			continue;
		}
		SelectObject(hdc,GraphPen[harm]);
		SetBkColor(hdc,BGCOLOR);
		SetTextColor(hdc,colors[harm]);
		lastx=0x80000001;
		psample=(int *) samples[harm];
		rate=*(psample++);
		bps=*(psample++);
		totalsample=*(psample++);
		freq=*(psample++);
		max_vol=1 << (bps-1);
		position=(int) ((double) posit/100*totalsample);
		if(freq) {
			x=sprintf(buf,"%d hz",freq);
		} else {
			x=sprintf(buf,"AVG");
		}
		rcb.left=rc.right-60-(harm*70);
		rcb.top=rc.bottom-30;
		rcb.right=rc.right-70-(harm*70);
		rcb.bottom=rc.bottom-20;
		FillRect(hdc,&rcb,GraphBrush[harm]);
		TextOut(hdc,rcb.left,rcb.bottom,buf,x);
		l=0;
		for(;(char *) psample < samples[harm]+samplesize[harm];psample++) {
			if(position > 0) {
				position--;
				continue;
			}
			if(l%100 == 0 ||  l+1 == totalsample) {
				sprintf(buf,"%03d%%",(l+1)*100/totalsample);
				TextOut(hdc,5,rc.bottom-20,buf,strlen(buf));
			}
			y=*psample*rc.bottom/2/max_vol;
			y=rc.bottom-(rc.bottom/2+y);
			d=((double) l++/totalsample*rc.right*zoom);
			x=(int) d;
			dif+=d-x;
			if(dif >= 1) {
				x++;
				dif-=1;
			}
			if(x >= rc.right) {
				x=rc.right-1;
				exit=1;
			}
			if(lastx == 0x80000001) {
				lastx=x;
				lasty=y;
			}
			MoveToEx(hdc,lastx,lasty,&pt);
			LineTo(hdc,x,y);
			lastx=x;
			lasty=y;
		}
		if(l%100 == 0 ||  l+1 == totalsample) {
			TextOut(hdc,5,rc.bottom-20,"100%",4);
		}
	}
}

/* execute whenever a new point is set */
static void newpoint(infoMode mode,int harm,int freq,int x,double y,int rate,int bps,int totalsample,void *usrptr)
{
	int	w;

	if(harm >= sizeof(samples)/sizeof(samples[0])) return;
	if(mode == im_start) {
		samplesize[harm]=0;
		if(samples[harm]) free(samples[harm]);
		samples[harm]=malloc(sizeof(int)*4);
		if(samples[harm]) {
			memcpy(samples[harm],&rate,sizeof(int));
			samplesize[harm]+=sizeof(int);
			memcpy(samples[harm]+samplesize[harm],&bps,sizeof(int));
			samplesize[harm]+=sizeof(int);
			memcpy(samples[harm]+samplesize[harm],&totalsample,sizeof(int));
			samplesize[harm]+=sizeof(int);
			memcpy(samples[harm]+samplesize[harm],&freq,sizeof(int));
			samplesize[harm]+=sizeof(int);
		}
	} else if(mode == im_end) {
	} else if(mode == im_data) {
		if(samples[harm]) {
			samples[harm]=realloc(samples[harm],samplesize[harm]+sizeof(int));
			w=(int) y;
			memcpy(samples[harm]+samplesize[harm],&w,sizeof(int));
			samplesize[harm]+=sizeof(int);
		}
	}
}

/* executed whenever the wave file is being constructed */
static void wavedata(infoMode mode,char *data,int size,void *usrptr)
{
	if(mode == im_start) {
		wavesize=0;
		if(wav) free(wav);
		wav=NULL;
	} else if(mode == im_end) {
	} else if(mode == im_data) {
		if(wav) wav=realloc(wav,wavesize+size);
		else wav=malloc(size);
		memcpy(wav+wavesize,data,size);
		wavesize+=size;
	}
}

/*
	Graphical User Interface - events handler
*/
BOOL CALLBACK evthandler(HWND h,UINT uMsg,WPARAM wParam,LPARAM  lParam)
{
	char			buf[512];
	DWORD			samplerate;
	HDC				hdc;
	PAINTSTRUCT		paint;
	int				freq,phase,zoom,posit;
	static int		lastzoom=500,lastposit=0;
	OPENFILENAME	ofn;

	switch(uMsg) {
	case WM_INITDIALOG:
		/* (sample rate) set minimum and maximum values and position */
		SendMessage(GetDlgItem(h,IDC_SLIDER1),TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(1,3));
		SendMessage(GetDlgItem(h,IDC_SLIDER1),TBM_SETPOS,(WPARAM) TRUE,(LPARAM) 3); 
		/* (zoom) set minimum and maximum values and position */
		SendMessage(GetDlgItem(h,IDC_ZOOM),TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(1,5000));
		SendMessage(GetDlgItem(h,IDC_ZOOM),TBM_SETPOS,(WPARAM) TRUE,(LPARAM) lastzoom); 
		/* (position) set minimum and maximum values and position */
		SendMessage(GetDlgItem(h,IDC_POSIT),TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0,100));
		SendMessage(GetDlgItem(h,IDC_POSIT),TBM_SETPOS,(WPARAM) TRUE,(LPARAM) lastposit); 
		/* Phase */
		SetWindowText(GetDlgItem(h,IDC_PHASE),"0");
		/* Frequency */
		SetWindowText(GetDlgItem(h,IDC_FREQ),"3500");
		SetWindowText(GetDlgItem(h,IDC_MSG),"SOS");
		break;
	case WM_CTLCOLORSTATIC:
		break;
	case WM_HSCROLL:
	case WM_VSCROLL:
        switch(LOWORD(wParam)) {
		case TB_LINEUP:
		case TB_LINEDOWN:
		case TB_PAGEUP:
		case TB_PAGEDOWN:
		case TB_THUMBTRACK:
		case TB_TOP:
			samplerate=SendMessage(GetDlgItem(h,IDC_SLIDER1),TBM_GETPOS,0,0);
			adjustfreq(h,samplerate);
			if(GetDlgCtrlID((HWND) lParam) == IDC_ZOOM || GetDlgCtrlID((HWND) lParam) == IDC_POSIT) {
				zoom=SendMessage(GetDlgItem(h,IDC_ZOOM),TBM_GETPOS,0,0);
				posit=SendMessage(GetDlgItem(h,IDC_POSIT),TBM_GETPOS,0,0);
				if(zoom != lastzoom || posit != lastposit) {
					KillTimer(h,0);
					SetTimer(h,0,40,NULL);
					lastzoom=zoom;
					lastposit=posit;
				}
			}
			break;
        }
		break;
	case WM_TIMER:
		KillTimer(h,0);
		InvalidateRect(GetDlgItem(h,IDC_GRAPH),NULL,TRUE);
		SendMessage(h,WM_PAINT,0,0);
		break;
	case WM_CLOSE:
		EndDialog(h,TRUE);
		break;
	case WM_COMMAND:
		if(LOWORD(wParam) == (WORD) IDC_PLAY) {
			if(wav) freq=PlaySound(wav,NULL,SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
		} else if(LOWORD(wParam) == (WORD) IDC_PHASE) {
			GetWindowText(GetDlgItem(h,IDC_PHASE),buf,sizeof(buf));
			if(atoi(buf) > 360) {
				SetWindowText(GetDlgItem(h,IDC_PHASE),"360");
			}
		} else if(LOWORD(wParam) == (WORD) IDC_FREQ) {
			samplerate=SendMessage(GetDlgItem(h,IDC_SLIDER1),TBM_GETPOS,0,0);
			adjustfreq(h,samplerate);
		} else if(LOWORD(wParam) == (WORD) IDC_GENERATE) {
			GetWindowText(GetDlgItem(h,IDC_FREQ),buf,sizeof(buf));
			freq=atoi(buf);
			GetWindowText(GetDlgItem(h,IDC_PHASE),buf,sizeof(buf));
			phase=atoi(buf);
			samplerate=rates[SendMessage(GetDlgItem(h,IDC_SLIDER1),TBM_GETPOS,0,0)-1];
			GetWindowText(GetDlgItem(h,IDC_MSG),buf,sizeof(buf));
			if(strlen(buf)) {
				generate(buf,freq,phase,8,.05,16,samplerate,newpoint,wavedata,h);
				InvalidateRect(GetDlgItem(h,IDC_GRAPH),NULL,TRUE);
				SendMessage(h,WM_PAINT,0,0);
			}
		} else if(LOWORD(wParam) == (WORD) IDC_SAVE) {
			if(!wavesize || !wavesize) {
				break;
			}
			memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner=h;
			ofn.nMaxFile=sizeof(buf);
			ofn.nMaxFileTitle=sizeof(buf);
			strcpy(buf,"morse.wav");
			ofn.lpstrFile=buf;
			ofn.lpstrFilter="Wave File (*.wav)\0*.*\0\0";
			ofn.lpstrDefExt="wav";
			ofn.Flags=OFN_HIDEREADONLY|OFN_ENABLESIZING|OFN_OVERWRITEPROMPT;
			if(GetSaveFileName(&ofn)) {
				FILE	*f;

				f=fopen(ofn.lpstrFile,"wb");
				if(f) {
					fwrite(wav,1,wavesize,f);
					fclose(f);
				}
			}
			InvalidateRect(GetDlgItem(h,IDC_GRAPH),NULL,TRUE);
			SendMessage(h,WM_PAINT,0,0);
		}
		break;
	case WM_PAINT:
		hdc=BeginPaint(GetDlgItem(h,IDC_GRAPH),&paint);
		if(hdc) {
			zoom=SendMessage(GetDlgItem(h,IDC_ZOOM),TBM_GETPOS,0,0);
			posit=SendMessage(GetDlgItem(h,IDC_POSIT),TBM_GETPOS,0,0);
			drawgraph(GetDlgItem(h,IDC_GRAPH),hdc,zoom,posit);
			EndPaint(GetDlgItem(h,IDC_GRAPH),&paint);
		}
		break;
	}
	return(FALSE);
}

/*
	Graphical User Interface - Initial entry point
*/
int startgui(HINSTANCE instance)
{
	int	x;

	BgBrush=CreateSolidBrush(BGCOLOR);
	DashPen=CreatePen(PS_DOT,1,DOTCOLOR);
	for(x=0;x < sizeof(samples)/sizeof(samples[0]);x++) {
		GraphPen[x]=CreatePen(PS_SOLID,1,colors[x]);
		GraphBrush[x]=CreateSolidBrush(colors[x]);
		samples[x]=NULL;
		samplesize[x]=0;
	}
	wav=NULL;
	wavesize=0;
	LoadLibraryEx("COMCTL32.DLL",NULL,0);
	DialogBoxParam(instance,"SOUNDPANEL",NULL,evthandler,(LPARAM) NULL);
	return(0);
}
