#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include "Reqchannel.h"
#include "MQreqchannel.h"
#include "TCPreqchannel.h"
#include "SHMreqchannel.h"
#include <thread>
#include <time.h>
using namespace std;

RequestChannel *create_new_channel (RequestChannel* mainchan, string ival, int mb, string host, string port) {
	RequestChannel* newchan;
	if (ival == "t") {
		newchan = new TCPRequestChannel (host, port, RequestChannel::CLIENT_SIDE);
		return newchan;
	}
	else {
		char name[1024];
		MESSAGE_TYPE m = NEWCHANNEL_MSG;
		mainchan->cwrite (&m, sizeof(m));
		mainchan->cread(name, 1024);
		if (ival == "f") {
			newchan = new FIFORequestChannel (name, RequestChannel::CLIENT_SIDE);
		}
		else if (ival == "q") {
			newchan = new MQRequestChannel (name, RequestChannel::CLIENT_SIDE, mb);
		}
		else if (ival == "s") {
			newchan = new SHMRequestChannel (name, RequestChannel::CLIENT_SIDE, mb);	
		}
		return newchan;
	}
}
 

void patient_thread_function(int n, int pno, BoundedBuffer* request_buffer){
	datamsg d(pno, 0.0, 1);
	double resp = 0;
	for (int i=0; i<n; i++) {
		request_buffer->push((char*)&d, sizeof(datamsg));
		d.seconds += 0.004;
	}
}

void file_thread_function (string fname, BoundedBuffer* request_buffer, RequestChannel* chan, int mb) {
	//1. create the file
	string recvfname = "recv/" + fname;
	//make it as long as the original length
	char buf [1024];
	filemsg f (0,0);
	memcpy (buf, &f, sizeof(f));
	strcpy(buf + sizeof(f), fname.c_str());
	chan->cwrite(buf, sizeof(f) + fname.size() + 1);
	__int64_t filelength;
	chan->cread (&filelength, sizeof(filelength));
	FILE* fp = fopen(recvfname.c_str(), "w");
	fseek(fp, filelength, SEEK_SET);
	fclose(fp);

	//2. generate all the file message
	filemsg* fm = (filemsg*)buf;
	__int64_t remlen = filelength;

	while (remlen > 0) {
		fm->length = min (remlen, (__int64_t)mb);
		request_buffer->push (buf, sizeof(filemsg) + fname.size() + 1);
		fm->offset += fm->length;
		remlen -= fm->length;
	}	
}

void worker_thread_function(RequestChannel* chan, BoundedBuffer* request_buffer, HistogramCollection* hc, int mb){
	char buf [1024];
	double resp = 0;

	char recvbuf [mb];
	while(true) {
		request_buffer->pop(buf, 1024);
		MESSAGE_TYPE* m = (MESSAGE_TYPE*) buf;
		if (*m == DATA_MSG) {
			chan->cwrite(buf, sizeof(datamsg));
			chan->cread(&resp, sizeof(double));
			hc->update(((datamsg *)buf)->person, resp);
		}
		else if (*m == QUIT_MSG) {
			chan->cwrite (m, sizeof(MESSAGE_TYPE));
			delete chan;
			break;
		}
	 	else if (*m == FILE_MSG) {
	 		filemsg* fm = (filemsg*) buf;
	 		string fname = (char*)(fm +1);
	 		int sz = sizeof(filemsg) + fname.size() +1;
	 		chan->cwrite(buf, sz);
	 		chan->cread(recvbuf, mb);

	 		string recvfname = "recv/" + fname;

	 		FILE* fp = fopen(recvfname.c_str(), "r+");
	 		fseek(fp, fm->offset, SEEK_SET);
	 		fwrite(recvbuf, 1, fm->length, fp);
	 		fclose(fp);
		}
	}
}




int main(int argc, char *argv[])
{	//cout << "./client\t-n <# requests>\n\t        -b <bounded buffer size>\n\t        -w <# threads>\n\t        -m <buffer capacity>\n\t        -i <f|q|s|t>\n\t        -h <host name or IP address of server>\n\t        -r <port no of the server process>";    
	int n = 15000;    //default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 100;    //default number of worker threads
    int b = 500; 	// default capacity of the request buffer, you should change this default
	int m = MAX_MESSAGE; 	// default capacity of the message buffer
    srand(time_t(NULL));
    string fname = "10.csv";
    int opt = -1;
    string ival = "f";
    bool file = false;

    string hostname = "";
    string port = "";
    while ((opt = getopt(argc, argv, "m:n:b:w:p:f:i:h:r:")) != -1) {
    	switch(opt) {
    		case 'm':
    			m = atoi (optarg);
    			break;
    		case 'n':
    			n = atoi (optarg);
    			break;
    		case 'p':
    			p = atoi(optarg);
    			break;
    		case 'b':
    			b = atoi(optarg);
    			break;
    		case 'w':
    			w = atoi(optarg);
    			break;
    		case 'f':
    			file = true;
    			fname = optarg;
    			break;
    		case 'i':
    			ival = optarg;
    			break;
    		case 'h':
    			hostname = optarg;
    			break;
    		case 'r':
    			port = optarg;
    			break;
    	}
    }
    
  //   int pid = fork();
  //   if (pid == 0){
		// // modify this to pass along m
  //       execl ("server", "server", "-m", to_string(m).c_str(), "-i", ival.c_str(), (char *)NULL);
  //   }
    
	RequestChannel* chan;
	if (ival == "f") {
	  chan = new FIFORequestChannel("control", RequestChannel::CLIENT_SIDE);
	}
	else if (ival == "q") {
		chan = new MQRequestChannel ("control", RequestChannel::CLIENT_SIDE, m);
	}
	else if (ival == "t") {
		chan = new TCPRequestChannel (hostname, port, RequestChannel::CLIENT_SIDE);
	}
	else if (ival == "s") {
		chan = new SHMRequestChannel ("control", RequestChannel::CLIENT_SIDE, m);
	}
    BoundedBuffer request_buffer(b);
	HistogramCollection hc;
	
	//making histograms and adding to the histogram collection hc
	for (int i = 0; i < p; i++) {
		Histogram* h = new Histogram(10, -2.0, 2.0);
		hc.add(h);
	}

	//make w worker channels (sequentially in the main)
	RequestChannel* wchans [w];
	for (int i = 0; i < w; i++) {
		wchans[i] = create_new_channel(chan, ival, m, hostname, port);
	}

    struct timeval start, end;
    gettimeofday (&start, 0);

    /* Start all threads here */
    thread workers [w];
	for (int i=0; i<w; i++) {
			workers[i] = thread(worker_thread_function, wchans[i], &request_buffer, &hc, m);
	}
    if (!file) {
		thread patient[p];
		for (int i = 0; i < p; i++) {
			patient[i] = thread (patient_thread_function, n, i+1, &request_buffer);
		}

		/* Join all threads here */
		for (int i = 0; i < p; i++) {
			patient[i].join();
		}
		cout << "Patient Threads Finished\n";
	}
	else {
		thread filethread (file_thread_function, fname, &request_buffer, chan, m);
		/* Join all threads here */
		filethread.join();
		cout << "File threads Finished\n";
	}
	

	for (int i = 0; i < w; i++) {
		MESSAGE_TYPE q = QUIT_MSG;
		request_buffer.push((char*)&q, sizeof(q));
	}

	for (int i=0; i<w; i++) {
		workers[i].join();
	}
	cout << "Worker Threads Finished\n";
	// print the results
	if (!file)
		hc.print ();

    gettimeofday (&end, 0);
    //print time difference
 


	

    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    //cleaning the main channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite ((char *) &q, sizeof (MESSAGE_TYPE));
    cout << "All Done!!!" << endl;
    delete chan;
    
}
