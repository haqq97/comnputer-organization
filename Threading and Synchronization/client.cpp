#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <thread>
#include <time.h>
using namespace std;


void patient_thread_function(int n, int pno , BoundedBuffer* request_buffer){
    datamsg d(pno, 0.0, 1);
    double resp = 0;
    for (int i=0; i<n; i++) {
    	request_buffer->push ((char*)&d, sizeof(datamsg));
    	d.seconds += 0.004;
    }
}

void worker_thread_function(FIFORequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc ){
    char buf[1024];
    double resp = 0;
    while(true) {
    	request_buffer->pop(buf, 1024);
    	MESSAGE_TYPE* m = (MESSAGE_TYPE*)buf;
    	if (*m == DATA_MSG) {
    		chan->cwrite(buf, sizeof(datamsg));
    		chan->cread(&resp, sizeof(double));
    		hc->update(((datamsg*)buf)->person, resp);
    	}
    	else if (*m == FILE_MSG) {

    	}
    	else if (*m == QUIT_MSG) {
    		chan->cwrite(m, sizeof(MESSAGE_TYPE));
    		break;
    	}
    }
}

FIFORequestChannel *create_new_channel (FIFORequestChannel* mainchan) {
	char name[1024];
	MESSAGE_TYPE m = NEWCHANNEL_MSG;
	mainchan->cwrite (&m, sizeof(m));
	mainchan->cread(name, 1024);
	FIFORequestChannel* newchan = new FIFORequestChannel (name, FIFORequestChannel::CLIENT_SIDE);
	return newchan;
}
 

int main(int argc, char *argv[])
{
    int n = 1000;    //default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 100;    //default number of worker threads
    int b = 20; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    srand(time_t(NULL));
    string fname = 
    
    int pid = fork();
    if (pid == 0){
		// modify this to pass along m
        execl ("server", "server", (char *)NULL);
    }
    
	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
	
	//making histograms and adding to the histogram collection hc
	for (int i = 0; i < p; i++) {
		Histogram* h = new Histogram(10, -2.0, 2.0);
		hc.add(h);
	}

	//make worker channels
	FIFORequestChannel* wchans [w];
	for (int i = 0; i < w; i++) {
		wchans[i] = create_new_channel(chan);
	}


	


    struct timeval start, end;
    gettimeofday (&start, 0);
    /* Start all threads here */

    thread patient [p];
	for (int i = 0; i < p; i++) {
		patient[i] = thread(patient_thread_function, n, i+1, &request_buffer);
	}
	
	thread workers [w];
	for (int i=0; i<w; i++) {
		workers[i] = thread(worker_thread_function, wchans[i], &request_buffer, &hc);
	}

	/* Join all threads here */
	for (int i = 0; i < p; i++) {
		patient[i].join();
	}


    gettimeofday (&end, 0);
    //print time difference
 

    // print the results
	hc.print ();



    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    //cleaning the main channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
