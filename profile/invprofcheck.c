
/* 
 * Argyll Color Correction System
 * Inverse profile checker.
 *
 * Author: Graeme W. Gill
 * Date:   1999/11/29
 *
 * Copyright 1999 - 2005 Graeme W. Gill
 * All rights reserved.
 *
 * This material is licenced under the GNU AFFERO GENERAL PUBLIC LICENSE Version 3 :-
 * see the License.txt file for licencing details.
 */

/*
 * This program takes checks the round trip errors of 
 * the colorimetric forward and inverse profile direction
 * of an ICC profile.
 * (Was called icc/fbtest.c)
 */


/* TTBD:
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>
#include "copyright.h"
#include "config.h"
#include "numlib.h"
#include "icc.h"
#include "xicc.h"

/* Resolution of the sampling modes */
#define TRES 11
#define HTRES 27
#define UHTRES 61

/* ------------------------------------------------------- */
/* Macros for an di or fdi dimensional counter */
/* Declare the counter name nn, dimensions di, & count */

#define DCOUNT(nn, di, start, reset, count) 				\
	int nn[MAX_CHAN];	/* counter value */					\
	int nn##_di = (di);		/* Number of dimensions */		\
	int nn##_stt = (start);	/* start count value */			\
	int nn##_rst = (reset);	/* reset on carry value */		\
	int nn##_res = (count); /* last count +1 */				\
	int nn##_e				/* dimension index */

/* Set the counter value to 0 */
#define DC_INIT(nn) 								\
{													\
	for (nn##_e = 0; nn##_e < nn##_di; nn##_e++)	\
		nn[nn##_e] = nn##_stt;						\
	nn##_e = 0;										\
}

/* Increment the counter value */
#define DC_INC(nn)									\
{													\
	for (nn##_e = 0; nn##_e < nn##_di; nn##_e++) {	\
		nn[nn##_e]++;								\
		if (nn[nn##_e] < nn##_res)					\
			break;	/* No carry */					\
		nn[nn##_e] = nn##_rst;						\
	}												\
}

/* After increment, expression is TRUE if counter is done */
#define DC_DONE(nn)									\
	(nn##_e >= nn##_di)
	
/* ---------------------------------------- */

void usage(void) {
	fprintf(stderr,"Check fwd to bwd relative transfer of an ICC file, Version %s\n",ARGYLL_VERSION_STR);
	fprintf(stderr,"Author: Graeme W. Gill\n");
	fprintf(stderr,"usage: invprofcheck [-] profile.icm\n");
	fprintf(stderr," -v [level]   verbosity level (default 1), 2 to print each DE\n");
	fprintf(stderr," -l limit     set total ink limit (estimate by default)\n");
	fprintf(stderr," -L klimit    set black channel ink limit (estimate by default)\n");
	fprintf(stderr," -h           high res test (%d)\n",HTRES);
	fprintf(stderr," -u           Ultra high res test (%d)\n",UHTRES);
	fprintf(stderr," -R res       Specific grid resolution\n");
	fprintf(stderr," -c           Show CIE94 delta E values\n");
	fprintf(stderr," -k           Show CIEDE2000 delta E values\n");
	fprintf(stderr," -w           create VRML visualisation (profile.wrl)\n");
	fprintf(stderr," -x           Use VRML axes\n");
	fprintf(stderr," -e           Color vectors acording to delta E\n");
	fprintf(stderr," profile.icm  Profile to check\n");
	exit(1);
}

FILE *start_vrml(char *name, int doaxes);
void start_line_set(FILE *wrl);
void add_vertex(FILE *wrl, double pp[3]);
void make_lines(FILE *wrl, int ppset);
void make_de_lines(FILE *wrl);
void end_vrml(FILE *wrl);

#if defined(__IBMC__) && defined(_M_IX86)
void bug_workaround(int *co) { };			/* Workaround optimiser bug */
#endif

int
main(
	int argc,
	char *argv[]
) {
	int fa,nfa;						/* argument we're looking at */
	int verb = 0;
	int cie94 = 0;
	int cie2k = 0;
	int dovrml = 0;
	int doaxes = 0;
	int dodecol = 0;
	char in_name[MAXNAMEL+1];
	char out_name[MAXNAMEL+1], *xl;		/* VRML name */
	icmFile *rd_fp;
	icc *icco;
	int rv = 0;
	int tres = TRES;
	double tlimit = -1.0;
	double klimit = -1.0;
	FILE *wrl = NULL;

	error_program = "invprofcheck";

	if (argc < 2)
		usage();

	/* Process the arguments */
	for(fa = 1;fa < argc;fa++) {
		nfa = fa;					/* skip to nfa if next argument is used */
		if (argv[fa][0] == '-')	{	/* Look for any flags */
			char *na = NULL;		/* next argument after flag, null if none */

			if (argv[fa][2] != '\000')
				na = &argv[fa][2];		/* next is directly after flag */
			else {
				if ((fa+1) < argc) {
					if (argv[fa+1][0] != '-') {
						nfa = fa + 1;
						na = argv[nfa];		/* next is seperate non-flag argument */
					}
				}
			}

			if (argv[fa][1] == '?')
				usage();

			/* Verbosity */
			else if (argv[fa][1] == 'v' || argv[fa][1] == 'V') {
				verb = 1;
				if (na != NULL && isdigit(na[0])) {
					verb = atoi(na);
				}
			}

			/* Resolution */
			else if (argv[fa][1] == 'h' || argv[fa][1] == 'H') {
				tres = HTRES;

			}
			/* Resolution */
			else if (argv[fa][1] == 'u' || argv[fa][1] == 'U') {
				tres = UHTRES;
			}

			/* Resolution */
			else if (argv[fa][1] == 'R') {
				int res;
				fa = nfa;
				if (na == NULL) usage();
				res = atoi(na);
				if (res < 2 || res > 500)
					usage();
				tres = res;
			}

			else if (argv[fa][1] == 'l') {
				int limit;
				fa = nfa;
				if (na == NULL) usage();
				limit = atoi(na);
				if (limit < 1)
					limit = 1;
				tlimit = limit/100.0;
			}

			else if (argv[fa][1] == 'L') {
				int limit;
				fa = nfa;
				if (na == NULL) usage();
				limit = atoi(na);
				if (limit < 1)
					limit = 1;
				klimit = limit/100.0;
			}

			/* VRML */
			else if (argv[fa][1] == 'w' || argv[fa][1] == 'W')
				dovrml = 1;

			/* Axes */
			else if (argv[fa][1] == 'x' || argv[fa][1] == 'X')
				doaxes = 1;

			/* Delta E coloring */
			else if (argv[fa][1] == 'e' || argv[fa][1] == 'E')
				dodecol = 1;

			else if (argv[fa][1] == 'c' || argv[fa][1] == 'C') {
				cie94 = 1;
				cie2k = 0;
			}

			else if (argv[fa][1] == 'k' || argv[fa][1] == 'K') {
				cie94 = 0;
				cie2k = 1;
			}

			else 
				usage();
		}
		else
			break;
	}

	if (fa >= argc || argv[fa][0] == '-') usage();
	strncpy(in_name,argv[fa++],MAXNAMEL); in_name[MAXNAMEL] = '\000';


	strncpy(out_name,in_name,MAXNAMEL-4); out_name[MAXNAMEL-4] = '\000';
	if ((xl = strrchr(out_name, '.')) == NULL)	/* Figure where extention is */
		xl = out_name + strlen(out_name);
	strcpy(xl,".wrl");

	/* Open up the file for reading */
	if ((rd_fp = new_icmFileStd_name(in_name,"r")) == NULL)
		error ("Read: Can't open file '%s'",in_name);

	if ((icco = new_icc()) == NULL)
		error ("Read: Creation of ICC object failed");

	/* Read the header and tag list */
	if ((rv = icco->read(icco,rd_fp,0)) != 0)
		error ("Read: %d, %s",rv,icco->err);

	/* Check the forward lookup against the bwd function */
	{
		xcal *cal = NULL;                   /* Device calibration curves */
		icColorSpaceSignature ins, outs;	/* Type of input and output spaces of fwd */
		int inn, outn;						/* Channels of fwd conversion */
		int kch;							/* Black channel, -1 if not known/applicable */
		icmLuBase *luo1, *luo2;
		double merr = 0.0;		/* Max */
		double aerr = 0.0;		/* Avg */
		double rerr = 0.0;		/* RMS */
		double nsamps = 0.0;

		/* Get a Device to PCS conversion object */
		if ((luo1 = icco->get_luobj(icco, icmFwd, icRelativeColorimetric, icSigLabData, icmLuOrdNorm)) == NULL) {
			if ((luo1 = icco->get_luobj(icco, icmFwd, icmDefaultIntent, icSigLabData, icmLuOrdNorm)) == NULL)
				error ("%d, %s",icco->errc, icco->err);
		}
		/* Get details of conversion */
		luo1->spaces(luo1, &ins, &inn, &outs, &outn, NULL, NULL, NULL, NULL, NULL);

		/* Get a PCS to Device conversion object */
		if ((luo2 = icco->get_luobj(icco, icmBwd, icRelativeColorimetric, icSigLabData, icmLuOrdNorm)) == NULL) {
			if ((luo2 = icco->get_luobj(icco, icmBwd, icmDefaultIntent, icSigLabData, icmLuOrdNorm)) == NULL)
				error ("%d, %s",icco->errc, icco->err);
		}

		if (dovrml) {
			wrl = start_vrml(out_name, doaxes);
			start_line_set(wrl);
		}
		
		/* Grab any device calibration curves */
		cal = xiccReadCalTag(icco);

		kch = icxGuessBlackChan(icco);

		/* Set the default ink limits if not set by user */
		if (tlimit < 0.0 || klimit < 0.0) {
			double max[MAX_CHAN], total;

			total = icco->get_tac(icco, max, cal != NULL ? xiccCalCallback : NULL, (void *)cal);

			if (tlimit < 0.0)
				tlimit = total;

			if (klimit < 0.0 && kch >= 0)
				klimit = max[kch];
		}

		if (verb) {
			printf("Grid resolution is %d\n",tres);
			if (tlimit >= 0.0)
				printf("Input total ink limit assumed is %3.1f%%\n",100.0 * tlimit);
			if (klimit >= 0.0)
				printf("Input black ink limit assumed is %3.1f%%\n",100.0 * klimit);
		}

		{
			double dev[MAX_CHAN], cdev[MAX_CHAN], pcsin[3], devout[MAX_CHAN], pcsout[3];
			DCOUNT(co, inn, 0, 0, tres);		/* Multi-D counter */
	
			/* Go through the chosen device grid */
			DC_INIT(co)
			for (; !DC_DONE(co);) {
				int n, rv1, rv2;
				double sum;
				double de;

				/* Check the (possibly calibrated) device values */
				/* end reject any over the limits. */
				for (sum = 0, n = 0; n < inn; n++) {
					cdev[n] = dev[n] = co[n]/(tres-1.0);
					sum += cdev[n];
				}
				if (cal != NULL) {
					cal->interp(cal, cdev, dev);
					for (sum = 0, n = 0; n < inn; n++)
						sum += cdev[n];
				}

				if (tlimit > 0.0 && sum > tlimit
				 || klimit > 0.0 && kch >= 0 && cdev[kch] > klimit) {
					DC_INC(co);
					continue;
				}

				/* Generate the in-gamut PCS test point */
				/* by converting device to pcsin */
				if ((rv1 = luo1->lookup(luo1, pcsin, dev)) > 1)
					error ("%d, %s",icco->errc,icco->err);

				/* Now do the check */
				/* PCS -> Device */
				if ((rv2 = luo2->lookup(luo2, devout, pcsin)) > 1)
					error ("%d, %s",icco->errc,icco->err);

				/* Device to PCS */
				if ((rv2 = luo1->lookup(luo1, pcsout, devout)) > 1)
					error ("%d, %s",icco->errc,icco->err);

				/* Delta E */
				if (dovrml) {
					add_vertex(wrl, pcsin);
					add_vertex(wrl, pcsout);
				}
	
				/* Check the result */
				if (cie2k)
					de = icmCIE2K(pcsout, pcsin);
				else if (cie94)
					de = icmCIE94(pcsout, pcsin);
				else
					de = icmLabDE(pcsout, pcsin);
	
				aerr += de;
				rerr += de * de;
				if (de > merr)
					merr = de;
				nsamps++;

				if (verb > 1) {
					printf("[%f] %f %f %f -> ",de, pcsin[0], pcsin[1], pcsin[2]);
					for (n = 0; n < inn; n++)
						printf("%f ",devout[n]);
					printf("-> %f %f %f\n",pcsout[0], pcsout[1], pcsout[2]);
				}

				DC_INC(co);
			}
		}
		if (dovrml) {
			if (dodecol)
				make_de_lines(wrl);
			else
				make_lines(wrl, 2);
			end_vrml(wrl);
		}

		printf("Profile check complete, errors%s: max. = %f, avg. = %f, RMS = %f\n",
            cie2k ? "(CIEDE2000)" : cie94 ? " (CIE94)" : "", merr, aerr/nsamps, sqrt(rerr/nsamps));

		/* Done with lookup object */
		luo1->del(luo1);
		luo2->del(luo2);
	}

	icco->del(icco);
	rd_fp->del(rd_fp);

	return 0;
}


/* ------------------------------------------------ */
/* Some simple functions to do basix VRML work */
/* !!! Should change to plot/vrml lib !!! */

#define GAMUT_LCENT 50.0
static int npoints = 0;
static int paloc = 0;
static struct { double pp[3]; } *pary;

static void Lab2RGB(double *out, double *in);
static void DE2RGB(double *out, double in);

FILE *start_vrml(char *name, int doaxes) {
	FILE *wrl;

	/* Define the axis boxes */
	struct {
		double x, y, z;			/* Box center */
		double wx, wy, wz;		/* Box size */
		double r, g, b;			/* Box color */
	} axes[5] = {
		{ 0, 0,   50-GAMUT_LCENT, 2, 2, 100, .7, .7, .7 },	/* L axis */
		{ 50, 0,  0-GAMUT_LCENT,  100, 2, 2,  1,  0,  0 },	/* +a (red) axis */
		{ 0, -50, 0-GAMUT_LCENT,  2, 100, 2,  0,  0,  1 },	/* -b (blue) axis */
		{ -50, 0, 0-GAMUT_LCENT,  100, 2, 2,  0,  1,  0 },	/* -a (green) axis */
		{ 0,  50, 0-GAMUT_LCENT,  2, 100, 2,  1,  1,  0 },	/* +b (yellow) axis */
	};

	/* Define the labels */
	struct {
		double x, y, z;
		double size;
		char *string;
		double r, g, b;
	} labels[6] = {
		{ -2, 2, -GAMUT_LCENT + 100 + 10, 10, "+L*",  .7, .7, .7 },	/* Top of L axis */
		{ -2, 2, -GAMUT_LCENT - 10,      10, "0",    .7, .7, .7 },	/* Bottom of L axis */
		{ 100 + 5, -3,  0-GAMUT_LCENT,  10, "+a*",  1,  0,  0 },	/* +a (red) axis */
		{ -5, -100 - 10, 0-GAMUT_LCENT,  10, "-b*",  0,  0,  1 },	/* -b (blue) axis */
		{ -100 - 15, -3, 0-GAMUT_LCENT,  10, "-a*",  0,  0,  1 },	/* -a (green) axis */
		{ -5,  100 + 5, 0-GAMUT_LCENT,  10, "+b*",  1,  1,  0 },	/* +b (yellow) axis */
	};

	if ((wrl = fopen(name,"w")) == NULL)
		error("Error opening VRML file '%s'\n",name);

	npoints = 0;

	fprintf(wrl,"#VRML V2.0 utf8\n");
	fprintf(wrl,"\n");
	fprintf(wrl,"# Created by the Argyll CMS\n");
	fprintf(wrl,"Transform {\n");
	fprintf(wrl,"children [\n");
	fprintf(wrl,"	NavigationInfo {\n");
	fprintf(wrl,"		type \"EXAMINE\"        # It's an object we examine\n");
	fprintf(wrl,"	} # We'll add our own light\n");
	fprintf(wrl,"\n");
	fprintf(wrl,"    DirectionalLight {\n");
	fprintf(wrl,"        direction 0 0 -1      # Light illuminating the scene\n");
	fprintf(wrl,"        direction 0 -1 0      # Light illuminating the scene\n");
	fprintf(wrl,"    }\n");
	fprintf(wrl,"\n");
	fprintf(wrl,"    Viewpoint {\n");
	fprintf(wrl,"        position 0 0 340      # Position we view from\n");
	fprintf(wrl,"    }\n");
	fprintf(wrl,"\n");
	if (doaxes != 0) {
		int n;
		fprintf(wrl,"    # Lab axes as boxes:\n");
		for (n = 0; n < 5; n++) {
			fprintf(wrl,"    Transform { translation %f %f %f\n", axes[n].x, axes[n].y, axes[n].z);
			fprintf(wrl,"      children [\n");
			fprintf(wrl,"        Shape{\n");
			fprintf(wrl,"          geometry Box { size %f %f %f }\n",
			                       axes[n].wx, axes[n].wy, axes[n].wz);
			fprintf(wrl,"          appearance Appearance { material Material ");
			fprintf(wrl,"{ diffuseColor %f %f %f} }\n", axes[n].r, axes[n].g, axes[n].b);
			fprintf(wrl,"        }\n");
			fprintf(wrl,"      ]\n");
			fprintf(wrl,"    }\n");
		}
		fprintf(wrl,"    # Axes identification:\n");
		for (n = 0; n < 6; n++) {
			fprintf(wrl,"    Transform { translation %f %f %f\n", labels[n].x, labels[n].y, labels[n].z);
			fprintf(wrl,"      children [\n");
			fprintf(wrl,"        Shape{\n");
			fprintf(wrl,"          geometry Text { string [\"%s\"]\n",labels[n].string);
			fprintf(wrl,"            fontStyle FontStyle { family \"SANS\" style \"BOLD\" size %f }\n",
			                                  labels[n].size);
			fprintf(wrl,"                        }\n");
			fprintf(wrl,"          appearance Appearance { material Material ");
			fprintf(wrl,"{ diffuseColor %f %f %f} }\n", labels[n].r, labels[n].g, labels[n].b);
			fprintf(wrl,"        }\n");
			fprintf(wrl,"      ]\n");
			fprintf(wrl,"    }\n");
		}
		fprintf(wrl,"\n");
	}

	return wrl;
}

void
start_line_set(FILE *wrl) {

	fprintf(wrl,"\n");
	fprintf(wrl,"Shape {\n");
	fprintf(wrl,"  geometry IndexedLineSet { \n");
	fprintf(wrl,"    coord Coordinate { \n");
	fprintf(wrl,"	   point [\n");
}

void add_vertex(FILE *wrl, double pp[3]) {

	fprintf(wrl,"%f %f %f,\n",pp[1], pp[2], pp[0]-GAMUT_LCENT);
	
	if (paloc < (npoints+1)) {
		paloc = (paloc + 10) * 2;
		if (pary == NULL)
			pary = malloc(paloc * 3 * sizeof(double));
		else
			pary = realloc(pary, paloc * 3 * sizeof(double));

		if (pary == NULL)
			error ("Malloc failed");
	}
	pary[npoints].pp[0] = pp[0];
	pary[npoints].pp[1] = pp[1];
	pary[npoints].pp[2] = pp[2];
	npoints++;
}


void make_lines(FILE *wrl, int ppset) {
	int i, j;

	fprintf(wrl,"      ]\n");
	fprintf(wrl,"    }\n");
	fprintf(wrl,"  coordIndex [\n");

	for (i = 0; i < npoints;) {
		for (j = 0; j < ppset; j++, i++) {
			fprintf(wrl,"%d, ", i);
		}
		fprintf(wrl,"-1,\n");
	}
	fprintf(wrl,"    ]\n");

	/* Color */
	fprintf(wrl,"            colorPerVertex TRUE\n");
	fprintf(wrl,"            color Color {\n");
	fprintf(wrl,"              color [			# RGB colors of each vertex\n");

	for (i = 0; i < npoints; i++) {
		double rgb[3], Lab[3];
		Lab[0] = pary[i].pp[0];
		Lab[1] = pary[i].pp[1];
		Lab[2] = pary[i].pp[2];
		Lab2RGB(rgb, Lab);
		fprintf(wrl,"                %f %f %f,\n", rgb[0], rgb[1], rgb[2]);
	}
	fprintf(wrl,"              ] \n");
	fprintf(wrl,"            }\n");
	/* End color */

	fprintf(wrl,"  }\n");
	fprintf(wrl,"} # end shape\n");
}

/* Assume 2 ppset, and make line color prop to length */
void make_de_lines(FILE *wrl) {
	int i, j;

	fprintf(wrl,"      ]\n");
	fprintf(wrl,"    }\n");
	fprintf(wrl,"  coordIndex [\n");

	for (i = 0; i < npoints;) {
		for (j = 0; j < 2; j++, i++) {
			fprintf(wrl,"%d, ", i);
		}
		fprintf(wrl,"-1,\n");
	}
	fprintf(wrl,"    ]\n");

	/* Color */
	fprintf(wrl,"            colorPerVertex TRUE\n");
	fprintf(wrl,"            color Color {\n");
	fprintf(wrl,"              color [			# RGB colors of each vertex\n");

	for (i = 0; i < npoints; i++) {
		double rgb[3], ss;
		for (ss = 0.0, j = 0; j < 3; j++) {
			double tt = (pary[i & ~1].pp[j] - pary[i | 1].pp[j]);
			ss += tt * tt;
		}
		ss = sqrt(ss);
		DE2RGB(rgb, ss);
		fprintf(wrl,"                %f %f %f,\n", rgb[0], rgb[1], rgb[2]);
	}
	fprintf(wrl,"              ] \n");
	fprintf(wrl,"            }\n");
	/* End color */

	fprintf(wrl,"  }\n");
	fprintf(wrl,"} # end shape\n");
}

void end_vrml(FILE *wrl) {

	fprintf(wrl,"\n");
	fprintf(wrl,"  ] # end of children for world\n");
	fprintf(wrl,"}\n");

	if (fclose(wrl) != 0)
		error("Error closing VRML file\n");
}


/* Convert a gamut Lab value to an RGB value for display purposes */
static void
Lab2RGB(double *out, double *in) {
	double L = in[0], a = in[1], b = in[2];
	double x,y,z,fx,fy,fz;
	double R, G, B;

	/* Scale so that black is visible */
	L = L * (100 - 40.0)/100.0 + 40.0;

	/* First convert to XYZ using D50 white point */
	if (L > 8.0) {
		fy = (L + 16.0)/116.0;
		y = pow(fy,3.0);
	} else {
		y = L/903.2963058;
		fy = 7.787036979 * y + 16.0/116.0;
	}

	fx = a/500.0 + fy;
	if (fx > 24.0/116.0)
		x = pow(fx,3.0);
	else
		x = (fx - 16.0/116.0)/7.787036979;

	fz = fy - b/200.0;
	if (fz > 24.0/116.0)
		z = pow(fz,3.0);
	else
		z = (fz - 16.0/116.0)/7.787036979;

	x *= 0.9642;	/* Multiply by white point, D50 */
	y *= 1.0;
	z *= 0.8249;

	/* Now convert to sRGB values */
	R = x * 3.2410  + y * -1.5374 + z * -0.4986;
	G = x * -0.9692 + y * 1.8760  + z * 0.0416;
	B = x * 0.0556  + y * -0.2040 + z * 1.0570;

	if (R < 0.0)
		R = 0.0;
	else if (R > 1.0)
		R = 1.0;

	if (G < 0.0)
		G = 0.0;
	else if (G > 1.0)
		G = 1.0;

	if (B < 0.0)
		B = 0.0;
	else if (B > 1.0)
		B = 1.0;

	R = pow(R, 1.0/2.2);
	G = pow(G, 1.0/2.2);
	B = pow(B, 1.0/2.2);

	out[0] = R;
	out[1] = G;
	out[2] = B;
}

/* Convert a delta E value into a signal color: */
static void
DE2RGB(double *out, double in) {
	struct {
		double de;
		double r, g, b;
	} range[6] = {
		{ 10.0, 1, 1, 0 },		/* yellow */
		{ 4.0,  1, 0, 0 },		/* red */
		{ 2.0, 1, 0, 1 },		/* magenta */
		{ 1.0, 0, 0, 1 },		/* blue */
		{ 0.5, 0, 1, 1 },		/* cyan */
		{ 0.0, 0, 1, 0 }		/* green */
	};
	int i;
	double bl;

//printf("~1 input de = %f\n",in);

	/* Locate the range we're in */
	if (in > range[0].de) {
		out[0] = range[0].r;
		out[1] = range[0].g;
		out[2] = range[0].b;
//printf("~1 too big\n");
	} else {
		for (i = 0; i < 5; i++) {
			if (in <= range[i].de && in >= range[i+1].de)
				break;
		}
		bl = (in - range[i+1].de)/(range[i].de - range[i+1].de);
//printf("~1 located at ix %d, bl = %f\n",i,bl);
		out[0] = bl * range[i].r + (1.0 - bl) * range[i+1].r;
		out[1] = bl * range[i].g + (1.0 - bl) * range[i+1].g;
		out[2] = bl * range[i].b + (1.0 - bl) * range[i+1].b;
	}
//printf("~1 returning rgb %f %f %f\n",out[0],out[1],out[2]);
}


