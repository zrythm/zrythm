#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! https://waf.io/book/index.html#_obtaining_the_waf_file

import sys,os,json,shlex,pipes
from waflib import Logs,TaskGen,Task
Task.Task.keep_last_cmd=True
@TaskGen.feature('c','cxx')
@TaskGen.after_method('process_use')
def collect_compilation_db_tasks(self):
	try:
		clang_db=self.bld.clang_compilation_database_tasks
	except AttributeError:
		clang_db=self.bld.clang_compilation_database_tasks=[]
		self.bld.add_post_fun(write_compilation_database)
	tup=tuple(y for y in[Task.classes.get(x)for x in('c','cxx')]if y)
	for task in getattr(self,'compiled_tasks',[]):
		if isinstance(task,tup):
			clang_db.append(task)
def write_compilation_database(ctx):
	database_file=ctx.bldnode.make_node('compile_commands.json')
	Logs.info('Build commands will be stored in %s',database_file.path_from(ctx.path))
	try:
		root=json.load(database_file)
	except IOError:
		root=[]
	clang_db=dict((x['file'],x)for x in root)
	for task in getattr(ctx,'clang_compilation_database_tasks',[]):
		try:
			cmd=task.last_cmd
		except AttributeError:
			continue
		directory=getattr(task,'cwd',ctx.variant_dir)
		f_node=task.inputs[0]
		filename=os.path.relpath(f_node.abspath(),directory)
		entry={"directory":directory,"arguments":cmd,"file":filename,}
		clang_db[filename]=entry
	root=list(clang_db.values())
	database_file.write(json.dumps(root,indent=2))
for x in('c','cxx'):
	if x not in Task.classes:
		continue
	t=Task.classes[x]
	def runnable_status(self):
		def exec_command(cmd,**kw):
			pass
		run_status=self.old_runnable_status()
		if run_status==Task.SKIP_ME:
			setattr(self,'old_exec_command',getattr(self,'exec_command',None))
			setattr(self,'exec_command',exec_command)
			self.run()
			setattr(self,'exec_command',getattr(self,'old_exec_command',None))
		return run_status
	setattr(t,'old_runnable_status',getattr(t,'runnable_status',None))
	setattr(t,'runnable_status',runnable_status)
