#include"TcpConnection.h"
#include"HttpServer.h"
#include"HttpResponse.h"
#include"HttpRequest.h"
#include"HttpServer.h"

#include"User.h"
#include"Mysql.h"

#include<rapidjson/document.h>

void api::user::HandleRegister(const HttpObjs& hs){
  WithConnection(hs.server->GetMysqlPool(),[&](std::unique_ptr<Mysql> &db){
    rapidjson::Document & dom = hs.request->GetDom();       
    auto res_db = db->Insert(dom["username"].GetString(), dom["password"].GetString());
    if(res_db == Mysql::MysqlStatus::kSuccess){
      hs.response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
    }
    else{
      hs.response->SetStatusCode(HttpResponse::HttpStatusCode::k100Continue);
    }

    hs.response->SetBody(db->GetMsg(res_db));
    hs.response->AddHeader("Content-Type","text/html");

  });
}

void api::user::HandleLogin(const HttpObjs& hs){
  WithConnection(hs.server->GetMysqlPool(),[&](std::unique_ptr<Mysql> &db){
    rapidjson::Document & dom = hs.request->GetDom();      
    auto res_db = db->Login(dom["username"].GetString(), dom["password"].GetString());
    if(res_db == Mysql::MysqlStatus::kSuccess){
      hs.server->AddConn(dom["username"].GetString(), hs.conn);
      hs.response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
       
      hs.conn->SetOnCloseBusi([id = std::string(dom["username"].GetString()), server = hs.server](){ 
        server->RemoveConn(std::move(id));
      });

    }
    else{
      hs.response->SetStatusCode(HttpResponse::HttpStatusCode::k100Continue);
    }
    
    hs.response->SetBody(db->GetMsg(res_db));
    hs.response->AddHeader("Content-Type","text/html");
    
  });
}

void api::user::HandleProfile(const HttpObjs& hs){
  WithConnection(hs.server->GetMysqlPool(),[&](std::unique_ptr<Mysql> &db){
    rapidjson::Document & dom = hs.request->GetDom();       
    auto res_db = db->Update(dom["username"].GetString(), dom["password"].GetString());
    if(res_db == Mysql::MysqlStatus::kSuccess){
      hs.response->SetStatusCode(HttpResponse::HttpStatusCode::k200K);
    }
    else{
      hs.response->SetStatusCode(HttpResponse::HttpStatusCode::k100Continue);
    }
  
    hs.response->SetBody(db->GetMsg(res_db));
    hs.response->AddHeader("Content-Type","text/html");
  
  });
}
