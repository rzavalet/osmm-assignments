#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#define IRQ8 0x08

void interrupt (*old_routine)(void);

int i = 0, j = 0;

void interrupt custom_routine()
{
  disable();

  //printf("I am here...\n");

  if (i%20 == 0) {
	j++;
	printf("Second %d\n", j);
  }

  i++;

  /* Finish the program after 20 seconds */
  if (j >= 20) {
	setvect(IRQ8, old_routine);
	old_routine();
	enable();																																																																						;
	exit (0);
  }

  old_routine();
  enable();
}

int main()
{
	printf("Starting program\n");
	old_routine = getvect(IRQ8);
	setvect(IRQ8, custom_routine);

	do {
	} while(!kbhit());

	setvect(IRQ8, old_routine);
	return 0;
}
