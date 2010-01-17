#ifndef MUNKI_H

/* 
 * Argyll Color Correction System
 *
 * X-Rite ColorMunki related defines
 *
 * Author: Graeme W. Gill
 * Date:   12/1/2009
 *
 * Copyright 2006 - 2009, Graeme W. Gill
 * All rights reserved.
 *
 * This material is licenced under the GNU AFFERO GENERAL PUBLIC LICENSE Version 3 :-
 * see the License.txt file for licencing details.
 *
 * (Based on i1pro.h)
 */

/* 
   If you make use of the instrument driver code here, please note
   that it is the author(s) of the code who take responsibility
   for its operation. Any problems or queries regarding driving
   instruments with the Argyll drivers, should be directed to
   the Argyll's author(s), and not to any other party.

   If there is some instrument feature or function that you
   would like supported here, it is recommended that you
   contact Argyll's author(s) first, rather than attempt to
   modify the software yourself, if you don't have firm knowledge
   of the instrument communicate protocols. There is a chance
   that an instrument could be damaged by an incautious command
   sequence, and the instrument companies generally cannot and
   will not support developers that they have not qualified
   and agreed to support.
 */

#include "inst.h"

/* MUNKI communication object */
struct _munki {
	INST_OBJ_BASE

	int       dtype;			/* Device type: 0 = ?? */	

	/* *** munki private data **** */
	inst_capability  cap;		/* Instrument capability */
	inst2_capability cap2;		/* Instrument capability 2 */

	void *m;					/* Implementation - munkiimp type */

	/* Other state */
	int     led_state;			/* : Current LED on/off state */
	double	led_period, led_on_time_prop, led_trans_time_prop;	/* Pulse state */

}; typedef struct _munki munki;

/* Constructor */
extern munki *new_munki(icoms *icom, int debug, int verb);

#define MUNKI_H
#endif /* MUNKI_H */
