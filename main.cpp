#include <iostream>
using namespace std;
extern "C"{
    void aprint(char*,int);
    void aprintRed(char*,int);
}
int main(){
    //data
    char str[] = "Hello,World!";

    //text
    aprint(str,sizeof(str));
    aprintRed(str,sizeof(str));
    return 0;
}