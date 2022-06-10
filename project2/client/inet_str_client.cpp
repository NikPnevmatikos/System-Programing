#include <iostream>
#include <sys/types.h>	     
#include <sys/socket.h>	     
#include <netinet/in.h>	     
#include <unistd.h>          
#include <netdb.h>	         
#include <stdlib.h>	         
#include <string.h>	        
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;


int main(int argc, char *argv[]) {

    string server_ip;
    int server_port = -1;
    string directory;

    if(argc != 7){
        perror("Wrong number of arguments given\n");
        exit(EXIT_FAILURE);
    }

    //arguments must be accepted at every order
    for (int i = 1; i < argc; i+=2){
        if(strcmp(argv[i], "-i") == 0){
            if(server_ip.empty() == false){
                perror("-i argument is given more than 1 times\n");
                exit(1);
            }
            server_ip = argv[i + 1];
        }
        else if(strcmp(argv[i], "-p") == 0){
            if(server_port != -1){
                perror("-p argument is given more than 1 times\n");
                exit(1);
            }
            server_port = atoi(argv[i + 1]);

        }
        else if(strcmp(argv[i], "-d") == 0){
            if(directory.empty() == false){
                perror("-d argument is given more than 1 times\n");
                exit(1);
            }
            directory = argv[i + 1];

        }
        else{
            perror("unknown command given\n");
            exit(1);
        }
    }

    cout << "Client's parameters are: " << endl;
    cout << "serverIP: " << server_ip << endl;
    cout << "port: " << server_port << endl;
    cout << "directory: " << directory << endl;

    int sock;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;

    //Create socket
    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
    	perror("socket");
        exit(EXIT_FAILURE);
    }
	//Find server address
    if ((rem = gethostbyname(server_ip.c_str())) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }

    server.sin_family = AF_INET;       //Internet domain

    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(server_port);         //Server port

    
    //Initiate connection
    if (connect(sock, serverptr, sizeof(server)) < 0){
	   perror("connect");
       exit(EXIT_FAILURE);
    }

    cout << "Connecting to " << server_ip << " port " << server_port << endl;

    directory.push_back('\0');
    int len = directory.length();
    len = htonl(len);
    //write directory size server
    if (write(sock, &len , sizeof(int)) < 0){
        perror("write directory length\n");
        exit(EXIT_FAILURE);
    }

    //write directory name to server
    if (write(sock, directory.c_str() , directory.length()) < 0){
        perror("write directory name\n");
        exit(EXIT_FAILURE);
    }

    int size;
    //read number of files to be duplicated to client
    if(read(sock, &size, sizeof(int)) < 0){
        perror("reading number of files\n");
        exit(EXIT_FAILURE);
    }

    size = ntohl(size);

    //for every file to be created
    for(int i= 0; i < size; i++){

        int filelen;
        //read file name lenght
        if(read(sock, &filelen, sizeof(int)) < 0){
            perror("reading size of file\n");
            exit(EXIT_FAILURE);  
        }
        filelen = ntohl(filelen);

        char buf[filelen];

        //read file name
        if(read(sock, buf, filelen) < 0){
            perror("reading file name\n");
            exit(EXIT_FAILURE);  
        } 

        string name = buf;

        int pos = name.find("/");
        string file = name;
        string start = name.substr(0,pos); //charecters until first / is the name of the directory asked by client
        name = name.substr(pos+1);

        //create directory asked
        if((mkdir(start.c_str(),0777) < 0) && (errno != EEXIST)){
            perror("can't create folder\n");
            exit(EXIT_FAILURE);
        }

        //between / is folder names and after final / is file name
        while((pos = name.find("/")) != string::npos){
            start = start.append("/");
            start = start.append(name.substr(0,pos));

            //create subfolders
            if((mkdir(start.c_str(),0777) < 0) && (errno != EEXIST)){
                perror("can't create folder\n");
                exit(EXIT_FAILURE);
            }

            name = name.substr(pos+1);
        }

        start.erase();

        cout << "Received: " << file << endl;

        //stat is used to check if filename already exist
        struct stat st;
        int exist = stat(file.c_str() ,&st);
        if(exist == 0){
            remove(file.c_str());//if it exist delete
        }

        //create and open file resived
        int out;
        if((out = open(file.c_str(), O_WRONLY|O_CREAT,0666)) < 0){        //open or create .out file
            perror("can't create file\n");
            return 1;       
        }

        //read file resived bytes size
        int filesize;
        if(read(sock, &filesize, sizeof(int)) < 0){
            perror("reading file size in bytes\n");
            exit(EXIT_FAILURE);
        }

        filesize = ntohl(filesize);

        int sum = 0;
        int n;

        //write to file until all bytes are writen
        while (sum < filesize){

            int readsize;
            //read how many bytes to write to file
            if(read(sock, &readsize, sizeof(int)) < 0){
                perror("reading file data\n");
                exit(EXIT_FAILURE);
            }
            readsize = ntohl(readsize);

            char bufff[readsize];
            
            //read file date from server
            n = read(sock, bufff, readsize);
            if( n == -1) {
                perror("couln not read file");
                exit(EXIT_FAILURE);
            }            

            //write file data to created file
            if(write(out, bufff, n) < 0){
                perror("writing data to file\n");
                exit(EXIT_FAILURE);
            }

            sum += n;
        }
        close(out);

    }
    
    close(sock);                 //Close socket and exit
}			     

