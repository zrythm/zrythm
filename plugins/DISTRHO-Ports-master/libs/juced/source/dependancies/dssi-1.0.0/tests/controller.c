/*
 *  Copyright (C) 2004 Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: controller.c,v 1.2 2004/03/11 19:01:00 theno23 Exp $
 */

#include <stdio.h>
#include <dssi.h>

int main()
{
    int controller = DSSI_NONE;

    if (DSSI_CONTROLLER_IS_SET(controller)) {
	printf("DSSI_IS_SET failed %s:%d\n", __FILE__, __LINE__);
	return 1;
    }

    controller = DSSI_CC(23);
    if (!DSSI_IS_CC(controller)) {
	printf("DSSI_IS_CC failed %s:%d\n", __FILE__, __LINE__);
	return 1;
    }
    if (DSSI_CC_NUMBER(controller) != 23) {
	printf("DSSI_CC_NUMBER failed (%d != 23) %s:%d\n",
		DSSI_CC_NUMBER(controller), __FILE__, __LINE__);
	return 1;
    }
    
    controller |= DSSI_NRPN(1069);
    if (!DSSI_IS_CC(controller)) {
	printf("DSSI_IS_CC failed %s:%d\n", __FILE__, __LINE__);
	return 1;
    }
    if (DSSI_CC_NUMBER(controller) != 23) {
	printf("DSSI_CC_NUMBER failed %s:%d\n", __FILE__, __LINE__);
	return 1;
    }
    if (!DSSI_IS_NRPN(controller)) {
	printf("DSSI_IS_NRPN failed %s:%d\n", __FILE__, __LINE__);
	return 1;
    }
    if (DSSI_NRPN_NUMBER(controller) != 1069) {
	printf("DSSI_NRPN_NUMBER failed (%d != 1069) %s:%d\n",
		DSSI_NRPN_NUMBER(controller), __FILE__, __LINE__);
	return 1;
    }

    printf("test passed\n");

    return 0;
}

/* vi:set ts=8 sts=4 sw=4: */
