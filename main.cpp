#include <iostream>
using namespace std;
extern "C"{
    int maxofthree(int,int,int);
    void aprint();
    void aprintRed();
}
int main(){
    cout << maxofthree(1,2,3) << endl;
    aprint();
    aprintRed();
    return 0;
}