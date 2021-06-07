# 过滤掉当前目录里的makefile文件
DIRS =$(filter-out %makefile,$(wildcard *))
OBJS =$(patsubst %.cpp, %.o, $(SRCS))
TRGS =$(patsubst %.cpp, %, $(SRCS))

.PHONY: all

all: 
	$(foreach N,$(DIRS),$(MAKE) -C $(N);)

# 下面这种也行，还是foreach方便
#$(MAKE) -C $@

clean:
	$(foreach N,$(DIRS),$(MAKE) clean -C $(N);)

# 下面这种看着行，实际上不行，dirs和DIRS一样的话，make clean指令和make指令会冲突
#$(dirs):
#	cd $@ && $(MAKE) clean
