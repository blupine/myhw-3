#include <sys/types.h>
#include <sys/resource.h>
#include <limits.h>
#include <stdio.h>

#include "run.h"
#include "util.h"

#define NULL (void*)0

void *base = 0;
p_meta premeta = 0;

// RETURN 0 IF NOT EXIST PROPER SPACE

p_meta find_meta(p_meta *last, size_t size) {
    p_meta index = base;
    p_meta result = base;

    switch(fit_flag){
        case FIRST_FIT:
        {
            while(index){
                if(index->free && (index->size) >= size){
                    result = index;
                    break;
                }
                premeta = index;
                index = (index->next);
            }
        }
        break;

      case BEST_FIT:
         {
            p_meta best = 0;   

            while(index != 0){
               if(index->free != 0)
                  if((index->size) >=size){
                     if(best == 0)
                        best = index;
                     else if(best->size > index->size)
                        best = index;
                  }
               index = (index->next);
            }
            result = best;
         }
         break;

      case WORST_FIT:
         {
            p_meta worst = 0;

            while(index != 0){
               if(index->free !=0)
                  if((index->size) >= size){
                     if(worst == 0)
                        worst = index;
                     else if(worst->size < index->size)
                        worst = index;
                  }
               index = (index->next);
            }
            result = worst;
         }

        break;
    }
    return result;
}

void *m_malloc(size_t size) {

    p_meta meta = find_meta(NULL, size);

    if(size % 4 != 0)
        size += (4-size%4);

    if(base == meta){
        if(base == 0){
            meta = sbrk(size + META_SIZE);
            base = meta;
            premeta = base;
            meta->next = 0;
            meta->prev = 0;
            meta->free = 0;
            meta->size = size;
            return meta->data;
        }
        meta = sbrk(size + META_SIZE);
        if(meta == 1){
            return NULL;
        } else{
            meta->size = size;
            meta->prev = premeta;
            meta->next = 0;
            meta->free = 0;
            premeta->next = meta;
            return meta->data;
       }
    } else{
        m_realloc(meta->data, size);
        return meta->data;
    }
}

void m_free(void *ptr) {
    p_meta cur = ptr - META_SIZE;

    ptr = NULL;
    cur->free = 1;

    if(cur->prev && (cur->prev)->free){      
        (cur->prev)->next = cur->next;
        (cur->prev)->size += cur->size + META_SIZE;
        cur = cur->prev;
    }
    // why this sentence doesn't work ? (if(cur->next && (cur->next)->free))
	if(cur->next){ 
		if((cur->next)->free){
			cur->size += (cur->next)->size + META_SIZE;
			cur->next = (cur->next)->next;
            (cur->next)->prev = cur;
        }
	} 
	else if(cur->prev){
        (cur->prev)->next = NULL;
	}
}

void* m_realloc(void* ptr, size_t size){   // free and merge hole
    p_meta cur = ptr - META_SIZE;   
    
    if(size % 4 != 0)    // resize
        size += (4-size%4);
	
	cur->free = 0;    // current block is free now

    if(cur->next && (cur->next)->free) {// if next exists and free, merge cur and next.
        cur->size += (cur->next)->size + META_SIZE;
        cur->next = (cur->next)->next;
        (cur->next)->prev = cur;
    }
    if(cur->prev && (cur->prev)->free){      
        (cur->prev)->next = cur->next;
        (cur->prev)->size += cur->size + META_SIZE;
        cur = cur->prev;
    }


    if(cur->size >= size){
        if(cur->size < size+META_SIZE)  // internal fragmentation
            return cur->data;
        else{ 
            p_meta insert = cur + size + META_SIZE;  // temporal block to insert
            insert->next = cur->next;                 
            insert->prev = cur;
            cur->next = insert;            
            insert->size = cur->size - size - META_SIZE;
            insert->free = 1;
            cur->size = size;                     
        }    
    } else {  // if (cur->size < size) // free block
		cur->free = 1;  
		p_meta temp = m_malloc(size);
        memcpy(temp, ptr, size);		
		return temp;
	}
    return ptr;
}

