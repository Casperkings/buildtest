include Makefile.common

XTENSA_TOOLS = `xt-run --xtensa-core=$(CORE) --show-config=xttools`
TC_TOOLS = `xt-run  --xtensa-core=$(CORE) --show-config=tctools`

TESTS = xipc_barrier xipc_cond xipc_counted_event xipc_cqueue xipc_msg_channel xipc_pkt_channel xipc_mutex xipc_sem xipc_primitives xipc_rpc xipc_rpc_shared

default:
	for T in $(TESTS) ; do \
    cd $$T && make clean; make build; make; make xtsc-run; cd .. ; \
  done 

build:
	$(XTENSA_TOOLS)/libexec/xt-mbuild -root ./ -ten -tctools $(TC_TOOLS) -socfile $(XTSYS) -preclean -build sw
	$(XTENSA_TOOLS)/libexec/xt-sysbuilder -subsys $(YML) -swtools $(XTENSA_TOOLS) -build sys-x86_64-linux/subsys.xt_sysbuild 

clean:
	for T in $(TESTS) ; do \
		cd $$T && make clean; \
		cd ..; \
	done
