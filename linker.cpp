//
//  linker.cpp
//  lab1
//  yuze gong
//  Created by apple on 2020/09/27.
//

#include <iostream>
#include <regex>
#include <map>
#include <iomanip>
#include <cstring>
#include <string>

using namespace std;

/*define constants*/
const int MAX_SYMBOL_LEN = 16;
const int MAX_COUNT_DEF = 16;
const int MAX_COUNT_USE=16;
const int MAX_MEMORY_SIZE = 512;

/**
 * global variables
 */
int linenum=1;
int currentlinenum=1;
int modulenum=0;
int modulecount=0;
int lineoffset=0;
int charoffset=1;
int charcount=0;

char* filepath;
FILE * input;

bool End_module = false;//flag to handle the special case when EOF is space or new line;
bool is_pass_one = false;
bool is_pass_two = false;

regex symbolname("([A-Z]|[a-z])([A-Z]|[a-z]|[0-9])*");

map<string,int> symboltable;
map<string,int> symbolmodule;
map<string,int> symbolusecount;
map<string,int> current_def_list;

map<string,bool> is_symbol_used_tmp;
map<string, bool> multiple_defined_symbols;

vector<string> current_use_list;

/**
 * The function to print the errors occurred at parsing to console
 * @param errcode
 */
void __parseerror(int errcode) {
    static char* errstr[] = {
            "NUM_EXPECTED", // Number expect
            "SYM_EXPECTED", // Symbol Expected
            "ADDR_EXPECTED", // Addressing Expected which is A/E/I/R
            "SYM_TOO_LONG", // Symbol Name is too long
            "TOO_MANY_DEF_IN_MODULE", // > 16
            "TOO_MANY_USE_IN_MODULE", // > 16
            "TOO_MANY_INSTR", //total num_instr exceeds memory size (512)
    };
    cout<<"Parse Error line "<<currentlinenum<< " offset "<<charoffset<<": "<<errstr[errcode]<<endl;
    exit(1);
}


void __passerror(int num)
{
    switch (num)
    {
        case 1:
            cout << "functions for pass_1 can only be called when IN_PASS_ONE=TRUE and IN_PASS_TWO=FALSE. " << endl;
            exit(1);
        case 2:
            cout << "functions for pass_2 can only be called when IN_PASS_ONE=FALSE and IN_PASS_TWO=TRUE. " << endl;
            exit(1);
    }
}


/**
 * the tokenizer-a token as a string without spaces
 */
string getToken(){
    string token;
    //cout<<"s="<<s<<endl;
    char current_char;
    current_char=fgetc(input);
    charcount++;
    if(feof(input)){
        return token;
    }
    /*read the token until hit a space or new line*/
    while(current_char!=' '&&current_char!='\t'&&current_char!='\n'&&!feof(input)){
        charcount++;
        token = token+current_char;
        current_char=fgetc(input);
    }

    currentlinenum = linenum;
    charoffset = charcount;
    
    //if(current_char=' ')
    //if(current_char='\t')
    
    if(current_char == '\n'){
        linenum++;
        charcount=0;
        //charoffset=0;
    }
    if(token.empty()){
        token = getToken();
    }
    
    return token;
}

/**
 * read the integers from input
*/

 int readInt(){
     int val = 0;
     string s = getToken();
     bool number;
     
     if(s.empty()){
         if(!End_module) {
             return -1;
         }
         else{
             charoffset-=s.length();
             __parseerror(0);
         }
     }
     
     for(int i=0;i < s.length();i++){
         if(isdigit(s[i])==false){
             number=false;
             break;
         }
         else{
             number=true;
         }
     }
     if(number){
         val = stoi(s);
     }
     else{
         charoffset-=s.length();
         __parseerror(0);//the token does not match the regex and is not an integer
     }
 

     return val;
 }
/**
 * read a symbol from input
 */
string readSymbol(){
    //cout<<"...read symbol"<<endl;
    string s = getToken();

    if(!regex_match(s,symbolname)){
        charoffset-=s.length();
        __parseerror(1);//the token is not a legal symbol name
    }
    if(s.length()>MAX_SYMBOL_LEN){
        charoffset-=s.length();
        __parseerror(3);//the token is bigger than 16 digits
    }
   

    return s;
}

/**
 * read the instruction list
 */

void readIEAR_pass_one(){
    //cout<<"...read inst list"<<endl;
    int codecount = readInt();
    int temp_num=0;
    temp_num=codecount;
    
    if(lineoffset+codecount>MAX_MEMORY_SIZE){
        charoffset-=to_string(codecount).length();
        __parseerror(6);
    }

    for(int i=0;i<codecount;i++){
        string type =getToken();

        if(type.compare("A")!=0&&type.compare("E")!=0&&type.compare("I")!=0&&type.compare("R")!=0){
            charoffset-=type.length();
            __parseerror(2);
        }
        if(is_pass_one && !is_pass_two){
            int instr = readInt();
        }
        else{
            __passerror(1);
        }
        
    }

    lineoffset = lineoffset + temp_num;
    
    for(auto const &deflist:current_def_list){
        if(deflist.second>lineoffset-1){
            symboltable[deflist.first] = lineoffset - temp_num;//reset to base
            cout<<"Warning: Module "<<modulecount<<": "<<deflist.first<<" too big "<<deflist.second<<" (max="<<lineoffset-1<<") assume zero relative"<<endl;
        }
    }
    
}

void readIEAR_pass_two(){
    //cout<<"...read inst list"<<endl;
    int codecount = readInt();
    int temp_num;
    temp_num=codecount;
    
    if(lineoffset+codecount>MAX_MEMORY_SIZE){
        charoffset-=to_string(codecount).length();
        __parseerror(6);
    }

    for(int i=0;i<codecount;i++){
        string type =getToken();

        if(type.compare("A")!=0&&type.compare("E")!=0&&type.compare("I")!=0&&type.compare("R")!=0){
            charoffset-=type.length();
            __parseerror(2);
        }

        int instr_num = readInt();
        string str;

        if(!is_pass_one && is_pass_two) {
            cout << setfill('0') << setw(3) << modulenum << ": ";
            if(instr_num>=10000){
                instr_num=9999;
                cout<< setfill('0') << setw(4) << instr_num;
                if(type.compare("I")==0){
                    cout << " Error: Illegal immediate value; treated as 9999";
                }
                else{
                    cout<<" Error: Illegal opcode; treated as 9999";
                }
            }
            else {
                if (type.compare("I") == 0){
                    cout<< setfill('0') << setw(4) << instr_num;
                }
                if (type.compare("R") == 0) {
                    if(instr_num%1000>temp_num){
                        instr_num = instr_num/1000*1000+lineoffset;
                        cout<< setfill('0') << setw(4) << instr_num;
                        cout<<" Error: Relative address exceeds module size; zero used";
                    }
                    else {
                        instr_num = instr_num+lineoffset;
                        cout<< setfill('0') << setw(4) << instr_num;
                    }
                }
                if (type.compare("E") == 0) {
                    if(instr_num % 1000>=stoi(current_use_list[0])){
                        cout<< setfill('0') << setw(4) << instr_num;
                        cout << " Error: External address exceeds length of uselist; treated as immediate";
                    }
                    else {
                        string s = current_use_list[instr_num % 1000+1];
                        is_symbol_used_tmp[s]=true;
                        if (symboltable.find(s) == symboltable.end())//not found
                        {
                            cout<< setfill('0') << setw(4) << instr_num;
                            str = s;
                            cout << " Error: " << str << " is not defined; zero used";
                        } else {//found
                            symbolusecount[s] += 1;
                            instr_num =instr_num/1000 * 1000+ symboltable[s];
                            cout<< setfill('0') << setw(4) << instr_num;
                        }
                    }
                }
                if(type.compare("A")==0){
                    if (instr_num%1000>MAX_MEMORY_SIZE){
                        instr_num = instr_num-instr_num%1000;
                        cout<< setfill('0') << setw(4) << instr_num;
                        cout<< " Error: Absolute address exceeds machine size; zero used";
                    }
                    else{
                    cout<< setfill('0') << setw(4) << instr_num;
                    }
                }

            }
            

            cout << endl;
            ++modulenum;

            current_use_list.clear();
        }
        else{
            __passerror(2);
        }
    }
    
    if(!is_pass_one && is_pass_two){
    for (auto const &ent1 : is_symbol_used_tmp) {
        if (!ent1.second) {
            cout << "Warning: Module " << modulecount << ": " << ent1.first
                << " appeared in the uselist but was not actually used"<<endl;
        }
    }
        is_symbol_used_tmp.clear();
    }
    else{
        __passerror(2);
    }
    lineoffset = lineoffset + temp_num;
    
    for(auto const &deflist:current_def_list){
        if(deflist.second>lineoffset-1){
            symboltable[deflist.first]=lineoffset-temp_num;//reset to base
            cout<<"Warning: Module "<<modulecount<<": "<<deflist.first<<" too big "<<deflist.second<<" (max="<<lineoffset-1<<") assume zero relative"<<endl;
        }
    }
    
}

/**
 * pass one read def list
 */
void creatSymbol(string str,int val){
    if(is_pass_one && !is_pass_two){
        if(symboltable.find(str)==symboltable.end())//not found
            {
                symboltable[str]=lineoffset+val;
                current_def_list[str]=lineoffset+val;
                symbolmodule[str]=modulecount;
                symbolusecount[str]=0;
            }
        else{//found
            multiple_defined_symbols[str]=modulecount;
        }
    }
    else{
        __passerror(1);
    }
}
/**
 * create the module
 */

void create_module_pass_one(){
    //cout<<"...create module"<<endl;
    modulecount++;
    //read_def_list();
    
    int defcount=readInt();
    if (defcount==-1){
        return;
    }
    if(defcount>MAX_COUNT_DEF){
        charoffset-=to_string(defcount).length();
        __parseerror(4);
    }
    
    End_module = true;//Set flag to true since new module is started
    
    for (int i=0;i<defcount; i++) {
        string str= readSymbol();
        int val=readInt();
        creatSymbol(str,val);
    }

    if(End_module) {
        //read_use_list();
        
        int usecount = readInt();

        if(usecount>MAX_COUNT_USE){
            charoffset-=to_string(usecount).length();
            __parseerror(5);
        }
        current_use_list.push_back(to_string(usecount));
        for(int i = 0;i<usecount;i++){
            string s = readSymbol();
            current_use_list.push_back(s);
        }

        for (int i = 1;i<=stoi(current_use_list[0]);i++) {
            is_symbol_used_tmp[current_use_list[i]]=false;
        }
        
        readIEAR_pass_one();
        
        current_def_list.clear();
    }
    else return;
}

void create_module_pass_two(){
    //cout<<"...create module"<<endl;
    modulecount++;
    //read_def_list();
    
    int defcount=readInt();
    if (defcount==-1){
        return;
    }
    if(defcount>MAX_COUNT_DEF){
        charoffset-=to_string(defcount).length();
        __parseerror(4);
    }
    
    End_module = true;//Set flag to true since new module is started
    
    for (int i=0;i<defcount; i++) {
        string str= readSymbol();
        int val=readInt();
        //creatSymbol(str,val);
    }

    if(End_module) {
        //read_use_list();
        
        int usecount = readInt();

        if(usecount>MAX_COUNT_USE){
            charoffset-=to_string(usecount).length();
            __parseerror(5);
        }
        current_use_list.push_back(to_string(usecount));
        for(int i = 0;i<usecount;i++){
            string s = readSymbol();
            current_use_list.push_back(s);
        }

        for (int i = 1;i<=stoi(current_use_list[0]);i++) {
            is_symbol_used_tmp[current_use_list[i]]=false;
        }
        
        readIEAR_pass_two();
        
        current_def_list.clear();
    }
    else return;
}

/**
 * first pass of the linker
 */
void first_pass() {
    input = fopen(filepath,"r");
    is_pass_one = true;
    is_pass_two = false;
    if (input == NULL) {
        cout<<"Error opening file";
    }
    else {
        while (!feof(input)) {
            //get rid of extra spaces
            create_module_pass_one();
            End_module = false;//set flag to false to indicate that a new module is not started yet
        }
    }
    fclose(input);//The linker must process the input twice.
}

/**
 * second pass of the linker
 */
void second_pass(){
    input = fopen(filepath,"r");
    is_pass_one = false;
    is_pass_two = true;
    lineoffset = 0;
    modulecount=0;
    if (input == NULL) {
        cout<<"Error opening file";
    }
    else{
        while (!feof(input)){
        create_module_pass_two();
        End_module = false;//set flag to false to indicate that a new module is not started yet
        }
    }
    cout<<endl;
    for (auto const &ent1 : symbolusecount) {
        if(ent1.second==0){
            cout<<"Warning: Module "<< symbolmodule[ent1.first] <<": " << ent1.first<<" was defined but never used"<<endl;
        }
    }

    fclose(input);
}

/**
 * main program
*/

int main(int argc, char* argv[])
{
    // Invalid input file path
    if(argc == 1){
        
        cout << "Not given a input file path" << endl;
        return 1;
        
    }
    
    if (argc > 2) {// argc should be two, take one file a time as input
        
            cout << "One text file at a time. Not a valid input file path: " << "arguments length is " << argc << "!= 2" << endl;
            return 1;
    }
    
    filepath = argv[1];
    
    
    first_pass();
    
    // print symbol table
    cout<<"Symbol Table"<<endl;
    for (auto i=symboltable.begin(); i!=symboltable.end(); i++) {
        cout << i->first << "=" << i->second;
        if(multiple_defined_symbols[i->first]){
            cout<<" Error: This variable is multiple times defined; first value used";
        }
        
        cout<<endl;
    }
    cout<<"\nMemory Map"<<endl;//even an empty program should have the “Symbol Table” and “Memory Map” line.
    

    second_pass();
    
    return 0;

}
