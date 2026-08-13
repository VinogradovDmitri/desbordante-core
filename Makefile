.PHONY: review

# Review preparation is intentionally independent of the C++ build. Override
# PYTHON when the repository venv is available: make review PYTHON=.venv/bin/python3
PYTHON ?= python3

review:
	@MODE="$(MODE)" BASE="$(BASE)" HEAD="$(HEAD)" HOURS="$(HOURS)" \
	PHASES="$(PHASES)" REVIEW_MODES="$(REVIEW_MODES)" VERIFY="$(VERIFY)" FORCE="$(FORCE)" \
	OUTPUT_DIR="$(OUTPUT_DIR)" REPORT="$(REPORT)" PATCH_DIR="$(PATCH_DIR)" \
	$(PYTHON) llm/review_prepare.py
