default: gaia

dirs: src contrib

clean:
	@$(MAKE) $@ -C src

.DEFAULT:
	@$(MAKE) $@ -C src
