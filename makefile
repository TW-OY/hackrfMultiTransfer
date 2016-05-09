edit:
	cc -o test hackrfMultiTransfer.c `pkg-config --libs --cflags libhackrf`

.PHONY: clean
clean:
	rm test
