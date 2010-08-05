
CFLAGS = -g -O2 -shared -fPIC -Wall -pedantic

all: app_voicechanger.so

app_voicechanger.so: app_voicechanger.o voicechanger.o
	gcc $(CFLAGS) -o $@ $^

app_voicechanger.o: app_voicechanger.c
	gcc $(CFLAGS) --std=gnu99 -c -o $@ $<

voicechanger.o: voicechanger.cpp
	g++ $(CFLAGS) -c -o $@ $< -lSoundTouch

install: all
	cp -av app_voicechanger.so /usr/lib/asterisk/modules/

clean:
	rm -f *.o *.so
