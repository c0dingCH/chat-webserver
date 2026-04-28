#include<iostream>
#include<vector>
#include<string>
#include"Redis.h"
#include"MysqlPool.h"
#include<sw/redis++/redis++.h>
#include<rapidjson/rapidjson.h>
#include<rapidjson/document.h>

using namespace std;

int main(){
  const char* json = "{\"sender_id\":\"root\",\"content\":\"{\\\"content\\\":\\\"\xe4\xbd\xa0\xe6\x98\xaf...\\\"}\",\"date\":\"\"}";
rapidjson::Document doc;
doc.Parse(json,strlen(json));
assert(!doc.HasParseError());   // 通过
auto t = doc["date"].GetString(); 
std::cout<< t << std::endl;
}
