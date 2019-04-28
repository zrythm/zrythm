#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! http://waf.googlecode.com/git/docs/wafbook/single.html#_obtaining_the_waf_file

import sys
if sys.hexversion < 0x020400f0: from sets import Set as set
from waflib import Utils,Task,Options,Logs,Errors
from waflib.TaskGen import before_method,after_method,feature
from waflib.Tools import ccroot
from waflib.Configure import conf
ccroot.USELIB_VARS['cs']=set(['CSFLAGS','ASSEMBLIES','RESOURCES'])
ccroot.lib_patterns['csshlib']=['%s']
def apply_cs(self):
	cs_nodes=[]
	no_nodes=[]
	for x in self.to_nodes(self.source):
		if x.name.endswith('.cs'):
			cs_nodes.append(x)
		else:
			no_nodes.append(x)
	self.source=no_nodes
	bintype=getattr(self,'type',self.gen.endswith('.dll')and'library'or'exe')
	self.cs_task=tsk=self.create_task('mcs',cs_nodes,self.path.find_or_declare(self.gen))
	tsk.env.CSTYPE='/target:%s'%bintype
	tsk.env.OUT='/out:%s'%tsk.outputs[0].abspath()
	inst_to=getattr(self,'install_path',bintype=='exe'and'${BINDIR}'or'${LIBDIR}')
	if inst_to:
		mod=getattr(self,'chmod',bintype=='exe'and Utils.O755 or Utils.O644)
		self.install_task=self.bld.install_files(inst_to,self.cs_task.outputs[:],env=self.env,chmod=mod)
def use_cs(self):
	names=self.to_list(getattr(self,'use',[]))
	get=self.bld.get_tgen_by_name
	for x in names:
		try:
			y=get(x)
		except Errors.WafError:
			self.cs_task.env.append_value('CSFLAGS','/reference:%s'%x)
			continue
		y.post()
		tsk=getattr(y,'cs_task',None)or getattr(y,'link_task',None)
		if not tsk:
			self.bld.fatal('cs task has no link task for use %r'%self)
		self.cs_task.dep_nodes.extend(tsk.outputs)
		self.cs_task.set_run_after(tsk)
		self.cs_task.env.append_value('CSFLAGS','/reference:%s'%tsk.outputs[0].abspath())
def debug_cs(self):
	csdebug=getattr(self,'csdebug',self.env.CSDEBUG)
	if not csdebug:
		return
	node=self.cs_task.outputs[0]
	if self.env.CS_NAME=='mono':
		out=node.parent.find_or_declare(node.name+'.mdb')
	else:
		out=node.change_ext('.pdb')
	self.cs_task.outputs.append(out)
	try:
		self.install_task.source.append(out)
	except AttributeError:
		pass
	if csdebug=='pdbonly':
		val=['/debug+','/debug:pdbonly']
	elif csdebug=='full':
		val=['/debug+','/debug:full']
	else:
		val=['/debug-']
	self.cs_task.env.append_value('CSFLAGS',val)
class mcs(Task.Task):
	color='YELLOW'
	run_str='${MCS} ${CSTYPE} ${CSFLAGS} ${ASS_ST:ASSEMBLIES} ${RES_ST:RESOURCES} ${OUT} ${SRC}'
def configure(conf):
	csc=getattr(Options.options,'cscbinary',None)
	if csc:
		conf.env.MCS=csc
	conf.find_program(['csc','mcs','gmcs'],var='MCS')
	conf.env.ASS_ST='/r:%s'
	conf.env.RES_ST='/resource:%s'
	conf.env.CS_NAME='csc'
	if str(conf.env.MCS).lower().find('mcs')>-1:
		conf.env.CS_NAME='mono'
def options(opt):
	opt.add_option('--with-csc-binary',type='string',dest='cscbinary')
class fake_csshlib(Task.Task):
	color='YELLOW'
	inst_to=None
	def runnable_status(self):
		for x in self.outputs:
			x.sig=Utils.h_file(x.abspath())
		return Task.SKIP_ME
def read_csshlib(self,name,paths=[]):
	return self(name=name,features='fake_lib',lib_paths=paths,lib_type='csshlib')

feature('cs')(apply_cs)
before_method('process_source')(apply_cs)
feature('cs')(use_cs)
after_method('apply_cs')(use_cs)
feature('cs')(debug_cs)
after_method('apply_cs','use_cs')(debug_cs)
conf(read_csshlib)