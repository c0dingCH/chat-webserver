#include "Mysql.h"
#include "Logging.h"
#include <mysql/mysql.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

namespace {
const std::unordered_set<std::string> kValidTables = {"users", "msgs"};

const std::unordered_map<std::string, std::unordered_set<std::string>> kTableFields = {
    {"users", {"user_id", "password", "date"}},
    {"msgs", {"rowadd", "sender_id", "receiver_id", "content", "date"}}
};

bool BuildWhereClause(const std::vector<std::pair<std::string, std::string>> &field_vals,
                      std::string &where_clause, std::vector<std::string> &values) {
  for (size_t i = 0; i < field_vals.size(); ++i) {
    if (i > 0) {
      where_clause += " AND ";
    }
    where_clause += field_vals[i].first + " = ?";
    values.push_back(field_vals[i].second);
  }
  return true;
}

bool BuildSetClause(const std::vector<std::pair<std::string, std::string>> &field_vals,
                    std::string &set_clause, std::vector<std::string> &values) {
  for (size_t i = 0; i < field_vals.size(); ++i) {
    if (i > 0) {
      set_clause += ", ";
    }
    set_clause += field_vals[i].first + " = ?";
    values.push_back(field_vals[i].second);
  }
  return true;
}

bool BuildFieldList(const std::vector<std::pair<std::string, std::string>> &field_vals,
                   std::string &fields, std::string &placeholders, std::vector<std::string> &values) {
  for (size_t i = 0; i < field_vals.size(); ++i) {
    if (i > 0) {
      fields += ", ";
      placeholders += ", ";
    }
    fields += field_vals[i].first;
    placeholders += "?";
    values.push_back(field_vals[i].second);
  }
  return true;
}
bool ExecutePrepared(MYSQL *conn, const std::string &sql,
                     const std::vector<std::string> &params, std::string &msg) {
  MYSQL_STMT *stmt = mysql_stmt_init(conn);
  if (!stmt) {
    msg = "stmt init failed";
    return false;
  }

  if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length())) {
    msg = "stmt prepare failed: " + std::string(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  if (params.empty()) {
    if (mysql_stmt_execute(stmt)) {
      msg = "stmt execute failed: " + std::string(mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return false;
    }

    mysql_stmt_close(stmt);
    msg = "success";
    return true;
  }

  unsigned int param_count = mysql_stmt_param_count(stmt);
  if (param_count != params.size()) {
    msg = "param count mismatch";
    mysql_stmt_close(stmt);
    return false;
  }

  std::vector<MYSQL_BIND> binds(param_count);
  std::vector<std::string> param_bufs(param_count);

  for (size_t i = 0; i < param_count; ++i) {
    param_bufs[i] = params[i];
    binds[i].buffer_type = MYSQL_TYPE_STRING;
    binds[i].buffer = const_cast<char*>(param_bufs[i].data());
    binds[i].buffer_length = param_bufs[i].length();
  }

  if (mysql_stmt_bind_param(stmt, binds.data())) {
    msg = "stmt bind failed: " + std::string(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  if (mysql_stmt_execute(stmt)) {
    msg = "stmt execute failed: " + std::string(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  mysql_stmt_close(stmt);
  msg = "success";
  return true;
}

bool ExecutePreparedSelect(MYSQL *conn, const std::string &sql,
                         const std::vector<std::string> &params, std::string &msg) {
  MYSQL_STMT *stmt = mysql_stmt_init(conn);
  if (!stmt) {
    msg = "stmt init failed";
    return false;
  }

  if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length())) {
    msg = "stmt prepare failed: " + std::string(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  unsigned int param_count = mysql_stmt_param_count(stmt);
  if (params.size() > 0 && param_count != params.size()) {
    msg = "param count mismatch";
    mysql_stmt_close(stmt);
    return false;
  }

  if (param_count > 0) {
    std::vector<MYSQL_BIND> binds(param_count);
    std::vector<std::string> param_bufs(param_count);

    for (size_t i = 0; i < param_count; ++i) {
      param_bufs[i] = params[i];
      binds[i].buffer_type = MYSQL_TYPE_STRING;
      binds[i].buffer = const_cast<char*>(param_bufs[i].data());
      binds[i].buffer_length = param_bufs[i].length();
    }

    if (mysql_stmt_bind_param(stmt, binds.data())) {
      msg = "stmt bind failed: " + std::string(mysql_stmt_error(stmt));
      mysql_stmt_close(stmt);
      return false;
    }
  }

  if (mysql_stmt_execute(stmt)) {
    msg = "stmt execute failed: " + std::string(mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    return false;
  }

  MYSQL_RES *result = mysql_stmt_result_metadata(stmt);
  if (!result) {
    msg = "no result set";
    mysql_stmt_close(stmt);
    return false;
  }

  int col_count = mysql_num_fields(result);
  MYSQL_FIELD *db_fields = mysql_fetch_fields(result);

  std::vector<MYSQL_BIND> binds(col_count);
  std::vector<std::string> col_bufs(col_count);
  std::vector<unsigned long> col_lengths(col_count);

  for (int i = 0; i < col_count; ++i) {
    size_t n = 50;
    if(i == 2) n = 2020;
    col_bufs[i].resize(n);
    binds[i].buffer_type = MYSQL_TYPE_STRING;
    binds[i].buffer = const_cast<char*>(col_bufs[i].data());
    binds[i].buffer_length = n;
    binds[i].length = &col_lengths[i];
    binds[i].is_null = nullptr;
  }

  if (mysql_stmt_bind_result(stmt, binds.data())) {
    msg = "stmt bind result failed: " + std::string(mysql_stmt_error(stmt));
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    return false;
  }

  if (mysql_stmt_store_result(stmt)) {
    msg = "stmt store result failed: " + std::string(mysql_stmt_error(stmt));
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    return false;
  }

  std::string result_str;
  while (true) {
    int ret = mysql_stmt_fetch(stmt);
    if (ret == MYSQL_NO_DATA) {
      break;
    }
    if (ret != 0) {
      if (ret == 1) {
        msg = "stmt fetch failed: " + std::string(mysql_stmt_error(stmt));
      } else {
        msg = "stmt fetch failed";
      }
      mysql_free_result(result);
      mysql_stmt_close(stmt);
      return false;
    }
    for (int i = 0; i < col_count; ++i) {
      if (i > 0) {
        result_str += " ";
      }
      std::string field_name = db_fields[i].name;
      std::string field_value = col_lengths[i] > 0 ? col_bufs[i].substr(0, col_lengths[i]) : "NULL";
      result_str += field_name + " " + field_value;
    }
    break;
  }

  if (result_str.empty()) {
    msg = "Record not found";
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    return false;
  }

  msg = std::move(result_str);
  mysql_free_result(result);
  mysql_stmt_close(stmt);
  return true;
}

}  // namespace

bool Mysql::ValidateTable(const std::string &table) { //table legal
  return kValidTables.find(table) != kValidTables.end();
}

bool Mysql::ValidateFields(const std::string &table,
                           const std::vector<std::pair<std::string, std::string>> &fields) { // field legal
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

Mysql::Mysql() {
  mysql_init(&conn_);
  if (!mysql_real_connect(&conn_, "localhost", "chat", "chat123", "chat_db", 3306, NULL, 0)) {
    LOG_ERROR << "db connect error!";
  }
}

Mysql::~Mysql() {
  mysql_close(&conn_);
}

bool Mysql::Insert(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals) {
  std::string msg;
  return Insert(table, field_vals, msg);
}

bool Mysql::Insert(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg) {
  if (!ValidateTable(table) || !ValidateFields(table, field_vals)) {
    msg = "Invalid table or fields";
    return false;
  }

  if (Select(table, field_vals, msg)) {
    msg = "Record already exists";
    return false;
  }

  std::string fields;
  std::string placeholders;
  std::vector<std::string> values;
  BuildFieldList(field_vals, fields, placeholders, values);

  std::string sql = "INSERT INTO " + table + " (" + fields + ") VALUES (" + placeholders + ")";
  return ExecutePrepared(&conn_, sql, values, msg);
}

bool Mysql::Select(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg) {
  if (!ValidateTable(table) || !ValidateFields(table, field_vals)) {
    msg = "Invalid table or fields";
    return false;
  }

  std::string where_clause;
  std::vector<std::string> values;
  BuildWhereClause(field_vals, where_clause, values);

  std::string sql = "SELECT * FROM " + table + " WHERE " + where_clause;
  return ExecutePreparedSelect(&conn_, sql, values, msg);
}

bool Mysql::Delete(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals) {
  std::string msg;
  return Delete(table, field_vals, msg);
}

bool Mysql::Delete(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals, std::string& msg) {
  if (!ValidateTable(table) || !ValidateFields(table, field_vals)) {
    msg = "Invalid table or fields";
    return false;
  }

  if (!Select(table, field_vals, msg)) {
    msg = "Record not found";
    return false;
  }

  std::string where_clause;
  std::vector<std::string> values;
  BuildWhereClause(field_vals, where_clause, values);

  std::string sql = "DELETE FROM " + table + " WHERE " + where_clause;
  return ExecutePrepared(&conn_, sql, values, msg);
}

bool Mysql::Update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals,
                   const std::vector<std::pair<std::string, std::string>>& new_field_vals) {
  std::string msg;
  return Update(table, field_vals, new_field_vals, msg);
}

bool Mysql::Update(const std::string& table, const std::vector<std::pair<std::string, std::string>>& field_vals,
                   const std::vector<std::pair<std::string, std::string>>& new_field_vals, std::string& msg) {
  if (!ValidateTable(table) || !ValidateFields(table, field_vals) || !ValidateFields(table, new_field_vals)) {
    msg = "Invalid table or fields";
    return false;
  }

  if (!Select(table, field_vals, msg)) {
    msg = "Record not found";
    return false;
  }

  std::string where_clause;
  std::string set_clause;
  std::vector<std::string> values;

  BuildSetClause(new_field_vals, set_clause, values);
  BuildWhereClause(field_vals, where_clause, values);

  std::string sql = "UPDATE " + table + " SET " + set_clause + " WHERE " + where_clause;
  return ExecutePrepared(&conn_, sql, values, msg);
}

std::vector<std::string> Mysql::SelectMsgs(const std::string& sender_id, const std::string& receiver_id, int n, std::string& msg) {
  std::vector<std::string> results;

  std::string u1 = sender_id;
  std::string u2 = receiver_id;
  if (u1 > u2) {
    u1.swap(u2);
  }

  std::string sql = "SELECT sender_id, content, date FROM msgs WHERE "
                  "(sender_id = ? AND receiver_id = ?) OR "
                  "(sender_id = ? AND receiver_id = ?) "
                  "ORDER BY date DESC LIMIT ?";

  MYSQL_STMT* stmt = mysql_stmt_init(&conn_);
  if (!stmt) {
    msg = "stmt init failed";
    return results;
  }

  if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length())) {
    msg = "stmt prepare failed";
    mysql_stmt_close(stmt);
    return results;
  }

  std::vector<std::string> params = {u1, u2, u2, u1, std::to_string(n)};
  unsigned int param_count = mysql_stmt_param_count(stmt);
  if (param_count != params.size()) {
    msg = "param count mismatch";
    mysql_stmt_close(stmt);
    return results;
  }

  std::vector<MYSQL_BIND> binds(param_count);
  std::vector<std::string> param_bufs(param_count);
  for (size_t i = 0; i < param_count; ++i) {
    param_bufs[i] = params[i];
    binds[i].buffer_type = MYSQL_TYPE_STRING;
    binds[i].buffer = const_cast<char*>(param_bufs[i].data());
    binds[i].buffer_length = param_bufs[i].length();
  }

  if (mysql_stmt_bind_param(stmt, binds.data())) {
    msg = "stmt bind failed";
    mysql_stmt_close(stmt);
    return results;
  }

  if (mysql_stmt_execute(stmt)) {
    msg = "stmt execute failed";
    mysql_stmt_close(stmt);
    return results;
  }

  MYSQL_RES* result_meta = mysql_stmt_result_metadata(stmt);
  if (!result_meta) {
    msg = "no result set";
    mysql_stmt_close(stmt);
    return results;
  }

  int col_count = mysql_num_fields(result_meta);

  std::vector<MYSQL_BIND> result_binds(col_count);
  std::vector<std::string> col_bufs(col_count);
  std::vector<unsigned long> col_lengths(col_count);

  for (int i = 0; i < col_count; ++i) {
    size_t n = 50;
    if(i == 2) n = 2020;
    col_bufs[i].resize(n);
    result_binds[i].buffer_type = MYSQL_TYPE_STRING;
    result_binds[i].buffer = const_cast<char*>(col_bufs[i].data());
    result_binds[i].buffer_length = n;
    result_binds[i].length = &col_lengths[i];
  }

  if (mysql_stmt_bind_result(stmt, result_binds.data())) {
    msg = "stmt bind result failed";
    mysql_free_result(result_meta);
    mysql_stmt_close(stmt);
    return results;
  }

  if (mysql_stmt_store_result(stmt)) {
    msg = "stmt store result failed";
    mysql_free_result(result_meta);
    mysql_stmt_close(stmt);
    return results;
  }

  rapidjson::Document doc;
  while (true) {
    int ret = mysql_stmt_fetch(stmt);
    if (ret == MYSQL_NO_DATA) {
      break;
    }
    if (ret != 0) {
      msg = "stmt fetch failed";
      break;
    }

    std::string sender = col_bufs[0].substr(0, col_lengths[0]);
    std::string content = col_bufs[1].substr(0, col_lengths[1]);
    std::string date = col_bufs[2].substr(0, col_lengths[2]);

    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();
    doc.AddMember("sender_id", rapidjson::Value(sender.c_str(), allocator), allocator);
    doc.AddMember("content", rapidjson::Value(content.c_str(), allocator), allocator);
    doc.AddMember("date", rapidjson::Value(date.c_str(), allocator), allocator);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    results.emplace_back(buffer.GetString());
    
  }

  mysql_free_result(result_meta);
  mysql_stmt_close(stmt);
  msg = "success";
  return results;
}
