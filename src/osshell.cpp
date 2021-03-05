#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

void allocateArrayOfCharArrays(char ***array_ptr, size_t array_length, size_t item_size);
void freeArrayOfCharArrays(char **array, size_t array_length);
void splitString(std::string text, char d, std::vector<std::string>& result);
void removeLeadingSpaces(char *arr);
bool isHistoryCommand(char *arr);
bool isExitCommand(char *arr);
void splitPathAndCommand(std::string text,  std::vector<std::string>& commmand);
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result);
bool fileExecutableExists(std::string file_path);
void printHistory();
void appendCommandToHistory(std::string command,  int size);
void printHistoryNum(int num);
std::ofstream tempfile;
std::ifstream historyfile;

int main (int argc, char **argv)
{

    // Get list of paths to binary executables
    // `os_path_list` supports up to 16 directories in PATH,
    // each with a directory name length of up to 64 characters

    std::vector<std::string> os_path_list;
    char* os_path = getenv("PATH");
    splitString(os_path, ':', os_path_list);

    // Allocate space for input command lists
    // `command_list` supports up to 32 command line parameters,
    // each with a parameter string length of up to 128 characters
    std::string user_command;
    std::vector<std::string> command_list;
    char **command_list_exec; 

    // Repeat:
    //  Print prompt for user input: "osshell> " (no newline)
    //  Get user input for next command
    //  If command is `exit` exit loop / quit program
    //  If command is `history` print previous N commands
    //  For all other commands, check if an executable by that name is in one of the PATH directories
    //   If yes, execute it
    //   If no, print error statement: "<command_name>: Error command not found" (do include newline)
    // Welcome message

    printf("Welcome to OSShell! Please enter your commands ('exit' to quit).\n");

     // Shows how to split a command and prepare for the execv() function
    /*std::string example_command = "ls -lh";
    splitString(example_command, ' ', command_list);
    vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);*/
    // use `command_list_exec` in the execv() function rather than looping and printing
 
    std::string userinput;
    bool exitShould = true;
    while(exitShould) //while loop will run until user exits
    {
        std::cout << "osshell> ";
        std::getline(std::cin, userinput);//gets user input
        splitPathAndCommand(userinput, command_list);//splits user input
        vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);//converts the commands from vector to char**
        char exitStr[] = "exit";//used to compare against user input
        char history[] = "history";
        char clear[] = "clear";
        int pid;
        bool add = true;//whether or not to add to history
        
        if(!userinput.empty())//if empty then new line
        {
             if(strcmp(command_list_exec[0], exitStr) == 0)//means to exit
            {
                std::cout << "\n";
                exit(0);
            }
            else if(strcmp(command_list_exec[0], history) == 0)//one of the history commands
            {
                if(command_list_exec[1] == NULL)//history command
                {
                    printHistory();
                }

                else if(strcmp(command_list_exec[1], clear) == 0)//history command with clear
                {
                    std::remove("historyText.txt");
                    add = false;
                }

                else //number history command
                {
                    bool isNum = true;
                    for(int j = 0; j < sizeof(command_list_exec[1]);j++)
                    {
                        if(!isdigit(command_list_exec[1][j]))
                        {
                            isNum =false;
                            j = sizeof(command_list_exec[1]) +1; 
                        }
                    }
                    if(isNum && command_list_exec[2] == NULL)//is actually history 'number' command
                    {
                        std::string histNum(command_list_exec[1]);
                        printHistoryNum(stoi(histNum));
                    }
                    else//not actually history number command
                    {
                        std::cout << "<command_name>: Error command not found" << "\n";
                    }
                }
            }
            else if(command_list_exec[0][0] == '.' || command_list_exec[0][0] == '/')//if command in directory
            {
                if(std::filesystem::exists(command_list_exec[0]))//checks to see if command exist
                {
                    
                    pid = fork();//creates child
                    if(pid ==0)//checks to see if child
                    {
                        execv(command_list_exec[0], command_list_exec);
                        exit(0);
                    }
                    else//otherwise parent
                    {
                        int status;
                        waitpid(pid, &status, 0);//wait for child to finish
                        
                    }
                }
                else//command not found
                {
                    std::cout << "<command_name>: Error command not found" << "\n";
                }
            }
            else//command in any directory
            {
                int found = 0;
                for(int i = 0; i < os_path_list.size(); i++)//Runs through possible paths
                {
                    std::string filePath = os_path_list[i] + "/" + command_list_exec[0];//creation of path
                    if(std::filesystem::exists(filePath))//if file path exist
                    {

                        pid = fork();//creates child
                        if(pid ==0)//if child
                        {
                            execv(filePath.c_str(), command_list_exec);//run exec
                            exit(0);
                        }
                        else//parent
                        {
                            int status;
                            waitpid(pid, &status, 0);//wait for child
                        }
                        i = os_path_list.size() + 1;
                        found = 1;
                    }
                    
                }
                if(found == 0)//Did not find command
                {
                    std::cout << "<command_name>: Error command not found" << "\n";
                }
            }
        }
        if(add)//add command to history
        {
            appendCommandToHistory(userinput,  command_list.size());
        }
        freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
    }
    return 0;
}

/*

   array: list of strings to be freed

   array_length: number of strings in the list to free

*/

void freeArrayOfCharArrays(char **array, size_t array_length)

{

    int i;

    for (i = 0; i < array_length; i++)

    {

        delete[] array[i];

    }

    delete[] array;

}

 

/*

   text: string to split

   d: character delimiter to split `text` on

   result: NULL terminated list of strings (char **) - result will be stored here

*/

void splitString(std::string text, char d, std::vector<std::string>& result)
{
    enum states { NONE, IN_WORD, IN_STRING } state = NONE;

    int i;
    std::string token;
    result.clear();
    for (i = 0; i < text.length(); i++)
    {
        char c = text[i];
        switch (state) {
            case NONE:
                if (c != d)
                {
                    if (c == '\"')
                    {
                        state = IN_STRING;
                        token = "";
                    }
                    else
                    {
                        state = IN_WORD;
                        token = c;
                    }
                }
                break;
            case IN_WORD:
                if (c == d)
                { 
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
            case IN_STRING:
                if (c == '\"')
                {
                    result.push_back(token);
                    state = NONE;
                }
                else
                {
                    token += c;
                }
                break;
        }
    }
    if (state != NONE)
    {
        result.push_back(token);
    }

}

/*
   list: vector of strings to convert to an array of character arrays
   result: pointer to an array of character arrays when the vector of strings is copied to
*/
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result)
{
    int i;
    int result_length = list.size() + 1;
    *result = new char*[result_length];
    for (i = 0; i < list.size(); i++)
    {
        (*result)[i] = new char[list[i].length() + 1];
        strcpy((*result)[i], list[i].c_str());
    }
    (*result)[list.size()] = NULL;
}

bool fileExecutableExists(std::string file_path)
{
    bool exists = false;
    // check if `file_path` exists
    // if so, ensure it is not a directory and that it has executable permissions

    return exists;
}

void splitPathAndCommand(std::string text, std::vector<std::string>& command)//all this does is basically call splitstring
{
        std::vector<std::string> brokenText;
        splitString(text, ' ', command);
}

void printHistoryNum(int num)//command for print history for a certain number
{
    std::vector<std::string> fileLines;
    std::fstream history;
    if(std::filesystem::exists("historyText.txt"))
    {
        history.open("historyText.txt");
        std::string line;
        while(getline(history, line))
        {
            fileLines.push_back(line);
        }
        char** fileContent;
        vectorOfStringsToArrayOfCharArrays(fileLines, &fileContent);
        for(int i = fileLines.size()-num; i < fileLines.size() && i >= 0;i++)
        {
          std::cout << fileContent[i] << "\n";
        }
    }

    
    
    

}

void printHistory() {//print history
    std::ifstream f("historyText.txt");//open file

    if (f.is_open())//while open print to screen
        std::cout << f.rdbuf();
}

void appendCommandToHistory(std::string command,  int size) {//adds to the history file
    
    std::ofstream historyfile("historyText.txt", std::ios::app);//adds to history file, opens file
    historyfile << command;//writes to file
    historyfile << "\n";//new line
    historyfile.close();//close history file.
}
