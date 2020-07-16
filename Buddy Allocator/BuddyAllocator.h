#ifndef _BuddyAllocator_h_                   // include file only once
#define _BuddyAllocator_h_
#include <iostream>
#include <vector>
#include <cmath>
using namespace std;
typedef unsigned int uint;

/* declare types as you need */

class BlockHeader{
public:
	// think about what else should be included as member variables
	int block_size;  	// size of the block
	BlockHeader* next; 	// pointer to the next block
	bool isFree;  		//checks if the block is free to use or not

	BlockHeader() { block_size = 0; next = nullptr; isFree = true; }
};

class LinkedList{
	// this is a special linked list that is made out of BlockHeader s. 
public:
	BlockHeader* head;	// you need a head of the list
	int listSize;		//number of blocks in the list
public:
	LinkedList() { head = nullptr; listSize = 0; }

	void insert (BlockHeader* b){	// adds a block to the list
		if (head == nullptr) {		
			b->next = nullptr;
		}
		else {							
			b->next = head;		//always insert at head
		}
		head = b;
		b->isFree = true;
		listSize++;
	}

	void remove (BlockHeader* b){  // removes a block from the list
		BlockHeader* temp = head;
		if (temp == b) {
			if (temp->next == nullptr) {
				temp = nullptr;
			}
			else {
				temp = temp->next;
			}
			head = temp;
			listSize--;
			b->isFree = false;
		}
		else {
			while (temp->next != nullptr) {
				if (temp->next == b) {
					if (temp->next->next == nullptr) {
						temp->next = nullptr;
					}
					else {
						temp->next = temp->next->next;
					}
					listSize--;
					head = temp;
					b->isFree = false;
				}
				temp = temp->next;
			}
		}
	}	
};


class BuddyAllocator{
private:
	/* declare more member variables as necessary */
	vector<LinkedList> FreeList;	
	int basic_block_size;
	int total_memory_size;
	char* startMemory;		//points to the start address of entire memory

public:
	/* private function you are required to implement
	 this will allow you and us to do unit test */
	
	BlockHeader* getbuddy (BlockHeader * addr) { 
	// given a block address, this function returns the address of its buddy 
		int offset = (int)((char*)addr - startMemory)^addr->block_size;
		char* buddy = startMemory + offset;
		return (BlockHeader*)buddy;
	}

	bool arebuddies (BlockHeader* block1, BlockHeader* block2) {
	// checks whether the two blocks are buddies are not
	// note that two adjacent blocks are not buddies when they are different sizes
		BlockHeader* buddy1 = getbuddy(block1);
		BlockHeader* buddy2 = getbuddy(block2);
		if((char*)block1 == (char*)buddy2 && (char*)block2 == (char*)buddy1 && (buddy1->block_size == buddy2->block_size)) {
			return true;
		}
		return false;
	}

	BlockHeader* merge (BlockHeader* block1, BlockHeader* block2) {
	// this function merges the two blocks returns the beginning address of the merged block
	// note that either block1 can be to the left of block2, or the other way around
		int index = log2((block1->block_size)/(basic_block_size)); //find index of the buddies
		if ((char*) block2 > (char*)block1) {	//check for left buddy 
			FreeList[index].remove(block1);		//remove both buddies from freelist
			FreeList[index].remove(block2);
			block1->block_size *= 2;			//update header for left buddy
			return block1;	
		}
		else {
			FreeList[index].remove(block1);
			FreeList[index].remove(block2);
			block2->block_size *= 2;
			return block2;
		} 
	}

	BlockHeader* split (BlockHeader* block) {
	// splits the given block by putting a new header halfway through the block
	// also, the original header needs to be corrected
		
		char* temp = (char*)block + (block->block_size)/2; //start address + blocksize/2 (char can move by one byte)
		BlockHeader* newBlock = (BlockHeader*)temp;		   //type cast back to blockheader
		int index = log2((block->block_size)/(basic_block_size)); //index of bigger block
		FreeList[index].remove(block);					//remove the big block from the freelist
		int newIndex = log2(((block->block_size)/2)/(basic_block_size));	//index of the smaller blocksize
		newBlock->block_size = (block->block_size)/2; //update header information for the blocks
		block->block_size = newBlock->block_size;
		FreeList[newIndex].insert(newBlock); //insert both blocks in the freelist
		FreeList[newIndex].insert(block);
		
		return newBlock;
	}
 
public:
	BuddyAllocator (int _basic_block_size, int _total_memory_length);
	/* This initializes the memory allocator and makes a portion of 
	   ’_total_memory_length’ bytes available. The allocator uses a ’_basic_block_size’ as 
	   its minimal unit of allocation. The function returns the amount of 
	   memory made available to the allocator. 
	*/ 

	~BuddyAllocator(); 
	/* Destructor that returns any allocated memory back to the operating system. 
	   There should not be any memory leakage (i.e., memory staying allocated).
	*/ 

	void* alloc(int _length); 
	/* Allocate _length number of bytes of free memory and returns the 
		address of the allocated portion. Returns 0 when out of memory. */ 

	void free(void* _a); 
	/* Frees the section of physical memory previously allocated 
	   using alloc(). */ 
   
	void printlist ();
	/* Mainly used for debugging purposes and running short test cases */
	/* This function prints how many free blocks of each size belong to the allocator
	at that point. It also prints the total amount of free memory available just by summing
	up all these blocks.
	Aassuming basic block size = 128 bytes):

	[0] (128): 5
	[1] (256): 0
	[2] (512): 3
	[3] (1024): 0
	....
	....
	 which means that at this point, the allocator has 5 128 byte blocks, 3 512 byte blocks and so on.*/
};

#endif 
