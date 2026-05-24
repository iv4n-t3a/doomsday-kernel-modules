module:
	make -C kmodule

train: kmodule/weights.h

test: utest
	./utest

utest: kmodule/weights.h kmodule/inference.h kmodule/inference.c kmodule/umain.c
	gcc -lm kmodule/inference.c kmodule/umain.c kmodule/math.c -o utest

kmodule/weights.h: utrain/config.py utrain/train.py
	python3 -m venv venv
	venv/bin/python3 -m pip install -r utrain/requirements.txt
	venv/bin/python3 utrain/train.py kmodule/weights.h

.PHONY: module train utest
