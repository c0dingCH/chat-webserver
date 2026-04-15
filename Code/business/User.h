#pragma once
#include"MysqlPool.h"
#include"Api.h"
#include<memory>
#include<string>


namespace api::user{
  void HandleRegister(const HttpObjs& hs);
  void HandleLogin(const HttpObjs& hs);
  void HandleProfile(const HttpObjs& hs);
  void Download(const HttpObjs & hs, const std::string & path);
  void Upload(const HttpObjs & hs, const std::string & path);

  template<typename Func>
  void WithConnection(std::unique_ptr<MysqlPool>& mysql_pool, Func && func){
    std::unique_ptr<Mysql> db = mysql_pool->Get();
    func(db);
    mysql_pool->Put(db);
  }
   

}
