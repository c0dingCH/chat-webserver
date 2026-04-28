#include "Redis.h"
#include "Logging.h"
#include "MysqlPool.h"
#include "RedisPool.h"
#include "Mysql.h"
#include <sw/redis++/redis++.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <iostream>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <random>
#include <sstream>

namespace {
const std::unordered_set<std::string> kValidTables = {"users", "msgs"};

const std::unordered_map<std::string, std::unordered_set<std::string>> kTableFields = {
    {"users", {"user_id", "password"}},
    {"msgs", {"sender_id", "receiver_id", "content", "date"}}
};

const int kMinTtl = 300;
const int kMaxTtl = 600;

std::string SsGet(const std::string &field_vals, const std::string & field){
  bool flag = 0;
  std::string s;
  std::stringstream ss(field_vals);
  while(ss >> s){
    if(flag)return s;
    if(s == field)flag = 1;
  }
  return "NULL";
}

}  // namespace

bool Redis::ValidateTable(const std::string &table) {
  return kValidTables.find(table) != kValidTables.end();
}

bool Redis::ValidateFields(const std::string &table,
                           const std::vector<std::pair<std::string, std::string>> &fields) {
  auto it = kTableFields.find(table);
  if (it == kTableFields.end()) {
    return false;
  }
  for (const auto &kv : fields) {
    if (it->second.find(kv.first) == it->second.end()) {
      return false;
    }
  }
  return true;
}

std::string Redis::BuildMsgKey(const std::string &user1, const std::string &user2) {
  std::string u1 = user1;
  std::string u2 = user2;
  if (u1 > u2) u1.swap(u2);
  return "msgs:" + u1 + ":" + u2;
}

int Redis::GetRandomTtl() {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(kMinTtl, kMaxTtl);
  return dis(gen);
}

Redis::Redis(MysqlPool* mysql_pool)
  : conn_(std::make_unique<sw::redis::Redis>("tcp://127.0.0.1:6379")),
    mysql_pool_(mysql_pool){}

Redis::~Redis() {}

void Redis::CacheFromMysql(const std::string &table,
                           const std::vector<std::pair<std::string, std::string>> &field_vals,
                           bool found, const std::string &mysql_result) {
  if (!mysql_pool_) {
    return;
  }

  if (table == "users") {
    std::string username;
    for (const auto &kv : field_vals) {
      if (kv.first == "user_id") {
        username = kv.second;
      }
    }
    if (found) {
      conn_->hset("users", username, mysql_result);
    } 
    else {
      conn_->hset("users", username, "NULL");
    }
    conn_->expire("users", std::chrono::seconds(GetRandomTtl()));
  }
}

bool Redis::Insert(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals) {
  std::string msg;
  return Insert(table, field_vals, msg);
}

bool Redis::Insert(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg) {
  if (!ValidateTable(table) || !ValidateFields(table, field_vals)) {
    msg = "Invalid table or fields";
    return false;
  }
  if (table == "users") {
    std::string username;
    std::string password;
    for (const auto &kv : field_vals) {
      if (kv.first == "user_id") {
        username = kv.second;
      } else if (kv.first == "password") {
        password = kv.second;
      }
    }
    if (mysql_pool_) {
      bool success = false;
      mysql_pool_->WithConnection([&](std::unique_ptr<Mysql> &db) {
        success = db->Insert(table, field_vals, msg);
      });
      if (!success) {
        return false;
      }
    }

    std::lock_guard<std::mutex> lock(mtx_);
    conn_->hset("users", username, password);
    msg = "success";
    return true;
  }

  if (table == "msgs") {
    std::string sender_id;
    std::string receiver_id;
    std::string content;
    std::string date;
    for (const auto &kv : field_vals) {
      if (kv.first == "sender_id") {
        sender_id = kv.second;
      } else if (kv.first == "receiver_id") {
        receiver_id = kv.second;
      } else if (kv.first == "content") {
        content = kv.second;
      } else if (kv.first == "date") {
        date = kv.second;
      }
    }

    if (mysql_pool_) {
      bool success = false;
      mysql_pool_->WithConnection([&](std::unique_ptr<Mysql> &db) {
        success = db->Insert(table, field_vals, msg);
      });
      if (!success) {
        return false;
      }
    }

    std::lock_guard<std::mutex> lock(mtx_);
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    doc.AddMember("sender_id", rapidjson::Value(sender_id.c_str(), allocator), allocator);
    doc.AddMember("content", rapidjson::Value(content.c_str(), allocator), allocator);
    doc.AddMember("date", rapidjson::Value(date.c_str(), allocator), allocator);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    std::string key = BuildMsgKey(sender_id, receiver_id);
    conn_->lpush(key, buffer.GetString());
    conn_->expire(key, std::chrono::seconds(GetRandomTtl()));
    msg = "success";
    return true;
  }

  msg = "Invalid table";
  return false;
}

bool Redis::Select(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg) {
  if (!ValidateTable(table) || !ValidateFields(table, field_vals)) {
    msg = "Invalid table or fields";
    return false;
  }

  std::lock_guard<std::mutex> lock(mtx_);

  if (table == "users") {
    std::string username;
    for (const auto &kv : field_vals) {
      if (kv.first == "user_id") {
        username = kv.second;
      }
    }
    auto result = conn_->hget("users", username);

    if (!result && mysql_pool_) {
      std::string mysql_result;
      bool found = false;
      mysql_pool_->WithConnection([&](std::unique_ptr<Mysql> &db) {
        found = db->Select("users", field_vals, mysql_result);
      });
     
      
      CacheFromMysql("users", field_vals, found, SsGet(mysql_result, "password"));
      result = conn_->hget("users", username);
    }

    if(*result == "NULL")return false;
    msg = "password " + *result;
    return true;
  }

  msg = "Table not supported for select";
  return false;
}

bool Redis::Delete(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals) {
  std::string msg;
  return Delete(table, field_vals, msg);
}

bool Redis::Delete(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg) {
  if (!ValidateTable(table) || !ValidateFields(table, field_vals)) {
    msg = "Invalid table or fields";
    return false;
  }

  if (table == "users") {
    std::string username;
    for (const auto &kv : field_vals) {
      if (kv.first == "user_id") {
        username = kv.second;
      }
    }

    if (mysql_pool_) {
      bool success = false;
      mysql_pool_->WithConnection([&](std::unique_ptr<Mysql> &db) {
        success = db->Delete("users", field_vals, msg);
      });
      if (!success) {
        return false;
      }
    }

    std::lock_guard<std::mutex> lock(mtx_);
    conn_->hdel("users", username);
    msg = "success";
    return true;
  }

  msg = "Table not supported for delete";
  return false;
}

bool Redis::Update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals,
                   const std::vector<std::pair<std::string, std::string>>& new_field_vals) {
  std::string msg;
  return Update(table, field_vals, new_field_vals, msg);
}

bool Redis::Update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals,
                   const std::vector<std::pair<std::string, std::string>>& new_field_vals, std::string& msg) {
  if (!ValidateTable(table) || !ValidateFields(table, field_vals) || !ValidateFields(table, new_field_vals)) {
    msg = "Invalid table or fields";
    return false;
  }

  if (table == "users") {
    std::string username;
    std::string new_password;
    for (const auto &kv : field_vals) {
      if (kv.first == "user_id") {
        username = kv.second;
      }
    }
    for (const auto &kv : new_field_vals) {
      if (kv.first == "password") {
        new_password = kv.second;
      }
    }

    if (mysql_pool_) {
      bool success = false;
      mysql_pool_->WithConnection([&](std::unique_ptr<Mysql> &db) {
        success = db->Update("users", field_vals, new_field_vals, msg);
      });
      if (!success) {
        return false;
      }
    }

    std::lock_guard<std::mutex> lock(mtx_);
    conn_->hset("users", username, new_password);
    msg = "success";
    return true;
  }

  msg = "Table not supported for update";
  return false;
}

std::vector<std::string> Redis::GetRecentMsgs(std::string sender_id, std::string receiver_id, int n, std::string& msg) {
  std::vector<std::string> results;
  
  if (n <= 0) {
    msg = "invalid n";
    return {};
  }
  
  std::lock_guard<std::mutex> lock(mtx_);

  std::string key = BuildMsgKey(sender_id, receiver_id);
  auto len = conn_->llen(key);
  

  if(len > 0 && *conn_->lindex(key, len - 1) == "NULL"){
    if(0 <= len - 2)conn_->lrange(key, 0, len - 2, std::back_inserter(results));
    return results;
  }

  if(len >= n){
    conn_->lrange(key, 0, n - 1, std::back_inserter(results));
    return results;
  }

  if (!mysql_pool_) {
    msg = "no mysql pool";
    return {};
  }

  mysql_pool_->WithConnection([&](std::unique_ptr<Mysql>& db) {
    results = db->SelectMsgs(sender_id, receiver_id, n, msg);
  });

  if (!results.empty()) {
    conn_->rpush(key, results.begin() + len , results.end());
    conn_->expire(key, std::chrono::seconds(GetRandomTtl()));  
    if(results.size() < static_cast<size_t>(n)){
      conn_->rpush(key, "NULL");
    }
  }
  
  return results;
}
