#include <iostream>
#include <unistd.h>
#include "AsyncLogging.h"
#include "HttpRequest.h"
using namespace std;

int main(){
  HttpRequest r;
  r.SetBody("{\"name\":\"cheng\",\"age\":22}");
  r.ParseJson2Dom();
  auto &d = r.GetDom(); 
  
  cout<<d["name"].GetString()<<" "<<d["age"].GetInt()<<endl;
}
