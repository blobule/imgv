/*
 * This file is part of Profilometre.
 * 
 * @Copyright 2010 Université de Montréal
 *   Sébastien Roy,  roys@iro.umontreal.ca
 *   (Laboratoire Vision3D, Université de Montréal)
 *
 * Profilometre is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Profilometre is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Profilometre.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <time.h>
#include <sys/time.h>


#define USE_PROFILOMETRE
#include "profilometre.h"


/// Maximum of named timers that can be tracked 
#define MAX_H	100

/**
 * Internal timing structure for named timers
*/
typedef struct {
	char name[64];	///< All names for timers must be <64 char
	int count;
	double min,max;
	double sum,sum2;
	double start;
	// histogramme hmin...hmax, hstep
	double hmin,hmax,hstep;
	int bins; // nb de cases histo[0..bins-1] -> hmin...hmax par step de hstep
	int *histo; // [bins]
	} profilometreinfo;

/**
 * Global table for named timers
 *
 * @warning This global structure implied that the library might not work well as a shared library.
 * @warning This also means that this library is absolutely not thread safe.
*/
static profilometreinfo *H[MAX_H];


/**
 * Initialize the profilometre library
 *
 * @note This must be called before any other functions.
*/
void profilometre_init(void)
{
int i;
	for(i=0;i<MAX_H;i++) H[i]=NULL;
}

/**
 * Reset all accumulated timing information
*/
void profilometre_reset(void)
{
int i;
	for(i=0;i<MAX_H && H[i];i++) {
		H[i]->count=0;
		H[i]->sum=0.0;
		H[i]->sum2=0.0;
		H[i]->min=-1.0; // bidon
		H[i]->max=-1.0; // bidon
		if( H[i]->histo ) {
			int j;
			for(j=0;j<H[i]->bins;j++) H[i]->histo[j]=0;
		}
	}
}

/**
 * initialize the histogram functionality
 * call this before using start()
 *
*/
void profilometre_setup_histogram(const char *name,double hmin,double hmax,double hstep)
{
int i;
	for(i=0;i<MAX_H && H[i];i++) if( strcmp(H[i]->name,name)==0 ) break;
	if( i==MAX_H ) return; // pas trouve!
	if( H[i]==NULL ) {
		// nouveau nom!
		H[i]=(profilometreinfo *)malloc(sizeof(profilometreinfo));
		strcpy(H[i]->name,name);
		H[i]->count=0;
		H[i]->sum=0.0;
		H[i]->sum2=0.0;
		H[i]->min=-1.0; // bidon
		H[i]->max=-1.0; // bidon
		H[i]->hmin=hmin;
		H[i]->hmax=hmax;
		H[i]->hstep=hstep;
		H[i]->bins=(int)((hmax-hmin)/hstep+1);
		H[i]->histo=(int *)malloc(H[i]->bins*sizeof(int));
		int j;
		for(j=0;j<H[i]->bins;j++) H[i]->histo[j]=0;
	}
	
}

/**
 * Start a named timer.
 *
 * The same name must be used in profilometre_stop().
 */
void profilometre_start(const char *name)
{
int i;
	for(i=0;i<MAX_H && H[i];i++) if( strcmp(H[i]->name,name)==0 ) break;
	if( i==MAX_H ) return; // plus de place
	if( H[i]==NULL ) {
		// nouveau nom!
		H[i]=(profilometreinfo *)malloc(sizeof(profilometreinfo));
		strcpy(H[i]->name,name);
		H[i]->count=0;
		H[i]->sum=0.0;
		H[i]->sum2=0.0;
		H[i]->min=-1.0; // bidon
		H[i]->max=-1.0; // bidon
		H[i]->hmin=-1.0; // bidon
		H[i]->hmax=-1.0; // bidon
		H[i]->hstep=0.0;
		H[i]->bins=0;
		H[i]->histo=NULL; // -> pas d'histo
	}
	struct timeval tv;
	gettimeofday(&tv,NULL);
	H[i]->start=(double)tv.tv_sec+ (double)tv.tv_usec / 1000000.0;
}


/**
 * Stop a named timer.
 *
 * The name must correspond to the one used in profilometre_start().
 */
void profilometre_stop(const char *name)
{
int i;
struct timeval tv;
double stop;
	gettimeofday(&tv,NULL);
	stop=(double)tv.tv_sec+ (double)tv.tv_usec / 1000000.0;

	for(i=0;i<MAX_H && H[i];i++) if( strcmp(H[i]->name,name)==0 ) break;
	if( i==MAX_H ) return; // pas trouve!
	if( H[i]==NULL ) return; // pas trouve!

	stop=(stop-H[i]->start)*1000.0; // en ms
	if( H[i]->count==0 ) {
		H[i]->min=H[i]->max=stop;
	}else{
		if( stop<H[i]->min ) H[i]->min=stop;
		else if( stop>H[i]->max ) H[i]->max=stop;
	}
	H[i]->sum+=stop;
	H[i]->sum2+=stop*stop;
	H[i]->count++;
	// histogram
	if( H[i]->histo ) {
		int pos=(int)((stop-H[i]->hmin)/H[i]->hstep+0.5);
		if( pos<0 ) pos=0;
		if( pos>=H[i]->bins ) pos=H[i]->bins-1;
		H[i]->histo[pos]++;
	}
}

/**
 * Dump all statistics
 *
 * For all named timers, print stats on running time
 * This can be called at any time and will display current stats.
*/
void profilometre_dump(void)
{
  int i;
  //      Profiler       2.41ms +/-       3.05ms     3976 runs : decodeThread
  printf("               mean         stddev        min        max   count : name\n");
  for(i=0;i<MAX_H && H[i];i++) {
    double delta = (H[i]->sum2 - H[i]->sum*H[i]->sum/H[i]->count)/H[i]->count;
    if( delta<0.0 ) delta=0.0;
    double std = sqrt(delta);
    double mean = H[i]->sum/H[i]->count;
    printf("Profiler %8.2fms +/- %8.2fms %8.2fms %8.2fms %7d : %s\n",mean,std,H[i]->min,H[i]->max,H[i]->count,H[i]->name);
  }
  printf("\n");
  // les histogrammes
  for(i=0;i<MAX_H && H[i];i++) {
	if( H[i]->histo==NULL ) continue;
	printf("==== Histogramme : %s (hmin=%f,hmax=%f,hstep=%f,bins=%d ====\n",H[i]->name,H[i]->hmin,H[i]->hmax,H[i]->hstep,H[i]->bins);
	int c=0;
	int j;
	for(j=0;j<H[i]->bins;j++) c+=H[i]->histo[j];
	printf("== count=%7d,  hcount=%7d\n",H[i]->count,c);
	if( H[i]->count>0 ) {
		int cmax=0;
		for(j=0;j<H[i]->bins;j++) { if( H[i]->histo[j]>cmax ) cmax=H[i]->histo[j]; }
		for(j=0;j<H[i]->bins;j++) {
			double v=j*H[i]->hstep+H[i]->hmin;
			int c=H[i]->histo[j];
			printf("%8.2f %6.2f  |",v,c*100.0/H[i]->count);
			int k;
			for(k=0;k<(c*70+cmax/2)/cmax;k++) printf("*");
			printf("\n");
		}
	}
  }
  
}


//////////////////////////////////////////////////////
//
// To help keep statistics about counts, inside a data structure
//
//////////////////////////////////////////////////////

/**
 * Reset the named counter
 *
 * The statistics are reset, but the timer is not started.
 * @note This must be called before any other operation on the counter
*/
void profilometre_count_reset(profilometre_countinfo *ci)
{
	ci->lastUpdate=-1.0; // <0 means no updates yet
}


/**
 * Update the number in a counting timer
 *
 * Call this whenever the number N changes. The first call will initialize N
 * and start the timer.
*/
void profilometre_count_update(profilometre_countinfo *ci,int n)
{
struct timeval tv;
double now,delta;
	gettimeofday(&tv,NULL);
	now=(double)tv.tv_sec+ (double)tv.tv_usec / 1000000.0;
	if( ci->lastUpdate<0.0 ) {
		// first call to update. initialize everything
		ci->count=0.0;
		ci->sum=0.0;
		ci->sum2=0.0;
		ci->min=ci->max=n;
		ci->lastUpdate0=-1.0;
		ci->count0=0.0;
	}else{
		// not first call. Update from previous value info
		delta=now-ci->lastUpdate;
		//printf("counting value %d for time=%f\n",ci->lastVal,delta);
		ci->count+=delta;
		ci->sum+=delta*ci->lastVal;
		ci->sum2+=delta*ci->lastVal*ci->lastVal;
		if( n<ci->min ) ci->min=n;
		if( n>ci->max ) ci->max=n;
		// stats on percentage of time empty
		if( ci->lastVal==0 ) ci->count0+=delta;
		if( n==0 )  ci->lastUpdate0=now;
		else		ci->lastUpdate0=-1;

	}
	ci->lastVal=n;
	ci->lastUpdate=now;
}


/**
 * Compute the statistics
 *
 * Computes the mean, standard deviation et percent of time empty
 * statistics for this counting timer.
 *
 * The caller is expected to go in the @p ci structure later to
 * use the statistics.
*/
void profilometre_count_stats(profilometre_countinfo *ci)
{
	// finish current time
	profilometre_count_update(ci,ci->lastVal);

	if( ci->lastUpdate<0 ) {
		// update was never called!!
		ci->std=-1.0; // special value to see this case
		ci->mean = 0.0;
		ci->percent0 = 0.0;
	}else{
		ci->std=sqrt((ci->sum2 - ci->sum*ci->sum/ci->count)/ci->count);
		ci->mean = ci->sum/ci->count;
		ci->percent0 = ci->count0 / ci->count;
	}
}





