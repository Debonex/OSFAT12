extern void hello_other_lib(int*,int*);
extern int share;
int main(){
    int a=1;
    hello_other_lib(&a,&share);
    return 0;
}