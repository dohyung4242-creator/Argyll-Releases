#ifndef CCSS_H
#define CCSS_H

/* 
 * Argyll Color Correction System
 * Colorimeter Calibration Spectral Set support.
 *
 * Author: Graeme W. Gill
 * Date:   18/8/2011
 *
 * Copyright 2010 Graeme W. Gill
 * All rights reserved.
 *
 * This material is licenced under the GNU GENERAL PUBLIC LICENSE Version 2 or later :-
 * see the License2.txt file for licencing details.
 *
 * Based on ccmx.h
 */

/*
 * This object provides storage and application of emisive spectral
 * samples that can be used to compute calibration for suitable
 * colorimeters (such as the i1d3) tuned for particular types of displays.
 */

/* ------------------------------------------------------------------------------ */

struct _ccss {

  /* Public: */
	void (*del)(struct _ccss *p);

	/* Set the contents of the ccss. return nz on error. */
	/* (Makes copies of all parameters) */
	int (*set_ccss)(struct _ccss *p, char *orig, char *cdate,
	                char *desc, char *disp, char *tech, char *ref,
	                xspect *samples, int no_samp);	

	/* write to a CGATS .ccss file */
	/* return nz on error, with message in err[] */
	int (*write_ccss)(struct _ccss *p, char *filename);

	/* read from a CGATS .ccss file */
	/* return nz on error, with message in err[] */
	int (*read_ccss)(struct _ccss *p, char *filename);

  /* Private: */
	/* (All char * are owned by ccss) */
	char *orig;			/* Originator. May be NULL */
	char *crdate;		/* Creation date (in ctime() format). May be NULL */
	char *desc;			/* General Description (optional) */
	char *disp;			/* Description of the display (Manfrr and Model No) (optional if tech) */
	char *tech;			/* Technology (CRT, LCD + backlight type etc.) (optional if disp) */
	char *ref;			/* Name of reference spectrometer instrument (optional) */
	xspect *samples;	/* Set of spectral samples */
	int no_samp;		/* Number of samples */
	
	/* Houskeeping */
	int errc;				/* Error code */
	char err[200];			/* Error message */
}; typedef struct _ccss ccss;

/* Create a new, uninitialised ccss */
ccss *new_ccss(void);

#endif /* CCSS_H */




































