#pragma once
#include "MysqlPool.h"
#include "HttpServer.h"
#include <memory>
#include <string>


class User {
  public:
    explicit User(const std::string& user_id) : user_id_(user_id) {}

    static void HandleRegister(const HttpObjs& hs);
    static void HandleLogin(const HttpObjs& hs);
    static void HandleLogout(const HttpObjs& hs);
    void HandleProfile(const HttpObjs& hs);
    

    void HandleTransport(const HttpObjs& hs);
    void Download(const HttpObjs& hs, const std::string& path);
    void Upload(const HttpObjs& hs, const std::string& path);
    void GetRecentMsgs(const HttpObjs& hs);

    const std::string& GetUsername() const { return user_id_; }
    void SetUsername(const std::string& user_id) { user_id_ = user_id; }

  private:
    std::string user_id_;
};
