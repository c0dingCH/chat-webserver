#include<iostream>
#include<vector>
#include<string>
#include"Mysql.h"
#include<sw/redis++/redis++.h>
using namespace std;

int main(){
  sw::redis::Redis re("tcp://127.0.0.1:6379");
  re.lpush("que", {"c", "b","aaa"});
  vector<string>v;
  re.lrange("que", 0, -1, back_inserter(v));  
  if(v.size()){
    for(auto s : v)cout<<s<<endl;

  }
}
