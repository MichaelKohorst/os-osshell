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

 

void allocateArrayOfCharArrays(char ***array_ptr, size_t array_length, size_t item_size);
void freeArrayOfCharArrays(char **array, size_t array_length);
void splitString(std::string text, char d, std::vector<std::string>& result);
void removeLeadingSpaces(char *arr);
bool isHistoryCommand(char *arr);
bool isExitCommand(char *arr);
void splitPathAndCommand(std::string text, char *path, std::vector<std::string>& commmand);
void vectorOfStringsToArrayOfCharArrays(std::vector<std::string>& list, char ***result);
bool fileExecutableExists(std::string file_path);

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
 
    bool should_exit = true;
    while(should_exit) 
    {
        std::cout << "osshell> ";
        char *user_input = new char[128];
        std::cin.getline(user_input, 128);
        char path[64];
        strcat(path,"/usr/bin/");
        std::string str(user_input);
        splitPathAndCommand(str, path, command_list);
        vectorOfStringsToArrayOfCharArrays(command_list, &command_list_exec);
        strcat(path, command_list_exec[0]);
        //std::cout << "str: " << str << "\n"; 
        std::cout << "path: " << path << "\n"; 
        char exit[] = "exit";
        char history[] = "history";
        int pid;
        std::cout << "command_list_exec[0]: " << command_list_exec[0] << "\n"; 
        if(strcmp(command_list_exec[0], exit) == 0)
        {
           should_exit = false;
        }
        else 
        {
            int found = 0;
            for(int i = 0; i < os_path_list.size(); i++)
            {
                if(strcmp(os_path_list[i].c_str(), path))
                {
                    std::cout << "os_path: " << os_path_list[i].c_str() << "\n";
                    //pid = fork();
                    found = 1;

                }
            }

            /*if(found == 1 && pid == 0)
            {
               execv(path, command_list_exec); 
            }
            else if(found == 1 && pid != 0)
            {
                int status;
                //waitpid(pid, &status,0);
            }*/
            if(command_list_exec[0] == "")
            {

            }
            else    
            {
                std::cout << "<command_name>: Error command not found" << "\n";
            }
            std::cout << "found: " << found << "\n";
            freeArrayOfCharArrays(command_list_exec, command_list.size() + 1);
        }
        
    }
    
    return 0;
}

 

/*

   array_ptr: pointer to list of strings to be allocated

   array_length: number of strings to allocate space for in the list

   item_size: length of each string to allocate space for

*/

void allocateArrayOfCharArrays(char ***array_ptr, size_t array_length, size_t item_size)
{

    int i;

    *array_ptr = new char*[array_length];

    for (i = 0; i < array_length; i++)

    {

        (*array_ptr)[i] = new char[item_size];

    }

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

void splitPathAndCommand(std::string text, char *path, std::vector<std::string>& command)
{
        std::vector<std::string> brokenText;
        splitString(text, ' ', command);
}
