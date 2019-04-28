#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! http://waf.googlecode.com/git/docs/wafbook/single.html#_obtaining_the_waf_file

import os,sys
from waflib import Task,Utils
import waflib.Task
from waflib.Tools.ccroot import link_task,stlink_task
from waflib.TaskGen import extension,feature
class asm(Task.Task):
	color='BLUE'
	run_str='${AS} ${ASFLAGS} ${CPPPATH_ST:INCPATHS} ${AS_SRC_F}${SRC} ${AS_TGT_F}${TGT}'
def asm_hook(self,node):
	return self.create_compiled_task('asm',node)
class asmprogram(link_task):
	run_str='${ASLINK} ${ASLINKFLAGS} ${ASLNK_TGT_F}${TGT} ${ASLNK_SRC_F}${SRC}'
	ext_out=['.bin']
	inst_to='${BINDIR}'
	chmod=Utils.O755
class asmshlib(asmprogram):
	inst_to='${LIBDIR}'
class asmstlib(stlink_task):
	pass

extension('.s','.S','.asm','.ASM','.spp','.SPP')(asm_hook)