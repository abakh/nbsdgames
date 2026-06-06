.PHONY: all
all: $(MAKECMDGOALS)
$(MAKECMDGOALS):	
	@echo "Sources and resulting binaries are in src/ now, to avoid visually polluting the Github page."
	@echo "This script was made to avoid breaking package builds."
	@cd ./src && make $@ $(MAKE_FLAGS)
	find ./src -maxdepth 1 -type f -executable -print0 | xargs -0 -I {} cp {} .
