/*-----------------------------------------------------------------------------
 *  A program that simulates a scheduler in DOS.
 *
 *  The program defines four tasks that run concurrently. The tasks are
 *  suspended and resumed by a context-switching routine that is executed every
 *  ~50 ms (when the IRQ8 interruption is received.)
 *
 *  As per the assignment instructions, the four user tasks are as follows:
 *
 *  1) A Bouncing ball
 *  2) Print characters to the screen
 *  3) A clock
 *  4) Another clock
 *
 *  It uses the dos.h library provided by Turbo-C.
 * --------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>

#define IRQ8 0x08

/**
 * @brief Defines the Set of Registers available in DOS
 */
struct internal_register_set_t {
  unsigned bp;
  unsigned di;
  unsigned si;
  unsigned ds;
  unsigned es;
  unsigned dx;
  unsigned cx;
  unsigned bx;
  unsigned ax;
  unsigned ip;
  unsigned cs;
  unsigned flags;
};

#define AVAILABLE    -1
#define READY         0

#define STACK_SIZE 1024

/**
 * @brief Defines a process' information
 */
struct process_control_block_t {
  unsigned id;              //Identificador
  unsigned DirSeg;          //Semento de inicio de la tarea
  unsigned DirOff;          //Desplazamiento de inicio de la tarea
  unsigned status;          //Tiempo de espera en colo de retraso
  unsigned sp;              //Apuntador de Pila local
  unsigned ss;              //Apuntador de Pila Local
  int      state;           //Estado de la tarea
  unsigned priority;        //Prioridad de la tarea
  char    *name;            //Nombre de la tarea
  char far stack[4096];     //Espacio de Pila Local
};

typedef struct internal_register_set_t  process_registers_t;
typedef struct process_control_block_t  process_control_block_t;

typedef void (*task_t) (void);

void interrupt (*old_routine)(void);
int i = 0, j = 0;
process_control_block_t PCB[8];
process_control_block_t *running_taskP = NULL;
#define READY_QUEUE_SIZE 4
int ready_queue[READY_QUEUE_SIZE];
int next_free = 0;
int current_slot = 0;
int current_task = -1;



/*---------------------------------------------------------------------------
 *                          READY QUEUE API
 *-------------------------------------------------------------------------*/
/**
 * @brief Initialized the queue of processes that are in READY state.
 */
void init_queue()
{
  int k;
  for (k = 0; k < READY_QUEUE_SIZE; k++) {
    ready_queue[k] = AVAILABLE;
  }
}

/**
 * @brief Enqueue a READY task.
 *
 * @param task_id   Task to be enqueued.
 */
void enqueue_task(int task_id)
{
  ready_queue[next_free] = task_id;
  next_free = (next_free + 1) % READY_QUEUE_SIZE;
}

/**
 * @brief Dequeue the next task to be run.
 *
 * @return A task_id  that identifies the next process to run.
 */
int dequeue_task()
{
  while (ready_queue[current_slot] == AVAILABLE) {
    current_slot = (current_slot + 1) % READY_QUEUE_SIZE;
  }

  if (ready_queue[current_slot] != AVAILABLE) {
    int task_id = ready_queue[current_slot];
    ready_queue[current_slot] = AVAILABLE;
    return task_id;
  }
  else {
    return -1;
  }
}



/*---------------------------------------------------------------------------
 *                          SCHEDULER API
 *-------------------------------------------------------------------------*/
/**
 * @brief Function that schedules the next task to be run.
 *
 * @return Nothing.
 */
void interrupt context_switch()
{
  disable();

  if (current_task < 0) {
    printf("No task currently running\n");
  }
  else {
    PCB[current_task].ss = _SS;
    PCB[current_task].sp = _SP;
    printf("Enqueueing task: %d\n", current_task);
    enqueue_task(current_task);
  }

  current_task = dequeue_task();
  printf("Dequeued task: %d\n", current_task);

  if (current_task != -1) {
    _SS = PCB[current_task].ss;
    _SP = PCB[current_task].sp;
  }

  if (i%20 == 0) {
    j++;
    printf("Time elapsed: %d seconds\n", j);
  }

  i++;

  /* Finish the program after 20 seconds */
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
 * @brief Initializes a task.
 *
 * Captures the process info and stores it in the PCB (Process Control Block).
 *
 * @param taskP     Function pointer of the task to register.
 * @param task_id   An identified for the task.
 */
void init_task(task_t taskP, unsigned task_id)
{
  process_registers_t *regP = NULL;

  regP = (process_registers_t *) (PCB[task_id].stack + STACK_SIZE - sizeof(process_registers_t));

  PCB[task_id].sp = FP_OFF((process_registers_t far *) regP);
  PCB[task_id].ss = FP_SEG((process_registers_t far *) regP);

  regP->cs = FP_SEG(taskP);
  regP->ip = FP_OFF(taskP);
  regP->ds = _DS;
  regP->es = _DS;
  regP->flags = 0x20;

  PCB[task_id].state = READY;
  PCB[task_id].DirSeg = FP_SEG(taskP);
  PCB[task_id].DirOff = FP_OFF(taskP);

  enqueue_task(task_id);
}



/*---------------------------------------------------------------------------
 *                          USER TASKS
 *-------------------------------------------------------------------------*/
void task2()
{
  int k = 0;
  unsigned long int jj = 0;
  while (1) {
    if (++jj >= 100000) {
      printf("This is task 2. Iteration: %d\n", ++k);
      jj = 0;
    }
  }

  return;
}

void task1()
{
  int k = 0;
  unsigned long int jj = 0;
  while (1) {
    if (++jj >= 100000) {
      printf("This is task 1. Iteration: %d\n", ++k);
      jj = 0;
    }
  }

  return;
}



/**
 * The main() function:
 *  1) Sets the context_switch function to be executed when the real-time clock
 *     interrupt is triggered.
 *
 *  2) Registers four tasks.
 *
 *  3) Let the context_switch function drive the scheduling of the four tasks.
 */
int main()
{
  old_routine = getvect(IRQ8);
  setvect(IRQ8, context_switch);

  printf("Initializing task: %d\n", 1);
  init_task(task1, 1);

  printf("Initializing task: %d\n", 2);
  init_task(task2, 2);


  //init_task(task1, 3);
  //init_task(task1, 4);

  do {
  } while(!kbhit());

  setvect(IRQ8, old_routine);
  return 0;
}
