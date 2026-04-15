
#include "HttpRequest.h"
#include "Buffer.h"

#include <string>
#include <map>
#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

HttpRequest::HttpRequest(int stream_id)
: status_(HttpRequestStatus::kStart),
  stream_id_(stream_id)
{
  buffer_ = std::make_unique<Buffer>();  
};

HttpRequest::~HttpRequest(){};


void HttpRequest::AddHeader(const std::string &key, const std::string &value){
  headers_[key] = value;
}

std::string HttpRequest::GetHeader(const std::string &field) const{
  std::string result;
  auto it = headers_.find(field);
  if(it!=headers_.end()){
    result = it->second;
  }
  return result;
}

void HttpRequest::Append(const uint8_t * data, size_t len){
  buffer_->Append(reinterpret_cast<const char *>(data), len);
}

std::string HttpRequest::GetData(){
  return buffer_ -> RetrieveAllAsString();
}

void HttpRequest::SetRequestStatus(HttpRequestStatus status){
  status_ = status;
}

HttpRequest::HttpRequestStatus HttpRequest::GetRequestStatus(){
  return status_;
}


//json

void HttpRequest::ParseJson2Dom(){
  int len = buffer_->readablebytes();
  dom_.Parse(buffer_->RetrieveAllAsString().c_str(), len);
}

std::string HttpRequest::ParseJson2String(){
  return ParseJson2String(dom_);
}

std::string HttpRequest::ParseJson2String(rapidjson::Document & dom){
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  dom.Accept(writer);
  return buffer.GetString();
}

rapidjson::Document& HttpRequest::GetDom(){
  return dom_;
}


