#include "headers/leonlog.h"
#include <iostream>
#include <string>

using namespace std;

void print(){
    cout << "Hello from inside the library" << endl;
}

void print(const string& str){
    cout << "Hello from inside the library" << endl;
    cout << "You entered : " << str << endl;
}

// Edit this file according your needs. Add whatever fucntion
// or class you like, as long as you declare it in an .h file
