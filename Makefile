WAF=python tools/waf-light --jobs=1

all:
	@$(WAF) build

all-debug:
	@$(WAF) -v build

all-progress:
	@$(WAF) -p build

install:
	@$(WAF) install

uninstall:
	@$(WAF) uninstall

clean:
	@$(WAF) clean

distclean:
	@-rm -rf build/
	@-find tools/ -name "*.pyc" -delete

check:
	@tools/waf-light check

VERSION=$(shell git describe)
TARNAME=meguro-$(VERSION)

dist: 
	git archive --prefix=$(TARNAME)/ HEAD > $(TARNAME).tar
	gzip -f -9 $(TARNAME).tar

.PHONY: benchmark clean dist distclean check uninstall install all
