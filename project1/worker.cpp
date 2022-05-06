#include "worker.h"

using namespace std;

struct urls{
    string dom;                   //location name
    int appearance;               //times seen in file
};

void worker(){

    vector<struct urls> output;
    std::string fifoname = "fifos/myfifo";
    fifoname.append(std::to_string(getpid()));                  //namepipe name = fifos/myfifo{processid}

    int readfd;
    if ((readfd = open(fifoname.c_str(), O_RDONLY)) < 0){           //open named pipe
        perror("can't open read fifo \n");
        return;
    }

    char buf[1];
    string fifofile;

    while ((read (readfd, buf , 1)) > 0 ){                          //get the name of file
        fifofile.push_back(buf[0]);                                
    }

    int filename;
    if((filename = open(fifofile.c_str(), O_RDONLY)) < 0){          //open the file
        perror("can't open file given by fifo \n");
        return;
    }

    int pos;
    while((pos = fifofile.find("/")) != string::npos){                 //changing the destination folder 
        fifofile = fifofile.substr(pos + 1,fifofile.length());
    } 

    fifofile.append(".out");
    string out = "outfolder/";
    out.append(fifofile);                                           //.out file is gonna save to output/{filenname}.out

    int dotout;
    if((dotout = open(out.c_str(), O_WRONLY|O_CREAT,0666)) < 0){        //open or create .out file
        perror("can't open or create .out file\n");
        return;       
    }

    int n;
    string dom;
    bool endloop = false;

    while (endloop != true){
        n = read(filename, buf, 1);                                     //read one character at a time of file 
        if(n == -1 ){
            perror("error read file \n");
            break;
        }
        if (n == 0){                                                    //n = 0 -> EOF
            endloop = true;
        }

        if(buf[0] != ' ' && buf[0] != '\n' && n != 0){                  //get word by word 
            dom.push_back(buf[0]);
        }
        else{
            if(dom.compare("http://")==0){                              //if word is just http://
                dom.erase();                                            //its not a url
                continue;
            }

            if ((pos = dom.find("http://")) != string::npos) {

                dom = dom.substr(pos,dom.length());                     //remove anything before http://

                if(dom.compare("http://")==0){                          //if word is just http://
                    dom.erase();
                    continue;
                }

                dom = dom.substr(7);                           //remove http:// from posible url
                if (dom.find("www.") != string::npos){          
                    dom = dom.substr(4);                        //if www. exist remove it from posible url
                }
                if((pos = dom.find("/")) != string::npos){      //find the first / if exist
                    dom = dom.substr(0,pos);                    //and get the string before /
                }   
                int i;
                bool found = false;
                for(i = 0; i < output.size(); i++){
                    if(dom.compare(output[i].dom) == 0 ){               //check if location found has been found again
                        found = true;
                        output[i].appearance++;
                        break;
                    }
                }
                if(found == false || output.empty() == true){
                    urls newurl;                                    //create a new url
                    newurl.dom = dom;
                    newurl.appearance = 1;
                    output.push_back(newurl);                       //put it in vector

                }
            }
            dom.erase();                                            //erase string so next string dont overlap
        }
    }

    for (int i = 0; i < output.size(); i++){                        //for each url found

        write(dotout, (output[i].dom).c_str() , output[i].dom.length());        //write to .out the location

        write(dotout, (char*)"  " , 2);                                         //followed by two spaces
        
        char buf = output[i].appearance + '0';                                  //int to char
        write(dotout, &buf, 1);                                                 //followed by number of appearances
        
        write(dotout, (char*)"\n", 1);                                          //followed by change line
    }

    close(readfd);
    close(filename);
    close(dotout);
}