#include <iostream>
#include <cstring>
#include <sys/wait.h>	      
#include <sys/types.h>	     
#include <sys/socket.h>	     
#include <netinet/in.h>	     
#include <netdb.h>	         
#include <unistd.h>	         		
#include <stdlib.h>	         
#include <ctype.h>	         
#include <signal.h>          
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <list>
#include <queue>
#include <fcntl.h>

using namespace std;

//element to be added in queue
typedef struct element{
    int socket;
    string filename;
    bool last;
}element;

pthread_mutex_t mutex;
pthread_cond_t worker_cond;
pthread_cond_t com_cond;

queue<element> files;   //queue can be viewed by all threads
int queue_size = -1;    //initialised in main

void exit_funct(int sig){
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&worker_cond);
    pthread_cond_destroy(&com_cond);
    exit(EXIT_SUCCESS);
}

//returns a list with file names and relative paths
list<string> printdir(DIR *dr,string pathname, list<string> mylist){

	struct dirent *de;
	DIR *dr2;

	while ((de = readdir(dr)) != NULL){
        if(strcmp(de->d_name, "..") == 0){//without these two if directories will go back and forth infinate
            continue;
        }
        if(strcmp(de->d_name, ".") == 0){
            continue;
        }
        string newpath;
        newpath.append(pathname);
        newpath.append("/");                
        newpath.append(de->d_name);                     //each file or folder has the relative path too

        if((dr2 = opendir(newpath.c_str())) != NULL){       //if filename is folder
			mylist = printdir(dr2, newpath, mylist);        //call function with that folder as a parameter
        }
		else{
            string file = pathname;
            file.append("/");
            file.append(de->d_name);

            mylist.push_back(file);                         //add file with path to list
		}
	}
    return mylist;
}

void* worker(void* arg);                                    //worker thread function
void* communicator(void* newsock);                          //communicator function
void sigchld_handler (int sig); 

int main(int argc, char *argv[]) {

    int port = -1;
    int thread_pool_size = -1;
    int block_size = -1;

    if(argc != 9){
        perror("Wrong number of arguments given\n");
        exit(EXIT_FAILURE);
    }

    //get -p -s -q -b arguments in whatever order
    for (int i = 1; i < argc; i+=2){
        if(strcmp(argv[i], "-p") == 0){
            if(port != -1){
                perror("-p argument is given more than 1 times\n");
                exit(EXIT_FAILURE);
            }
            port = atoi(argv[i + 1]);
        }
        else if(strcmp(argv[i], "-s") == 0){
            if(thread_pool_size != -1){
                perror("-s argument is given more than 1 times\n");
                exit(EXIT_FAILURE);
            }
            thread_pool_size = atoi(argv[i + 1]);

        }
        else if(strcmp(argv[i], "-q") == 0){
            if(queue_size != -1){
                perror("-q argument is given more than 1 times\n");
                exit(EXIT_FAILURE);
            }
            queue_size = atoi(argv[i + 1]);

        }
        else if(strcmp(argv[i], "-b") == 0){
            if(block_size != -1){
                perror("-b argument is given more than 1 times\n");
                exit(EXIT_FAILURE);
            }
            block_size = atoi(argv[i + 1]);
        }
        else{
            perror("unknown command given\n");
            exit(EXIT_FAILURE);
        }
    }

    cout << "Server's parameters are: " << endl;
    cout << "port: " << port << endl;
    cout << "thread_pool_size: " << thread_pool_size << endl;
    cout << "queue_size: " << queue_size << endl;
    cout << "block_size: " << block_size << endl;

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&worker_cond, NULL);
    pthread_cond_init(&com_cond, NULL);

    int sock;
    int newsock;
    struct sockaddr_in server, client;
    socklen_t clientlen;
    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;

    //Create socket
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server.sin_family = AF_INET;       //Internet domain
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);      //The given port
    
    //Bind socket to address
    if (bind(sock, serverptr, sizeof(server)) < 0){
        perror("bind");
        exit(EXIT_FAILURE);
    }

    //Listen for connections
    if (listen(sock, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    //control-c
    signal(SIGCHLD, exit_funct);

    //create thread pool size workers
    pthread_t thread_pool[thread_pool_size];
    for(int i = 0; i < thread_pool_size; i++){
        if(pthread_create(&thread_pool[i], NULL, &worker , &block_size)){
            perror("thread creation\n");
            exit(EXIT_FAILURE);
        }
    }

    cout << "Server was successfully initialized..." << endl;
    cout << "Listening for connections to port " << port << endl;

    pthread_t pid;   
    while (1) { 
        clientlen = sizeof(client);
        //accept connection
    	if ((newsock = accept(sock, clientptr, &clientlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

    	cout << "Accepted connection\n" << endl;
        //pointer with socket value so each thread holds the right socket
        int* psock = new int;
        *psock = newsock;
        
        //communication thread creation
        if(pthread_create(&pid, NULL , &communicator, psock)){
            perror("thread creation\n");
            exit(EXIT_FAILURE);
        }

        if(pthread_detach(pid)){
            perror("thread detach\n");
            exit(EXIT_FAILURE);
        }

    }
}

void* communicator(void* sock) {

    int newsock = *(int*)sock;
    delete (int*)sock;              //dont need the pointer anymore

    string path;
    int len;

    //read directory length from client
    if (read(newsock, &len, sizeof(int)) < 0){
        perror("read directory size\n");
        exit(EXIT_FAILURE);
    }

    len = ntohl(len);                   //convert it 

    char buf[len];

    //read directory name from client
    if(read(newsock, buf, len) < 0){
        perror("read directory name\n");
        exit(EXIT_FAILURE);
    } 
    
    path.append(buf);   //path = directory name

    cout << "About to scan directory " << path << endl;

	DIR *dr = opendir(path.c_str());

	if (dr == NULL){
		printf("Could not open current directory" );
		return 0;
	}

    list<string> mylist;
    mylist = printdir(dr,path,mylist);              //return list with all folder files


    int size = htonl(mylist.size());

    //write to client the number of files included in folder
    if(write(newsock, &size, sizeof(int)) < 0){
        perror("write number of files\n");
        exit(EXIT_FAILURE);
    }

    list<string>::iterator it;

    //for each file
    for(it = mylist.begin() ; it != mylist.end(); it++){

        pthread_mutex_lock(&mutex);                 //only one thread push to queue at a time
        
        element newel;
        newel.filename = it->c_str();
        newel.socket = newsock;
        if(it->compare(mylist.back()) == 0){
            newel.last = true;                      //if true file is last one on folder
        }
        else{
            newel.last = false;
        }
        cout << "Adding file " << newel.filename << " to the queue..." << endl;
        files.push(newel);

        pthread_cond_signal(&worker_cond);                //unpause worker

        if(files.size() == queue_size){                     //pause if filrs in queue is max
            pthread_cond_wait(&com_cond, &mutex);
        }

        pthread_mutex_unlock(&mutex);

    }    

    pthread_exit(NULL);
}

void* worker(void* arg){
    
    int block_size = *(int*)arg;

    while(1){

        while(files.empty() == true){                       
            pthread_cond_wait(&worker_cond, &mutex);                //pause if queue is empty
        }

        element curel = files.front();                              //get file from queue
        string name;
        name = curel.filename;

        cout << "Received task: <" << name << ", " << curel.socket << ">" << endl;
        files.pop();

        pthread_cond_signal(&com_cond);                             //unpause communicator thread

        name.push_back('\0');                                    

        int filelen = htonl(name.length());
        
        //writing file name length to client
        if(write(curel.socket, &filelen , sizeof(int)) < 0){
            perror("writing file length\n");
            exit(EXIT_FAILURE);
        }

        //writing file name to client
        if(write(curel.socket, name.c_str() , name.length()) < 0){
            perror("writing file name\n");
            exit(EXIT_FAILURE);
        }

        //st will have the file size in bytes
        struct stat st;

        if (stat(name.c_str(), &st) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }

        int filesize = htonl(st.st_size);

        //write to client the file size in bytes
        if(write(curel.socket, &filesize , sizeof(int)) < 0){
            perror("writing file size in bytes\n");
            exit(EXIT_FAILURE);
        }

        //open file from server
        int out;
        if((out = open(name.c_str(), O_RDONLY|O_CREAT,0666)) < 0){ 
            perror("can't open or create .out file\n");
            exit(EXIT_FAILURE);       
        }

        int sum = 0;
        int n;
        char buf[block_size];

        cout << "About to read file " << name << endl;


        //while file has not been read completely
        while (sum < st.st_size){
            
            //read block size data from file
            n = read(out, buf, block_size);
            if( n == -1) {
                perror("couln not read file");
                exit(EXIT_FAILURE);

            }

            //if file ends with less bytes than block size add \0 to know the end
            if(n < block_size){
                buf[n] = '\0';
            }

            int readsize = htonl(n);
            //write to client how many bytes to read 
            if(write(curel.socket, &readsize , sizeof(int)) < 0){
                perror("writing bytes to be read\n");
                exit(EXIT_FAILURE);
            }

            //write file data to client
            if(write(curel.socket, buf, n) < 0){
                perror("writing file data\n");
                exit(EXIT_FAILURE);  
            }

            sum += n;

        }

        close(out);//close file from server

        if(curel.last == true){                     //if file was the last close the communication socket
            cout << "closing socket\n" << endl;
            close(curel.socket);
        }

    }
    pthread_exit(NULL);
}

