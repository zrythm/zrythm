#! /usr/bin/env python
# encoding: utf-8
# WARNING! Do not edit! http://waf.googlecode.com/git/docs/wafbook/single.html#_obtaining_the_waf_file

import sys
from waflib.Tools import ar,d
from waflib.Configure import conf
def find_gdc(conf):
	conf.find_program('gdc',var='D')
def common_flags_gdc(conf):
	v=conf.env
	v['DFLAGS']=[]
	v['D_SRC_F']=['-c']
	v['D_TGT_F']='-o%s'
	v['D_LINKER']=v['D']
	v['DLNK_SRC_F']=''
	v['DLNK_TGT_F']='-o%s'
	v['DINC_ST']='-I%s'
	v['DSHLIB_MARKER']=v['DSTLIB_MARKER']=''
	v['DSTLIB_ST']=v['DSHLIB_ST']='-l%s'
	v['DSTLIBPATH_ST']=v['DLIBPATH_ST']='-L%s'
	v['LINKFLAGS_dshlib']=['-shared']
	v['DHEADER_ext']='.di'
	v.DFLAGS_d_with_header='-fintfc'
	v['D_HDR_F']='-fintfc-file=%s'
def configure(conf):
	conf.find_gdc()
	conf.load('ar')
	conf.load('d')
	conf.common_flags_gdc()
	conf.d_platform_flags()

conf(find_gdc)
conf(common_flags_gdc)