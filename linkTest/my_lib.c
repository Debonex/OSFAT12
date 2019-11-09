int share = 100;
void hello_other_lib(int* a,int* b){
    int c=*a;
    *a=*b;
    *b=c;
}