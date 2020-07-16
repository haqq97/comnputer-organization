#include "Ackerman.h"
#include "BuddyAllocator.h"
#include <unistd.h>
#include <cstdlib>

void easytest(BuddyAllocator* ba){
  // be creative here
  // know what to expect after every allocation/deallocation cycle

  // here are a few examples
  ba->printlist();
  // allocating a byte
  char* mem = (char *) ba->alloc (1);
  //char* mem2 = (char *) ba->alloc (1);
  // char* mem3 = (char *) ba->alloc (1);
  // char* mem4 = (char *) ba->alloc (1);
  // char* mem5 = (char *) ba->alloc (1);
  // char* mem6 = (char *) ba->alloc (1);
  // char* mem7 = (char *) ba->alloc (1);
  // char* mem8 = (char *) ba->alloc (1);
  // char* mem9 = (char *) ba->alloc (1);
  // now print again, how should the list look now
  ba->printlist ();

  ba->free (mem); // give back the memory you just allocated
  //ba->free (mem2); // give back the memory you just allocated
  ba->printlist(); // shouldn't the list now look like as in the beginning

}

int main(int argc, char ** argv) {

  int basic_block_size = 128, memory_length = 1024 * 512 * 1024;
  int option;
  while ((option = getopt(argc, argv, "b:s:")) != -1) {
    switch(option) {
      case 'b':
        basic_block_size = atoi(optarg);
        break;
      case 's':
        memory_length = atoi(optarg);
        break;
      case '?':
      default:
        cout << "Invalid Command Line Arguments\n";
    }
  }

  // create memory manager
  BuddyAllocator * allocator = new BuddyAllocator(basic_block_size, memory_length);

 
  // the following won't print anything until you start using FreeList and replace the "new" with your own implementation
  //easytest (allocator);

  
  // stress-test the memory manager, do this only after you are done with small test cases
  Ackerman* am = new Ackerman ();
  
  am->test(allocator); // this is the full-fledged test. 
  
  // destroy memory manager
  delete allocator;
  delete am;
}
