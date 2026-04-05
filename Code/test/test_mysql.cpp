#include"Mysql.h"
#include"MysqlPool.h"
using namespace std;

int main(){
  auto * pool = new MysqlPool();
  pool->Start();
  auto conn = pool->Get(); 

  cout<<conn->GetMsg(conn->Select("123"))<<endl;


  cout<<conn->GetMsg(conn->Insert("123", "123"))<<endl;
  
  
  cout<<conn->GetMsg(conn->Update("123","321"))<<endl;
  
  
  cout<<conn->GetMsg(conn->Delete("123"))<<endl;

}
