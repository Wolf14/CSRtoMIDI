/*
Note this code as of now assumes that only 1 MTrk chunk is written to the output file
*/


#include<iostream>
#include<fstream>
#include<iomanip>
#include<cmath>

#define SKIP_BYTES 634
#define FILE_READ_SIZE 20

using std::cout;
using std::endl;
using std::cin;
using std::ofstream;
using std::ifstream;
using std::ios;
using std::hex;
using std::dec;

//iomanip
using std::setw;
using std::setfill;

class sample
{
    public:
    //const char* value1 = "hello";
    //char value2;
    //char value3;
    int value = 258;
};

void writeToFile(ofstream &file, int value, int size){
        file.write(reinterpret_cast<const char*> (&value), size);
    }

//Crude endian reversing function for int(4 bytes)
int ReverseEndian_4_Bytes(int val)
    {
        int tmp = val;
        int final_val = 0;
        int byte2 = 0;
        int byte3 = 0;
        final_val = (tmp << 8*3) + (tmp >> 8*3);
        tmp <<= 8;
        tmp >>= 16;
        byte2 = tmp >> 8;
        byte2 <<= 8;
        byte3 = tmp << 8*3;
        byte3 >>= 8*1;


        final_val += byte2 + byte3;
        
        




        return final_val;
    }


void WriteVariableTime(ofstream &file, unsigned int time)    //max limit is 4 bytes worth of time for now
{
    cout << "entered Entered WriteVariableTime \n";
    uint8_t c = 1;
    int power = 4;
    while(power > 0)
    {

        cout << "entered outside loop in WriteVariableTime function: \n";
        if(time >= pow(2,7*(power-1)) - 1 )
        {
            if(power == 1)
            {
                c=0;
                //cout <<"entered if \n";
            }

            c <<=7;
            unsigned int tmp = time;
            tmp <<= (32-7*(power));
            tmp >>= 25;
            cout << int(c) << "and" << tmp <<endl;
            c += tmp;
            cout << int(c) << "and" << tmp <<endl;
            file << c ;
            cout << int(c) <<endl;

            //cout << "entered loop once in final program \n";

        }
        c=1;
        power -=1;
        cout << "time is: " << time << "    , and comparison with: " << pow(2,7*(power-1)) - 1 << "    ,and power is: "<< power <<endl;
    }
}


int main(int argc, char** argv)
{
    uint8_t header_chunk_len = 0x06;    
    uint8_t ntracks = 0x01;
    short int tickdiv = 0x60;
    int file_pos = 0;
    int track_chunk_len = 0;

    uint8_t number_of_fc = 0;           //after 1st track there are 15 fc commands, after 2nd track there are 2, thus encountering 17 fc implies we have finished reading the file

    int file_size = 0;
    char var[FILE_READ_SIZE];
    
    std::string output_file_name = argv[argc-1];
    output_file_name += "_converted.midi";

    //open file
    ofstream FileWrite(output_file_name, ios::binary );
    ifstream FileRead(argv[argc-1], ios::binary | ios::ate);

    //
    file_size = ((int)FileRead.tellg()) - SKIP_BYTES;
    //set position to 616
    FileRead.seekg(SKIP_BYTES);

    //START OF HEADER CHUNK
    //write Identifier chunk to header
    FileWrite << "MThd";

    //bitshift example: if value = 259 = 0x0103
    //if directly written to a file with:
    //FileWrite << setfill('\0')<<setw(4)<<reinterpret_cast<char*>(&test1);
    //appears as 00 00 03 01    i.e in reverse endian
    //so instead we can write the custom hexadecimall value by:
    /*
    int value2 = 0x03;
    value2 <<=8;
    value2 +=0x01;
    cout << value2 << endl; //this would print 769 = 0x0301
    */



    //Write 0x00000006 to the file
    FileWrite << setfill('\0')<<setw(4)<<header_chunk_len;

    //Format = 0;   i.e only 1 track
    FileWrite << '\0' << '\0';

    //Write number of tracks
    FileWrite << '\0' << ntracks;

    //Write number of divisions in a tick
    FileWrite << '\0' << reinterpret_cast<char*>(&tickdiv);

    //HEADER CHUNK ENDS HERE


    //TRACK CHUNK BEGINS
    //Write Track chunk identifier
    FileWrite << "MTrk";

    //Read how many bytes have been written uptill now to fill in track chunk length later
    file_pos = ((int)FileWrite.tellp());
    FileWrite << '\0'<< '\0'<< '\0'<< '\0';

    //Set Program, in this case using piano on track 0
    int midi_program = 0x01c0;    //stored in reverse endian   
    FileWrite << setw(3)<<reinterpret_cast<char*>(&midi_program);

    //START WRITING THE NOTES OF THE PIECE:
    while(file_size > 0)
    {
        //unsigned char c;
        unsigned int time;

        //Read time and command
        FileRead.read(var,2);
        file_size = file_size-2;

        //write delay time to file
        time = static_cast<unsigned char>(var[0]) ;

        //not handled yet( ff to wait by so much time before the next event)
        //caveat of handling like this, if time exceeds about 361 seconds, memory cannot hold value
        if(static_cast<unsigned char>(var[1]) == (0xff))
        {


            //write delay time to file
            //FileWrite << time ;
            FileRead.read(var,1);   //For now dont do anything
            file_size -= 1;

            time += 128*static_cast<unsigned char>(var[0]);

            //Read time and command again 
            FileRead.read(var,2);
            file_size = file_size-2;
            //add the time from next event to variable time
            time += static_cast<unsigned char>(var[0]);
        }

        if(static_cast<unsigned char>(var[1]) == (0xbc))
        {
            cout << "it does read "<<endl <<endl << endl;
            FileRead.read(var,4);
            file_size -= 4;

            //write delay time to file
            cout << "called WriteVariableTime function" <<endl;
            WriteVariableTime(FileWrite, time) ;

            if(static_cast<unsigned char>(var[0]) == 0x7f)
            {
                uint8_t note_status = 0x80;
                uint8_t note_end_status = 0x40;
                
                FileWrite << note_status;
                FileWrite << var[2] << note_end_status;
            }
            else if(static_cast<unsigned char>(var[0]) != 0x7f)
            {
                 int8_t note_status = 0x90;
                 int8_t note_pressure = var[0];
                 note_pressure *= -1;
                 note_pressure += 127;
                 //cout << endl <<note_pressure << endl; 
                 
                 FileWrite << note_status << var[2] << note_pressure;   //write the note on status 0x09 and pressure    0x7f for full


            }


        }
        
        

        //not handled yet
        else if(static_cast<unsigned char>(var[1]) == (0xb1))
        {
            //write delay time to file
            WriteVariableTime(FileWrite, time) ;
            FileRead.read(var,1);   
            file_size -= 1;

            uint8_t channel = 0xB0;
            uint8_t program = 0x40;

            FileWrite << channel << program << var[0];
        }

        else if(static_cast<unsigned char>(var[1]) == (0xfc))
        {
            //write delay time to file
            //WriteVariableTime(FileWrite, time) ;
            number_of_fc++;         //keeps count of number 0xfc encountered
            FileRead.read(var,1);
            file_size-=1;
        }

        //read 1st byte(time)


        /*
        PrintBytes(var, 1, FileWrite, FileRead);
        file_size--;

        FileRead.read(var, 1);
        file_size--;
        c = static_cast<unsigned char>(var[0]);
        FileWrite << hex <<static_cast<int>(c) << "\t";

        
        if(static_cast<int>(c) == 0xbc)
        {
            //print next 4 bytes
            PrintBytes(var, 4, FileWrite, FileRead);
            FileWrite << "\n";
            file_size = file_size - 4;
            
        }
        else if(static_cast<int>(c) == 0xfc)
        {
            //print next 1 bytes
            PrintBytes(var, 1, FileWrite, FileRead);
            FileWrite << "\n";
            file_size--;
        }
        else if(static_cast<int>(c) == 0xb1)
        {
            //print next 1 bytes
            PrintBytes(var, 1, FileWrite, FileRead);
            FileWrite << "\n";
            file_size--;
        }
        else if(static_cast<int>(c) == 0xff)
        {
            //print next 1 bytes
            PrintBytes(var, 1, FileWrite, FileRead);
            FileWrite << "\n";
            file_size--;
            no_of_ff++;
        }
        else if(static_cast<int>(c) == 0x00)
        {
            //print next 2 bytes
            PrintBytes(var, 2, FileWrite, FileRead);
            FileWrite << "\n";
            file_size = file_size - 2;
        }
        else if(static_cast<int>(c) == 0x02)
        {
            //print next 2 bytes
            PrintBytes(var, 2, FileWrite, FileRead);
            FileWrite << "\n";
            file_size = file_size - 2;
        }
        */

    }
    uint8_t end_of_file[3] = {0xff,0x2f,0x00};

    FileWrite << '\0' << end_of_file[0] << end_of_file[1] << end_of_file[2] ;
    
    //Write to file number of bytes in chunklen
    track_chunk_len = FileWrite.tellp();
    track_chunk_len -= file_pos+4;

    track_chunk_len = ReverseEndian_4_Bytes(track_chunk_len);

    

    FileWrite.seekp(file_pos,ios::beg);
    writeToFile(FileWrite,track_chunk_len, 4);
    //FileWrite << track_chunk_len;
    cout << track_chunk_len;


    FileWrite.close();
    FileRead.close();
    return 0;
}