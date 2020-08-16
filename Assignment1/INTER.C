/*-----------------------------------------------------------------------------
 *  A program that executes a routine upon receiving the 
 *  IRQ8 interrupt(real-time clock), which happens every ~50 milliseconds.
 *
 *  It uses the dos.h library provided by Turbo-C.
 * --------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#define IRQ8 0x08

void interrupt (*old_routine)(void);

int i = 0, j = 0;

/**
 * @brief Function that prints a message every ~1 second.
 *
 * @return Nothing
 */
void interrupt custom_routine()
{
  disable();

  /* Print something every ~1 second */
  if (i%20 == 0) {
    j++;
    printf("Second %d\n", j);
  }

  i++;

  /* Finish the program after 20 seconds. We need to re-establish the original
   * interruption routine, before ending the program. */
  if (j >= 20) {
    setvect(IRQ8, old_routine);
    old_routine();
    enable();
    exit (0);
  }

  old_routine();
  enable();
}

/**
 * The main function simply registers a routine to be executed upon receiving
 * the IRQ8 interruption.
 */
int main()
{
  printf("Starting program\n");

  /* Save the default interruption routine and replace it with a custome one */
  old_routine = getvect(IRQ8);
  setvect(IRQ8, custom_routine);

  /* Just let the program run forever. The interrupt routine will be called
   * frequently during the rest of the time. */
  do {
  } while(!kbhit());

  setvect(IRQ8, old_routine);
  return 0;
}
