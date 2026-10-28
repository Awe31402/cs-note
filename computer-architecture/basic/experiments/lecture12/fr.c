int f(int n){volatile int a[8]; for(int i=0;i<8;i++)a[i]=i*n; return a[n&7];}
