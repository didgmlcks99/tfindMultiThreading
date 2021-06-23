#include <iostream>
#include <stdio.h>
#include <time.h>

using namespace std;

int some(int n, int m){
    int some[n+1][m+1];
        
    for(int i = 1; i <= n; i++){
        some[i][1] = 1;
    }
    for(int j = 1; j <= m; j++){
        some[1][j] = 1;
    }

    for(int x = 2; x <= n; x++){
        for(int y = 2; y <= m; y++){
            some[x][y] = some[x][y-1] + some[x-1][y] + some[x-1][y-1];
        }
    }
    return some[n][m];
}

int main(){
    int n, m;
    clock_t t;

    while(1){
        do{
            cout << "Insert two numbers : ";
            cin >> n >> m;

            if(n > 1 && m > 1) break;

            cout << "Both number has to be greater than 1" << endl;
        }while(1);

        t = clock();

        int result = some(n, m);

        t = clock() - t;

        cout << "result : " << result <<  endl;
        cout << "time : " << t << endl;
    }
    return 0;
}

