/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h"
#include "FIFOreqchannel.h"
#include <cstdlib>
#include <stdio.h>
#include <sys/wait.h>

using namespace std;


int main(int argc, char *argv[]){
    int numRequests = 100;    // default number of requests per "patient"
	int numPatients = 15;		// number of patients
    srand(time_t(NULL));

    //booleans for parsing command line arguments
    bool p = false, t = false, e = false, f = false, c = false, m = false;
    //setting default values
    int person = 0;     
    double time = 0.000;
    char *file;
    int ecg = 1;
    int opt;
    __int64_t maxBufferSize = MAX_MESSAGE;  //maximum allowed size for data transfer between server and client
    while ((opt = getopt(argc, argv, "m:p:t:e:f:c")) != -1) {
        switch (opt) {
            case 'p':
                person = atoi(optarg);
                p = true;
                if (person < 1 || person > 15) {
                    cout << "Invalid Arguments\n";
                    return 0;
                }
                break;
            case 't':
                t = true;
                time = atoi(optarg);
                if (time < 0 || time > 59.996) {
                    cout << "Invalid Arguments\n";
                    return 0;                    
                }
                break;
            case 'e':
                e = true;
                ecg = atoi(optarg);
                if (ecg < 1 || ecg > 2) {
                    cout << "Invalid Arguments\n";
                    return 0;
                }
                break;
            case 'f':
                f = true;
                file = optarg;
                break;
            case 'c':
                c = true;
                break;
            case 'm':
                m = true;
                maxBufferSize = atoi(optarg);
                break;
        }
    }
    int pid = fork(); //creates child process
    char* mVal;
    if (pid == 0) { //child process:
        if (m == false) { 
            char* arg[] = {"./server", nullptr};
            execvp(arg[0], arg); //executes server program file
        }
        else {
            string var = to_string(maxBufferSize);
            mVal = const_cast<char*>(var.c_str());
            char* arg[] = {"./server", "-m", mVal, nullptr}; 
            execvp(arg[0], arg);  
        }
    }
    //Parent Process:
    //channel to communicate with server
    FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

    //request containing specific patient, time, and ecg values
    if (t == true && e == true && p == true) { 
        struct timeval start, end;
        gettimeofday(&start, NULL); //start timer
        ios_base::sync_with_stdio(false); 
        datamsg d(person, time, ecg);     //setting values for specific data message
        chan.cwrite(&d, sizeof(datamsg));  //request sent to server with data type and size 
        double data;
        chan.cread((char*)&data, sizeof(double));   //receiving data from server into double
        cout << "Requested Data: " << data << endl;
        gettimeofday(&end, NULL); 
        double time_taken; 
        time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
        cout << "Time taken: " << fixed << time_taken << setprecision(6); 
        cout << " seconds" << endl;
    }
    //request for all data for a patient
    else if (p == true && t == false && e == false && f == false) {
        struct timeval start, end;
        gettimeofday(&start, NULL); //start timer
        ios_base::sync_with_stdio(false); 
        ofstream outfile;
        string filename = "received/x" + to_string(person) + ".csv";
        outfile.open(filename);
        if (!outfile.is_open()) {
            cout << "Error opening file\n";
            return 0;
        }
        double data = 0; string dataStr; string timeStr;
        int value = 0;
        //get both ecg1 and ecg2 values by writing to the server with datamsg
        //output to file in correct format
        while (value < 15000) { //number of data points per person is 15K
            datamsg dm1(person, time, ecg); 
            chan.cwrite(&dm1, sizeof(datamsg));
            chan.cread((char*)&data, sizeof(double));
            dataStr = to_string (data);
            dataStr.erase ( dataStr.find_last_not_of('0') + 1, std::string::npos ); //removes trailing zeros
            dataStr.erase ( dataStr.find_last_not_of('.') + 1, std::string::npos ); //removes trailing decimal points
            timeStr = to_string(time);
            timeStr.erase ( timeStr.find_last_not_of('0') + 1, std::string::npos );
            timeStr.erase ( timeStr.find_last_not_of('.') + 1, std::string::npos );
            outfile << timeStr << "," << dataStr << ",";
            ecg++; //for second ecg value
            datamsg dm2(person, time, ecg);
            chan.cwrite(&dm2, sizeof(datamsg));
            chan.cread((char*)&data, sizeof(double));
            dataStr = to_string (data);
            dataStr.erase ( dataStr.find_last_not_of('0') + 1, std::string::npos );
            dataStr.erase ( dataStr.find_last_not_of('.') + 1, std::string::npos );
            outfile << dataStr << endl;
            time += 0.004; ecg--; //increment time by 4ms and get ecg1 value first
            value++;
        }
        outfile.close();
        gettimeofday(&end, NULL); 
        double time_taken; 
        time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
        cout << "Time taken: " << fixed << time_taken << setprecision(6); 
        cout << " seconds" << endl;
    }
    //for file message
    else if (f == true) { 
    //create buffer with size of filemsg, file length and null character
    //copy the contents into the buffer
    //write to buffer with this package and recieve the number of bytes required to read the file
    //keep writing and receiving slices of the file until the file is completely transferred
        struct timeval start, end;
        gettimeofday(&start, NULL); //start timer
        ios_base::sync_with_stdio(false); 
        __int64_t numBytes = 0;
        char* firstBuffer = new char[sizeof(filemsg) + strlen(file) + 1]; 
        filemsg fm(0, 0); //will give us the number of bytes in the file 
        filemsg *filePtr = &fm;
        firstBuffer = (char*)filePtr;   //file type kept in the beginning of buffer
        strcpy(firstBuffer + sizeof(filemsg), file);    //filename is kept next to the file type
        strcpy(firstBuffer + sizeof(filemsg) + strlen(file), "\0"); //packet is ended with null character
        chan.cwrite(firstBuffer, sizeof(firstBuffer) + sizeof(filemsg) + strlen(file) + 1); 
        chan.cread(&numBytes, maxBufferSize);
        __int64_t remainingBytes = numBytes;
        __int64_t requestedBytes = 0;
        string filename = "received/";  filename+= file; 
        ofstream outfile;
        outfile.open(filename);
        char* receivedBuffer = new char[maxBufferSize];
        for (int i = 0; i < ceil((numBytes*1.0)/maxBufferSize); i++) {
            if (remainingBytes > maxBufferSize) {  //taking maximum number of bytes possible
                requestedBytes = maxBufferSize;
                remainingBytes -= maxBufferSize;
            }
            else {  //making sure to request exact number of bytes in the file
                requestedBytes = remainingBytes;
            }
            filemsg slice(i * maxBufferSize, requestedBytes); 
            filePtr = &slice;
            firstBuffer = (char*)filePtr;
            strcpy(firstBuffer + sizeof(filemsg), file); 
            strcpy(firstBuffer + sizeof(filemsg) + strlen(file), "\0");
            chan.cwrite(firstBuffer, sizeof(firstBuffer) + sizeof(filemsg) + strlen(file) + 1);
            chan.cread(receivedBuffer, maxBufferSize);
            outfile.write(receivedBuffer, requestedBytes);
        }
        outfile.close();
        gettimeofday(&end, NULL); 
        double time_taken; 
        time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
        time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
        cout << "Time taken: " << fixed << time_taken << setprecision(6); 
        cout << " seconds" << endl;
    }
    else if (c == true) {
        MESSAGE_TYPE newChannel = NEWCHANNEL_MSG;
        chan.cwrite(&newChannel, sizeof(MESSAGE_TYPE)); 
        char* buffer = new char[maxBufferSize];
        chan.cread(buffer, maxBufferSize);
        FIFORequestChannel chan2 (buffer, FIFORequestChannel::CLIENT_SIDE);
        //using the new channel to request some data messages
        datamsg d1(1, 1, 1);          
        datamsg d2(2, 1, 1);          
        datamsg d3(3, 1, 1); 
        double data;         
        chan2.cwrite((char*)&d1, sizeof(datamsg));
        chan2.cread(&data, sizeof(double));   
        cout << "Patient 1, time 1, ecg 1: " << data << endl;
        chan2.cwrite((char*)&d2, sizeof(datamsg));
        chan2.cread(&data, sizeof(double));
        cout << "Patient 2, time 1, ecg 1: " << data << endl;
        chan2.cwrite((char*)&d3, sizeof(datamsg));
        chan2.cread(&data, sizeof(double));
        cout << "Patient 3, time 1, ecg 1: " << data << endl;
        //closing the new channel
        MESSAGE_TYPE m1 = QUIT_MSG;
        chan2.cwrite (&m1, sizeof (MESSAGE_TYPE));
    }

    // closing the channel    
    MESSAGE_TYPE m2 = QUIT_MSG;
    chan.cwrite (&m2, sizeof (MESSAGE_TYPE));
    wait(NULL);
    return 0;
}
