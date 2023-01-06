all: uart_stim.txt example.cod

uart_stim.txt: 
	../rs232-gen -b 115200 -o uart_stim.txt -f 1843200 -c 3000:rs232-test

example.cod: example.asm
	gpasm -i -w 1 example.asm

clean:
	rm -f example.cod example.hex example.lst uart_stim.txt
