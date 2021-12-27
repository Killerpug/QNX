#ifndef _QUEUE_H_
#define _QUEUE_H_

struct queueNode
{
	struct queueNode* next_ptr;
	int data;
};

typedef struct queueNode queueNode_t;

int* get_data_and_remove_from_queue();
void write_to_hardware (int* data);
void add_to_queue(int buf[]);
int* get_data_and_remove_from_queue();
int add_element_to_queue(int data);
int print_queue();

#endif //_QUEUE_H_
