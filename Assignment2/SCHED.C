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
 *  To build:
 *    TCC -IC:\TURBOC3\INCLUDE -LC:\TURBOC3\LIB SCHED.C
 * --------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <conio.h>
#include <time.h>

#define IRQ8 0x08

#define TICK 1

#define SCREEN_X  0
#define SCREEN_Y  0
#define SCREEN_W  80
#define SCREEN_H  26

#if 0
#define DEBUG
#endif

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

struct panel_t {
  unsigned int  x;
  unsigned int  y;

  unsigned int  width;
  unsigned int  height;
};

/**
 * @brief Defines a process' information
 */
struct process_control_block_t {
  unsigned        id;       //Identificador
  char           *name;     //Nombre de la tarea
  int             state;    //Estado de la tarea
  unsigned        priority; //Prioridad de la tarea
  struct panel_t  panel;    // Process screen definitions.
  int             where_x;
  int             where_y;

  unsigned DirSeg;          //Semento de inicio de la tarea
  unsigned DirOff;          //Desplazamiento de inicio de la tarea
  unsigned status;          //Tiempo de espera en colo de retraso
  unsigned sp;              //Apuntador de Pila local
  unsigned ss;              //Apuntador de Pila Local

  char far stack[4096];     //Espacio de Pila Local
};

typedef struct internal_register_set_t  process_registers_t;
typedef struct process_control_block_t  process_control_block_t;

typedef void (*task_t) (void);

void interrupt (*old_routine)(void);
int i = 0, j = 0;
process_control_block_t PCB[8];
process_control_block_t *running_taskP = NULL;

typedef struct {
#define READY_QUEUE_SIZE 4
  int slots[READY_QUEUE_SIZE];

  int front;
  int rear;
  int count;

  int next_free;
  int current_slot;
} queue_t;

queue_t ready_queue;
int current_task = -1;



/*---------------------------------------------------------------------------
 *                          READY QUEUE API
 *-------------------------------------------------------------------------*/
/**
 * @brief Initialized the queue of processes that are in READY state.
 */
void init_queue(queue_t *q)
{
  int k;
  for (k = 0; k < READY_QUEUE_SIZE; k++) {
    q->slots[k] = AVAILABLE;
  }

  q->count = 0;
  q->front = 0;
  q->rear = -1;
}

/**
 * @brief Check if the queue is empty
 *
 * @param q - pointer to the queue structure
 *
 * @return 1 if empty; 0 otherwise
 */
int is_empty(queue_t *q)
{
  return q->count == 0;
}

/**
 * @brief Check if the queue is full
 *
 * @param q - pointer to the queue structure
 *
 * @return 1 if full; 0 otherwise
 */
int is_full(queue_t *q)
{
  return q->count == READY_QUEUE_SIZE;
}

/**
 * @brief Return the number of elements in the queue
 *
 * @param q - pointer to the queue structure
 *
 * @return The number of elements in the queue
 */
int size(queue_t *q)
{
  return q->count;
}

/**
 * @brief Add an element to the queue
 *
 * @param q - pointer to the queue structure
 * @param value - value to enqueue
 */
void enqueue(queue_t *q, int value)
{
  if (!is_full(q)) {
    q->rear = (q->rear + 1) % READY_QUEUE_SIZE;
    q->slots[q->rear] = value;
    q->count ++;
  }
}

/**
 * @brief Gets the next element in the queue.
 *
 * @param q - pointer to the queue structure.
 *
 * @return The next element in the queue or -1 if empty
 */
int dequeue(queue_t *q) 
{
  int data = -1;

  if (!is_empty(q)) {
    data = q->slots[q->front];

    q->front = (q->front + 1) % READY_QUEUE_SIZE;
    q->count --;
  }

  return data;
}





/*---------------------------------------------------------------------------
 *                          SCHEDULER API
 *-------------------------------------------------------------------------*/
/**
 * @brief Enqueue a READY task.
 *
 * @param task_id   Task to be enqueued.
 */
void enqueue_task(int task_id)
{
  enqueue(&ready_queue, task_id);
}

/**
 * @brief Dequeue the next task to be run.
 *
 * @return A task_id  that identifies the next process to run.
 */
int dequeue_task()
{
  return dequeue(&ready_queue);
}

/**
 * @brief Function that schedules the next task to be run.
 *
 * @return Nothing.
 */
void interrupt context_switch()
{
  disable();

  if (current_task < 0) {
#ifdef DEBUG
    printf("No task currently running\n");
#endif
  }
  else {
    PCB[current_task].ss = _SS;
    PCB[current_task].sp = _SP;

    PCB[current_task].where_x = wherex();
    PCB[current_task].where_y = wherey();

#ifdef DEBUG
    printf("Enqueueing task: %d\n", current_task);
#endif
    enqueue_task(current_task);
  }

  current_task = dequeue_task();
#ifdef DEBUG
  printf("Dequeued task: %d\n", current_task);
#endif

  if (i%20 == 0) {
    j++;
#ifdef DEBUG
    printf("Time elapsed: %d seconds\n", j);
#endif
  }

  i++;

  /* Finish the program after 20 seconds */
  if (j >= 10) {
    setvect(IRQ8, old_routine);
    old_routine();
    enable();
    exit (0);
  }

  if (current_task != -1) {
    gotoxy(PCB[current_task].where_x, PCB[current_task].where_y);
    _SS = PCB[current_task].ss;
    _SP = PCB[current_task].sp;
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
void init_task(task_t taskP, unsigned task_id, const char *task_name, 
               unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
  process_registers_t *regP = NULL;

  printf("Initializing task: %u: %s\n", task_id, task_name);

  regP = (process_registers_t *) (PCB[task_id].stack + STACK_SIZE - sizeof(process_registers_t));

  PCB[task_id].sp = FP_OFF((process_registers_t far *) regP);
  PCB[task_id].ss = FP_SEG((process_registers_t far *) regP);

  regP->cs = FP_SEG(taskP);
  regP->ip = FP_OFF(taskP);
  regP->ds = _DS;
  regP->es = _DS;
  regP->flags = 0x20;

  PCB[task_id].DirSeg = FP_SEG(taskP);
  PCB[task_id].DirOff = FP_OFF(taskP);

  PCB[task_id].id = task_id;
  PCB[task_id].name = strdup(task_name);
  PCB[task_id].state = READY;
  PCB[task_id].priority = 0;
  PCB[task_id].panel.x = x;
  PCB[task_id].panel.y = y;
  PCB[task_id].panel.width = width;
  PCB[task_id].panel.height = height;

  PCB[task_id].where_x = 0;
  PCB[task_id].where_y = 0;

  enqueue_task(task_id);
}



/*---------------------------------------------------------------------------
 *                          USER TASKS
 *-------------------------------------------------------------------------*/
void task1()
{
  int k = 0;
  unsigned long int jj = 0;

  unsigned int x = SCREEN_X + 1;
  unsigned int y = SCREEN_Y + 1;

  while (1) {
    if (++jj >= 1000) {
      gotoxy(x,y);
      printf("This is task 1. Iteration: %d", ++k);
      jj = 0;
    }
  }

  return;
}

void characters()
{
  unsigned long int jj = 0;
  unsigned int x = SCREEN_X;
  unsigned int y = SCREEN_Y + SCREEN_H / 2 + 1;
  unsigned int w = SCREEN_W / 2 - 2;
  unsigned int h = SCREEN_H / 2 - 2;

  unsigned int cur_x = x;
  unsigned int cur_y = y;
  unsigned int min_char = '!';
  unsigned int max_char = '~';
  unsigned int cur_char = min_char;

  while (1) {
    if (++jj >= 1000) {
      jj = 0;

      gotoxy(cur_x, cur_y);
      printf("%c", cur_char);
      //putchar(cur_char);

      if (++ cur_x > x + w) {
        cur_x = x;
        
        if (++ cur_y > y + h) {
          cur_y = y;
        }
      }

      if (++ cur_char > max_char) {
        cur_char = min_char;
      }
    }
  }

  return;
}

void clock1()
{
  time_t now;
  struct tm *tm_now;
  char   timebuff[64];
  unsigned long int jj = 0;

  unsigned int x = SCREEN_X + SCREEN_W / 2;
  unsigned int y = SCREEN_Y;
  unsigned int w = SCREEN_W / 2;
  unsigned int h = SCREEN_H / 2;
  unsigned int mid_x = x + w / 2;
  unsigned int mid_y = y + h / 2;

  while (1) {
    if (++jj >= TICK) {
      now = time(NULL);
      tm_now = localtime(&now);
      strftime(timebuff, sizeof(timebuff), "%H:%M:%S", tm_now);

      gotoxy(mid_x - strlen(timebuff) / 2, mid_y);
      printf("%s", timebuff);
      jj = 0;
    }
  }

  return;
}

void clock2()
{
  time_t now;
  struct tm *tm_now;
  char   timebuff[64];
  unsigned long int jj = 0;

  unsigned int x = SCREEN_X + SCREEN_W / 2;
  unsigned int y = SCREEN_Y + SCREEN_H / 2;
  unsigned int w = SCREEN_W / 2;
  unsigned int h = SCREEN_H / 2;
  unsigned int mid_x = x + w / 2;
  unsigned int mid_y = y + h / 2;

  while (1) {
    if (++jj >= TICK) {
      now = time(NULL);
      tm_now = localtime(&now);
      strftime(timebuff, sizeof(timebuff), "%H:%M:%S", tm_now);

      gotoxy(mid_x - strlen(timebuff) / 2, mid_y);
      printf("%s", timebuff);
      jj = 0;
    }
  }

  return;
}


void draw_screen(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
  unsigned int mid_x = x + w / 2;
  unsigned int mid_y = y + h / 2 - 1;
  int i = 0;

  clrscr();

  for (i=x; i<w; i++) {
    gotoxy(i, mid_y);
    putch('-'); 
  }

  for (i=y; i<h; i++) {
    gotoxy(mid_x, i);
    putch('|');
  }

  gotoxy(mid_x, mid_y);
  putch('+');

  gotoxy(0, 0);
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

  init_queue(&ready_queue);

  init_task(task1, 1, "Bouncing Ball", 
            SCREEN_X, SCREEN_Y, 
            SCREEN_W / 2, SCREEN_H / 2);

  init_task(clock1, 2, "Clock 1", 
            SCREEN_X + SCREEN_W / 2, SCREEN_Y, 
            SCREEN_W / 2, SCREEN_H / 2);

  init_task(characters, 3, "Printing Chars to Screen", 
            SCREEN_X, SCREEN_Y + SCREEN_H / 2, 
            SCREEN_W / 2, SCREEN_H / 2);

  init_task(clock2, 4, "Clock 2", 
            SCREEN_X + SCREEN_W / 2, SCREEN_Y + SCREEN_H / 2, 
            SCREEN_W / 2, SCREEN_H / 2);
  getch();

  draw_screen(SCREEN_X + 1, SCREEN_Y + 1, SCREEN_W, SCREEN_H);

  setvect(IRQ8, context_switch);

  do {
  } while(!kbhit());

  setvect(IRQ8, old_routine);
  return 0;
}
