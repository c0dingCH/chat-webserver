#include "Mysql.h"
#include "Logging.h"
#include <iostream>

Mysql::Mysql(){
  mysql_init(&conn_);
  if(!mysql_real_connect(&conn_, "localhost", "root", "","chat_db",3306,NULL, 0)){
    LOG_ERROR << "db connect error!";
  }
}

Mysql::~Mysql(){
  mysql_close(&conn_);
}

static std::string varchar(const std::string &s){
  return "'" + s + "'";
}

//static void print_res(MYSQL_RES * res){
//  MYSQL_FIELD * fields;
//  MYSQL_ROW row;
//  int cols = mysql_num_fields(res);
//  fields = mysql_fetch_fields(res);
//  for(int i = 0;i < cols;i++){
//    std::cout<<fields[i].name<<"\t\t";
//  }
//
//  std::cout<<std::endl;
//
//  while((row = mysql_fetch_row(res))){
//    for(int i = 0;i < cols;i++){
//      if(!row[i])std::cout<<"NULL\t\t";
//      else std::cout<<row[i]<<"\t\t";
//    }
//
//    std::cout<<std::endl;
//  }
//}
//
Mysql::MysqlStatus Mysql::Insert(const std::string &user_id, const std::string &password){
  if(Select(user_id) == MysqlStatus::kSuccess)return MysqlStatus::kAlreadyIn;
  std::string sql = "insert into users values (" + varchar(user_id) + " , " + varchar(password) + ")";
  int res  = mysql_query(&conn_, sql.c_str());
  if(!res)return MysqlStatus::kSuccess;
  else {
    LOG_ERROR << "Insert error !";
    return MysqlStatus::kFailed;
  }
}


Mysql::MysqlStatus Mysql::Delete(const std::string &user_id){
  if(Select(user_id) != MysqlStatus::kSuccess)return MysqlStatus::kNotFound;
  std::string sql = "delete from users where user_id = " + varchar(user_id);
  int res = mysql_query(&conn_, sql.c_str());
  if(!res)return MysqlStatus::kSuccess;
  else{
    LOG_ERROR<<"Delete error !";
    return MysqlStatus::kFailed;
  } 
}

Mysql::MysqlStatus Mysql::Update(const std::string &user_id, const std::string &password){
  if(Select(user_id) != MysqlStatus::kSuccess)return MysqlStatus::kNotFound;
  std::string sql = "update users set password = " + varchar(password) + " where user_id = " + varchar(user_id);
  int res = mysql_query(&conn_, sql.c_str());
  if(!res)return MysqlStatus::kSuccess;
  else {
    LOG_ERROR<<"Update error !";
    return MysqlStatus::kFailed;
  }
}




Mysql::MysqlStatus Mysql::Select(const std::string &user_id){
  std::string sql = "select * from users where user_id = " + varchar(user_id);
  MYSQL_RES * res{nullptr};
  int st = mysql_query(&conn_, sql.c_str());
  if(st != 0){
    LOG_ERROR << "Select error !";
    return MysqlStatus::kFailed;
  }

  res = mysql_store_result(&conn_);
  int row = mysql_num_rows(res);
  mysql_free_result(res);  

  return row == 1 ? MysqlStatus::kSuccess : MysqlStatus::kNotFound;
}

const char * Mysql::GetMsg(MysqlStatus state){
  switch(state){
    case MysqlStatus::kUserIdIllegal:return "User_id Illegal";
    case MysqlStatus::kPasswordIllegal:return "Password Illegal";
    case MysqlStatus::kAlreadyIn:return "User_id Already In";
    case MysqlStatus::kNotFound:return "Not Found";
    case MysqlStatus::kSuccess:return "Operation Successed";
    case MysqlStatus::kFailed: return "Operation Failed";
  }
  return "No such status";
}
