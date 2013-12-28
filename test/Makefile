#############################################################################
#
# C/C++ 程序通用 Makefile
#
# 用法:
# ------
# 1. 拷贝本文件到你的程序目录
# 2. 在 "自定义" 节中进行必要的定制:
#    * 想要使用非标准的 c/c++ 库, 设置预处理器或编译器选项  <MY_CFLAGS> 和连接选项
#      <MY_LIBS> 
#    * 想要在更多的止目录中搜索源文件, 设置  <SRCDIRS>
#    * 想要指定你喜欢的程序名, 设置 <PROGRAM>
# 3. 输入 make 开始编辑你的程序
#
# Make 目标:
# ------------
# 本 Makefile 提供下列 make 目标:
#   $ make           编译并连接
#   $ make NODEP=yes 编译并连接, 但不生成依赖项
#   $ make objs      仅编译 (不连接)
#   $ make tags      为 emacs 创建  tags
#   $ make ctags     为 VI 创建  ctags
#   $ make clean     清理临时文件和可执行程序
#   $ make distclean 清理临时文件、可执行程序和依赖项
#   $ make show      显示变量  (仅用于调试)
#   $ make install      安装
#   $ make help      获取 makefile 的用法
#
#===========================================================================

## 自定义: 自定义这些变量, 使它们适合你的程序
##==========================================================================
PROGRAM_NAME=test

# 预处理器或编译器选项.
MY_CFLAGS = -I../include/pr -I../include/io

# 连接选项, 如各个依赖库.
ifeq ($(OS_STYLE),win)
MY_LIBS   =-lws2_32 -lMU -L.
endif

# 用于 cpp 的预处理选项 (自已查 cpp 手册).
CPPFLAGS  = -Wall -I.

# 连接选项 .
LDFLAGS   =  -Wl 

# 指定源文件的路径, 如果不指定则仅在当前目录下找到.
SRCDIRS   =

ifndef DLL
DLL = 0
endif

# 可执行程序名, 如果不指定则为当前目录名或a.out.
PROGRAM   = $(PROGRAM_NAME).exe

## 隐含节: 在必要时更改它
##==========================================================================

# 源文件后缀名 (不包括头).
SRCEXTS = .c .C .cc .cpp .CPP .c++ .cxx .cp

# 头文件类型.
HDREXTS = .h .H .hh .hpp .HPP .h++ .hxx .hp

# 预处理器或编译器选项.
# 用户可以命令行中覆盖这些选项.
CFLAGS  = -g -O2
CXXFLAGS= -g -O2

# c 程序的编译器.
CC     = gcc

# c++ 程序的编译器.
#CXX    = g++

# 如果想用c++ 程序的编译器代替  c++ 程序的编译器 , 取消下面行的注释.
#CC     = $(CXX)

# 删除文件的命令.
RM     = rm -f

ETAGS = etags
ETAGSFLAGS =

CTAGS = ctags
CTAGSFLAGS =

## 静态节: 通常不需要改变它. 但它可以增加.
##==========================================================================
SHELL   = /bin/sh
EMPTY   =
SPACE   = $(EMPTY) $(EMPTY)

ifeq ($(PROGRAM),)
  CUR_PATH_NAMES = $(subst /,$(SPACE),$(subst $(SPACE),_,$(CURDIR)))
  PROGRAM = $(word $(words $(CUR_PATH_NAMES)),$(CUR_PATH_NAMES))
  ifeq ($(PROGRAM),)
    PROGRAM = a.out
  endif
endif
ifeq ($(SRCDIRS),)
  SRCDIRS = .
endif


SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
HEADERS = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(HDREXTS))))
SRC_CXX = $(filter-out %.c,$(SOURCES))
OBJS    = $(addsuffix .o, $(basename $(SOURCES)))
DEPS    = $(OBJS:.o=.d)

## 定义一些有用的变量.
DEP_OPT = $(shell if `$(CC) --version | grep "GCC" >/dev/null`; then \
                  echo "-MM -MP"; else echo "-M"; fi )
DEPEND      = $(CC)  $(DEP_OPT)  $(MY_CFLAGS) $(CFLAGS) $(CPPFLAGS)
DEPEND.d    = $(subst -g ,,$(DEPEND))
COMPILE.c   = $(CC)  $(MY_CFLAGS) $(CFLAGS)   $(CPPFLAGS) -c
COMPILE.cxx = $(CXX) $(MY_CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -c
LINK.c      = $(CC)  $(MY_CFLAGS) $(CFLAGS)   $(CPPFLAGS) $(LDFLAGS)
LINK.cxx    = $(CXX) $(MY_CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS)

.PHONY: all objs tags ctags clean distclean help show

# Delete the default suffixes
.SUFFIXES:

all: $(PROGRAM)

# 创建 dependency 文件(.d)的规则 .
#------------------------------------------

%.d:%.c
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@

%.d:%.cc
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@

%.d:%.cpp
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@

%.d:%.CPP
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@

%.d:%.c++
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@

%.d:%.cp
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@

%.d:%.cxx
	@echo -n $(dir $<) > $@
	@$(DEPEND.d) $< >> $@

# 生成 object 文件 (.o) 的规则.
#----------------------------------------
objs:$(OBJS)

%.o:%.c
	$(COMPILE.c) $< -o $@

%.o:%.C
	$(COMPILE.cxx) $< -o $@

%.o:%.cc
	$(COMPILE.cxx) $< -o $@

%.o:%.cpp
	$(COMPILE.cxx) $< -o $@

%.o:%.CPP
	$(COMPILE.cxx) $< -o $@

%.o:%.c++
	$(COMPILE.cxx) $< -o $@

%.o:%.cp
	$(COMPILE.cxx) $< -o $@

%.o:%.cxx
	$(COMPILE.cxx) $< -o $@

# 生成 tags 的规则.
#-------------------------------------
tags: $(HEADERS) $(SOURCES)
	$(ETAGS) $(ETAGSFLAGS) $(HEADERS) $(SOURCES)

ctags: $(HEADERS) $(SOURCES)
	$(CTAGS) $(CTAGSFLAGS) $(HEADERS) $(SOURCES)

# 生成 可执行文件 的规则.
#-------------------------------------
$(PROGRAM_NAME).exe:$(OBJS)
ifeq ($(SRC_CXX),)              # C program
	$(LINK.c) $(OBJS) $(MY_LIBS) -o $@
	@echo Type ./$@ to execute the program.
else                            # C++ program
	$(LINK.cxx) $(OBJS) $(MY_LIBS) -o $@
	@echo Type ./$@ to execute the program.
endif

ifndef NODEP
ifneq ($(DEPS),)
  sinclude $(DEPS)
endif
endif

clean:
	@echo aa
	$(RM) $(OBJS) $(PROGRAM) $(PROGRAM_NAME).a
	@echo bb

distclean: clean
	$(RM) $(DEPS) TAGS
	@echo cc

# 安装到指定的目录
install:
ifeq ($(DLL),1)
	$(INSTALL) $(PROGRAM_NAME).so  $(INSTPREFIX)/lib
else
	$(INSTALL) $(PROGRAM_NAME).a  $(INSTPREFIX)/lib
endif
ifeq ($(OS_STYLE),win)
	$(INSTALL) $(PROGRAM_NAME).a $(INSTPREFIX)/lib
endif

uninstall:
ifeq ($(DLL),1)
	$(RM) $(INSTPREFIX)/lib/$(PROGRAM_NAME).so
else
	$(RM) $(INSTPREFIX)/lib/$(PROGRAM_NAME).a
endif
ifeq ($(OS_STYLE),win)
	$(RM) $(INSTPREFIX)/lib/$(PROGRAM_NAME).a
endif

# 显示帮助.
help:
	@echo 'C/C++ 程序通用 Makefile (gcmakefile) version 0.5'
	@echo
	@echo '用法: make [目标]'
	@echo '目标:'
	@echo '  all       编译并连接.'
	@echo '  NODEP=yes 编译并连接, 但不生成依赖项.'
	@echo '  objs      仅编译 (不连接).'
	@echo '  tags      为 emacs 创建  tags.'
	@echo '  ctags     为 VI 创建  ctags.'
	@echo '  clean     清理临时文件和可执行程序.'
	@echo '  distclean 清理临时文件、可执行程序和依赖项.'
	@echo '  show      显示变量  (仅用于调试).'
	@echo '  install   安装软件'
	@echo '  help      获取 makefile 的用法.'
	@echo

# 显示变量  (仅用于调试.)
show:
	@echo 'PROGRAM     :' $(PROGRAM)
	@echo 'SRCDIRS     :' $(SRCDIRS)
	@echo 'HEADERS     :' $(HEADERS)
	@echo 'SOURCES     :' $(SOURCES)
	@echo 'SRC_CXX     :' $(SRC_CXX)
	@echo 'OBJS        :' $(OBJS)
	@echo 'DEPS        :' $(DEPS)
	@echo 'DEPEND      :' $(DEPEND)
	@echo 'COMPILE.c   :' $(COMPILE.c)
	@echo 'COMPILE.cxx :' $(COMPILE.cxx)
	@echo 'link.c      :' $(LINK.c)
	@echo 'link.cxx    :' $(LINK.cxx)

## Makefile 的结尾  
#############################################################################
