#include "SortedList.h"
#include <stdlib.h>
#include <stdio.h>
#include <sched.h>
#include <pthread.h>



void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
	struct SortedListElement * previous = list;
	struct SortedListElement * current = list->next;
	
	while(1){
		
		//if last node 
		if(current == NULL){
			previous->next = element;
			//yield in the middle of inserting
			if (opt_yield&INSERT_YIELD)
                        sched_yield();
			element->prev = previous;
			element->next = NULL;
			break;
		}
		
		//if other node 
		else if(*(element->key) <= *(current->key)){
			previous->next = element;
			element->prev = previous;
			
			//yield in middle of inserting
			if (opt_yield&INSERT_YIELD)
						
                        sched_yield();
			element->next = current;
			current->prev = element;
			break;
		}
		
		previous = current;
		current = current->next;
	}
	return;
}


int SortedList_delete( SortedListElement_t *element){
	
	//if last node
	if(element->next == NULL){
		if (element->prev->next!=element){
			return 1;
		}
		else{
			//yield after checking node, giving chance for other threads to modify
			if (opt_yield&DELETE_YIELD)
				
                        sched_yield();
			element->prev->next = NULL;
			return 0;
		}
		}
	else{
		if(element->prev->next!= element || element->next->prev!= element){
					return 1;
				}
		else{
			element->prev->next = element->next;
			//yield in the middle of deleting a node 
			if (opt_yield&DELETE_YIELD)
                        sched_yield();
			element->next->prev = element->prev;
			return 0;
		}
	}
}



SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
	
	struct SortedListElement * current = list->next;
	
	while(1){
		if(current == NULL){
			return NULL;
		}
		
		else if(*(current->key) == *(key)){
			//yield after finding element, giving chance for element to be modified
			if (opt_yield&LOOKUP_YIELD){
					
                        sched_yield();
			}
			
			return current;
		}
		current = current->next;
	}
	return NULL;
}



int SortedList_length(SortedList_t *list){
	struct SortedListElement * current = list->next;
	int counter =0;
	
	while(1){
		if(current == NULL){
			return counter;
		}
		
		else{
			if(current->next == NULL){
				if(current->prev->next!=current)
					return -1;
			}
			else{
				if (current->prev->next != current || current->next->prev!=current){
					return -1;
				}
			}
		}
		
		
		current = current->next;
		//yield after moving to next item, giving chance for list to change 
		if (opt_yield&LOOKUP_YIELD)
                        sched_yield();
		counter++;
	}
	return counter;
}


	