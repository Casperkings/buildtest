XT_RUN=$(XTENSA_SYSTEM)/../bin/xt-run
ifeq (,$(wildcard $(XT_RUN)))
  # Fall back to xt-run from PATH
  XT_RUN=xt-run
endif

XTCC=$(XTENSA_SYSTEM)/../bin/xt-clang
ifeq (,$(wildcard $(XTCC)))
  # Fall back to xtsc-run from PATH
  XTCC=xt-clang
endif

CC_FLAGS = -O3 -mcoproc -fmessage-length=0 -DIDMA_APP_USE_XTOS

CC_FLAGS += -DPROFILE_ENABLE

LSP ?= sim-stacklocal
MEM_DELAY ?= 1

SRC_FILES = matMul.c
INCLUDES = 
OBJ_FILES = $(SRC_FILES:.c=.o)

EXEC = matMul.out

$(EXEC) : $(OBJ_FILES) 
	$(XTCC) -o $@ $(OBJ_FILES) -lidma-xtos -lm -mlsp=$(LSP)

$(OBJ_FILES) : %.o : %.c
	$(XTCC) $(INCLUDES) $(CC_FLAGS) -c $^ -o $@

XT_RUN_PARAMS := --mem_model --read_delay=$(MEM_DELAY) --write_delay=$(MEM_DELAY) --write_repeat=1 --read_repeat=1

run: $(EXEC)
	$(XT_RUN) $(XT_RUN_PARAMS) $(EXEC)

clean:
	rm -rf *.o *.out
