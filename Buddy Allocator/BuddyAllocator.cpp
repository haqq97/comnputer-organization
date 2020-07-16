#include "BuddyAllocator.h"
#include <iostream>
#include <cstdlib>
using namespace std;

BuddyAllocator::BuddyAllocator (int _basic_block_size, int _total_memory_length){
  basic_block_size = _basic_block_size;
  total_memory_size = _total_memory_length;
  startMemory = new char[_total_memory_length];
  BlockHeader* entireBlock = (BlockHeader*)startMemory; 
  entireBlock->block_size = _total_memory_length;
  FreeList.resize(ceil(log2(ceil((double)(_total_memory_length)/(double)(basic_block_size))))+1);
  FreeList[FreeList.size()-1].insert(entireBlock);
}

BuddyAllocator::~BuddyAllocator () {
	delete [] startMemory;
}

void* BuddyAllocator::alloc(int length) {
  /* This preliminary implementation simply hands the call over the 
     the C standard library! 
     Of course this needs to be replaced by your implementation.
  */
  	if ((length + sizeof(BlockHeader)) > total_memory_size){ //check for valid requests
  		cout << "\nError: Not Enough Memory to allocate " << length  << " bytes. \n";
  		return nullptr;
  	}
  	int index = ceil(log2(ceil((double)(length + sizeof(BlockHeader))/(double)(basic_block_size))));
  	if (index < 0) { index = 0; }	//find the correct index for checking the list
  	int targetIndex = index;
  	BlockHeader* requestedBlock = nullptr;
  	while (index < FreeList.size() && FreeList[index].head == nullptr) {
  		index++;	//keep looping through list until a block is found
  	}
  	if (index == FreeList.size() && FreeList[FreeList.size()-1].head == nullptr) {
  		cout << "\nError: Out of Memory \n";	//if no block is found in the list
  		return nullptr;
  	}	
	while (index > targetIndex) {	//if a block is found
		split(FreeList[index].head);
		index--;
	}
	char* temp = (char*)(FreeList[targetIndex].head) + sizeof(BlockHeader);
	requestedBlock = (BlockHeader*)temp;
	FreeList[targetIndex].remove(FreeList[targetIndex].head);

  	return requestedBlock;
}

void BuddyAllocator::free(void* a) {
  /* Same here! */
  	char* temp = (char*)a - sizeof(BlockHeader);  //convert to char and make the pointer point to start of the block
  	BlockHeader* freeBlock = (BlockHeader*)temp;  //convert to blockheader type
  	freeBlock->isFree = true;					  //update the blockheader
  	int index = log2((freeBlock->block_size)/(basic_block_size)); //find the index at which it needs to be inserted in the list
  	bool inserted = false;
  	while (index < FreeList.size()+1 && inserted == false) { //keep looping until the block has been merged or inserted
  		if (FreeList[index].head == nullptr) { 
  			FreeList[index].insert(freeBlock);
  			inserted = true;
  		}
  		else {
  			BlockHeader* buddy = getbuddy(freeBlock);
  			if (buddy->isFree == true && arebuddies(freeBlock, buddy) == true) { //if the block has a free buddy, merge the blocks
  				freeBlock = merge(freeBlock, buddy);
  			}  			
  		
  		}
  		index++;
  	}
}

void BuddyAllocator::printlist (){
  cout << "\nPrinting the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
  int64_t total_free_memory = 0;
  for (int i=0; i<FreeList.size(); i++){
    int blocksize = ((1<<i) * basic_block_size); // all blocks at this level are this size
    cout << "[" << i <<"] (" << blocksize << ") : ";  // block size at index should always be 2^i * bbs
    int count = 0;
    BlockHeader* b = FreeList [i].head;
    // go through the list from head to tail and count
    while (b){
      total_free_memory += blocksize;
      count ++;
      // block size at index should always be 2^i * bbs
      // checking to make sure that the block is not out of place
      if (b->block_size != blocksize){
        cerr << "ERROR:: Block is in a wrong list" << endl;
        exit (-1);
      }
      b = b->next;
    }
    cout << count << endl;
    cout << "Amount of available free memory: " << total_free_memory << " bytes\n";  
  }
}

