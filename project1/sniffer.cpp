#include "worker.h"

struct worker{
    pid_t id;
    bool available;
};

std::vector<struct worker> workers;                             //instead of queue i used a vector to hold the workers


//signal handler function when parent process recives a SIGCHLD signal
void child_available(int signum){
    pid_t child;
    int status;

    child = waitpid(-1,&status, WSTOPPED | WNOHANG);                    //resive stoped worker id               
    for(int i = 0; i < workers.size(); i++){
        if (child == workers[i].id){                                    //find worker in vector

            workers[i].available = true;                                //and mark him available
            break;
        }
    }

}

bool endprog = false;                                                   

//signal function when parent process recives a SIGINT signal
void terminate(int sig){
    endprog = true;
}

bool endwork = false;

//signal function when a worker process recives a SIGINT signal
void worker_term(int sig){
    endwork = true;
}


int main(int argc, char *argv[]){

    char* path;
    if(argc == 3){
        if(strcmp(argv[1],"-p") == 0){
            path = argv[2];
        }
        else{
            perror("argument error\n");
            exit(1);
        }
    }
    else if(argc == 1){
        path = (char*)".";
    }
    else{
        perror("number of arguments\n");
        exit(1);
    }

    if ((mkdir("outfolder",0777) < 0) && (errno != EEXIST) ) {                      //create a folder for .out files
        perror("can't create folder for .out files");
    }

    if ((mkdir("fifos",0777) < 0) && (errno != EEXIST) ) {                          //create a folder for all the named pipes
        perror("can't create folder for .out files");
    }

    //signal handler for SIGCHLD
    struct sigaction sa;
    sa.sa_handler = &child_available;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaction(SIGCHLD, &sa , NULL);

    //signal handler for SIGINT
    struct sigaction sa2;
    sa2.sa_handler = &terminate;
    sa2.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa2 , NULL);

    int p[2]; //p[1] write p[0] read

    if (pipe(p) < 0){
        perror("error while reating a pipe\n");
        exit(1);
    }
    
    pid_t listener = fork();                                        //fork for listener

    if(listener == -1){
        perror("listener fork\n");
        exit(1);
    }
    else if(listener == 0){
        close(p[0]);                                                 //closing read chanel
        dup2(p[1],1);                                               //making the output of the execution of inotifywait to go in the pipe instead of stdout
        char *args[]={(char*)"inotifywait", path ,(char*)"-m", (char*)"-e", (char*)"create",(char*)"-e", (char*)"moved_to",NULL};       //monitor created files and moved_to files only
        execvp(args[0],args);
    }
    else{                                                              //manager code

        close(p[1]);                                                   //close writing chanel
        
        char buf[1]; 
        int loops = 0;
        int filenamestart = -1;
        std::string filename;
        while(1){
            if (endprog == true){                                    //when user give a SIGINT the program stop the infinate loop and ends
                break;
            }
            if (read(p[0], buf, 1)> 0 ){                             //read one character at a time of the output of inotifywait
                
                if(loops == 7 + strlen(path)){                      //7 characters after path name we need to check if how many characters until filename
                    if(buf[0] == 'E'){                              //if character is E that means the word is create
                        filenamestart = 9 + strlen(path);           //filename will start 2 characters after
                    }
                    else{                                           //word is moved_to
                        filenamestart = 11 + strlen(path);          //filename will start 4 characters after
                    }
                }
                if(filenamestart != -1){                                //founds the start of filename
                    if(loops >= filenamestart && buf[0] != '\n'){
                        filename.push_back((char)buf[0]);               
                    }
                }
                if (buf[0] == '\n'){                                        //change line means inotifywait command ended
                    pid_t id = -2;                                          //this is used to determined if manager need to create a worker
                    
                    for(int i = 0; i < workers.size(); i++){
                        if(workers[i].available == true){                           //if an available worker is found
                            //continue process
                            id = workers[i].id;                                     //change id so manager will not fork a worker
                            workers[i].available = false;
                            kill(id,SIGCONT);                                       //continue the process
                            break;
                        }
                    }

                    if(id == -2){                                                       //that means that no available worker is found
                        
                        id = fork();
                        struct worker newwork;                                          //create a worker to put in vector
                        newwork.available = false;
                        newwork.id = id;

                        workers.push_back(newwork);
                    }
                    if( id == -1){
                        perror("worker fork\n");
                        exit(1);
                    }
                    else if( id == 0){
                        //signal handler for worker SIGINT 
                        struct sigaction sa3;
                        sa3.sa_handler = &worker_term;
                        sa3.sa_flags = SA_RESTART;
                        sigaction(SIGINT, &sa3 , NULL);

                        while(1){
                            if(endwork == true){                                    //when manager give SIGINT to worker process will end
                                break;
                            }
                            worker();
                            raise(SIGSTOP);
                        }
                        exit(0);
                    }
                    else{

                        std::string fifoname = "fifos/myfifo";                          //each worker will have different named pipes with manager
                        fifoname.append(std::to_string(id));                            //named pipe name will be fifos/myfifo{process_id}

                        if ((mkfifo(fifoname.c_str(),0666) < 0) && (errno != EEXIST) ) {    //making the names pipe
                            perror("can't create fifo");
                        }
                        
                        int writefd;
                        std::string finalname = path;
                        finalname.append("/");
                        finalname.append(filename);

                        if ((writefd = open(fifoname.c_str(), O_WRONLY))  < 0)  {                  //open named pipe
                            perror("can't open write fifo");
                        }

                        int n = finalname.length();
                        if (write(writefd,finalname.c_str(), n) != n){                            //write filename to pipe
                            perror("client: filename write error");
                        }
                        close(writefd);

                        filename.erase();                            //erase filename so it wont overlap with next
                        loops = 0;                                   //reset loops
                        filenamestart = -1;                          //and file starting position
                    }
                }
                else{
                    loops++;
                }
            }
        }
        
        //outside infinate loop
        kill(listener,SIGINT);                                       //terminate listener

        for( int i = 0; i < workers.size(); i++){
            std::string fifo = "fifos/myfifo";
            fifo.append(std::to_string(workers[i].id));
            if ( unlink(fifo.c_str()) < 0) {                            //delete named pipes 
                perror("can't unlink \n");
            }
            if(workers[i].available == false){
                kill(workers[i].id,SIGCONT);                          //if a worker is stoped it wont recive SIGINT signal
            }
            kill(workers[i].id,SIGINT);                               //give SIGINT signal to workers
        }
        exit(0);
    }

}
