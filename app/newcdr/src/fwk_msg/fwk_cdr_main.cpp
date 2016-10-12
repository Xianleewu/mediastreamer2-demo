/*   ----------------------------------------------------------------------
Copyright (C) 2014-2016 Fuzhou Rockchip Electronics Co., Ltd

     Sec Class: Rockchip Confidential

V1.0 Dayao Ji <jdy@rock-chips.com>
---------------------------------------------------------------------- */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "fwk_gl_api.h"
#include "fwk_gl_def.h"
#include "fwk_gl_msg.h"



int main(int argc, char *argv[])
{

	fwk_controller_init();
	fwk_view_init();

	while(1)
	{
		
		sleep(10);
	}
	return 0;
}



