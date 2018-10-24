#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>


/* Queue Structures */

typedef struct queue_node_s {
  struct queue_node_s *next;
  struct queue_node_s *prev;
  char c;
} queue_node_t;

typedef struct {
  struct queue_node_s *front;
  struct queue_node_s *back;
  pthread_mutex_t lock;
} queue_t;


/* Thread Function Prototypes */
void *producer_routine(void *arg);
void *consumer_routine(void *arg);


/* Global Data */
long g_num_prod; /* number of producer threads */
pthread_mutex_t g_num_prod_lock;


/* Main - entry point */
int main(int argc, char **argv) {
  queue_t queue;
  pthread_t producer_thread, consumer_thread;
  void *thread_return = NULL;
  int result = 0;

  /* Initialization */

  printf("Main thread started with thread id %lu\n", pthread_self());

  memset(&queue, 0, sizeof(queue));
  pthread_mutex_init(&queue.lock, NULL);

  // BUG1: Mutex g_num_prod_lock wasn't intialised
  // Hence intialising it using pthread_mutex_init()
  pthread_mutex_init(&g_num_prod_lock, NULL);

  g_num_prod = 1; /* there will be 1 producer thread */

  /* Create producer and consumer threads */

  result = pthread_create(&producer_thread, NULL, producer_routine, &queue);
  if (0 != result) {
    fprintf(stderr, "Failed to create producer thread: %s\n", strerror(result));
    exit(1);
  }

  printf("Producer thread started with thread id %lu\n", producer_thread);

  result = pthread_detach(producer_thread);
  if (0 != result)
    fprintf(stderr, "Failed to detach producer thread: %s\n", strerror(result));

  result = pthread_create(&consumer_thread, NULL, consumer_routine, &queue);
  if (0 != result) {
    fprintf(stderr, "Failed to create consumer thread: %s\n", strerror(result));
    exit(1);
  }

  /* Join threads, handle return values where appropriate */

  // BUG2: A detached thread cannot be joined again. Producer thread was
  // detached in the above lines. Commenting out the unnecessary lines.

  // result = pthread_join(producer_thread, NULL);
  // if (0 != result) {
  //   fprintf(stderr, "Failed to join producer thread: %s\n", strerror(result));
  //   pthread_exit(NULL);
  // }

  result = pthread_join(consumer_thread, &thread_return);
  if (0 != result) {
    fprintf(stderr, "Failed to join consumer thread: %s\n", strerror(result));
    pthread_exit(NULL);
  }
  printf("\nPrinted %lu characters.\n", *(long*)thread_return);
  free(thread_return);

  pthread_mutex_destroy(&queue.lock);
  pthread_mutex_destroy(&g_num_prod_lock);
  return 0;
}


/* Function Definitions */

/* producer_routine - thread that adds the letters 'a'-'z' to the queue */
void *producer_routine(void *arg) {
  queue_t *queue_p = arg;
  queue_node_t *new_node_p = NULL;
  pthread_t consumer_thread;
  int result = 0;
  char c;

  result = pthread_create(&consumer_thread, NULL, consumer_routine, queue_p);
  if (0 != result) {
    fprintf(stderr, "Failed to create consumer thread: %s\n", strerror(result));
    exit(1);
  }

  result = pthread_detach(consumer_thread);
  if (0 != result)
    fprintf(stderr, "Failed to detach consumer thread: %s\n", strerror(result));

  for (c = 'a'; c <= 'z'; ++c) {

    /* Create a new node with the prev letter */
    new_node_p = malloc(sizeof(queue_node_t));
    new_node_p->c = c;
    new_node_p->next = NULL;

    /* Add the node to the queue */
    pthread_mutex_lock(&queue_p->lock);
    if (queue_p->back == NULL) {
      assert(queue_p->front == NULL);
      new_node_p->prev = NULL;

      queue_p->front = new_node_p;
      queue_p->back = new_node_p;
    }
    else {
      assert(queue_p->front != NULL);
      new_node_p->prev = queue_p->back;
      queue_p->back->next = new_node_p;
      queue_p->back = new_node_p;
    }
    pthread_mutex_unlock(&queue_p->lock);

    sched_yield();
  }

  /* Decrement the number of producer threads running, then return */

  // BUG3: g_num_prod is a global variable and needs to be inside a mutex
  // lock-unlock block
  pthread_mutex_lock(&g_num_prod_lock);
  --g_num_prod;
  pthread_mutex_unlock(&g_num_prod_lock);
  return (void*) 0;
}


/* consumer_routine - thread that prints characters off the queue */
void *consumer_routine(void *arg) {
  queue_t *queue_p = arg;
  queue_node_t *prev_node_p = NULL;
  // BUG4: When consumer thread exits, it release all it's resources and variable
  // count is rendered invalid. This causes problems when main thread tries to free
  // thread_return. Hence count is declared a pointer and free() can function properly
  // as the object pointed by count will be on heap.
  long* count = malloc(sizeof(long)); /* number of nodes this thread printed */

  printf("Consumer thread started with thread id %lu\n", pthread_self());

  /* terminate the loop only when there are no more items in the queue
   * AND the producer threads are all done */

  pthread_mutex_lock(&queue_p->lock);
  pthread_mutex_lock(&g_num_prod_lock);
  while(queue_p->front != NULL || g_num_prod > 0) {
    pthread_mutex_unlock(&g_num_prod_lock);

    if (queue_p->front != NULL) {

      /* Remove the prev item from the queue */
      prev_node_p = queue_p->front;

      if (queue_p->front->next == NULL)
        queue_p->back = NULL;
      else
        queue_p->front->next->prev = NULL;

      queue_p->front = queue_p->front->next;
      pthread_mutex_unlock(&queue_p->lock);

      /* Print the character, and increment the character count */
      printf("%c", prev_node_p->c);
      free(prev_node_p);
      // BUG4: Incrementing count pointer
      ++(*count);
    }
    else { /* Queue is empty, so let some other thread run */
      pthread_mutex_unlock(&queue_p->lock);
      sched_yield();
    }
    // BUG5: After the first iteration of the while loop, the locks are removed from
    // both mutexes. Hence adding the locks back again after every iteration of while.
    pthread_mutex_lock(&queue_p->lock);
    pthread_mutex_lock(&g_num_prod_lock);
  }
  pthread_mutex_unlock(&g_num_prod_lock);
  pthread_mutex_unlock(&queue_p->lock);

  // BUG4: Returning count pointer
  return count;
}
