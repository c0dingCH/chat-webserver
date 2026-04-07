
#include "HttpRequest.h"

#include <string>
#include <map>
#include <iostream>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

HttpRequest::HttpRequest() : method_(kInvalid), version_(kUnknown){
};

HttpRequest::~HttpRequest(){};

void HttpRequest::SetVersion(const std::string & ver){
  if(ver == "1.0"){
    version_ = Version::kHttp10;
  }else if(ver == "1.1"){
    version_ = Version::kHttp11;
  }else if(ver == "2.0"){
    version_ = Version::kHttp20;
  }else if(ver == "3.0"){
    version_ = Version::kHttp30;
  }else{
    version_ = Version::kUnknown;
  }
}

HttpRequest::Version HttpRequest::GetVersion() const{
  return version_;
}

std::string HttpRequest::GetVersionString() const{
  std::string ver;
  if (version_ == Version::kHttp10)
  {
    ver = "http1.0";
  }
  else if (version_ == Version::kHttp11){
    ver = "http1.1";
  }else{
    ver = "unknown";
  }
  return ver;
}

bool HttpRequest::SetMethod(const std::string &_method){
  if(_method == "GET"){
    method_ = Method::kGet;
  }else if(_method == "POST"){
    method_ =  Method::kPost;
  }else if(_method == "HEAD"){
    method_ = Method::kHead;
  }else if(_method == "PUT"){
    method_ = Method::kPut;
  }else if(_method == "DELETE"){
    method_ = Method::kDelete;
  }else{
    method_ = Method::kInvalid;
  }
  return  method_ != Method::kInvalid;
}

HttpRequest::Method HttpRequest::GetMethod() const{
  return method_;
}
std::string HttpRequest::GetMethodString() const{
  std::string _method;
  if (method_ == Method::kGet)
  {
    _method = "GET";
  }
  else if (method_ == Method::kPost)
  {
    _method = "POST";
  }
  else if (method_ == Method::kHead){
    _method =  "HEAD";
  }
  else if(method_ == Method::kPut){
    _method = "PUT";
  }
  else if (method_ == Method::kDelete)
  {
    _method =  "DELETE";
  }else{
    _method = "INVALID";
  }
  return _method;
}

void HttpRequest::SetUrl(const std::string &url){
  url_ = std::move(url);
}
const std::string & HttpRequest::GetUrl() const{
  return url_;
}

void HttpRequest::SetRequestParams(const std::string &key, const std::string &value){
  request_params_[key] = value;
}
std::string HttpRequest::GetRequestValue(const std::string &key) const{
  std::string ret;
  auto it = headers_.find(key);
  return it == headers_.end() ? ret : it->second;
}
const std::map<std::string, std::string> & HttpRequest::GetRequestParams() const{
  return request_params_;
}

void HttpRequest::SetProtocol(const std::string &str){
  protocol_ = std::move(str);
}
const std::string & HttpRequest::GetProtocol() const{
  return protocol_;
}

void HttpRequest::AddHeader(const std::string &field, const std::string &value){
    headers_[field] = value;
}
std::string HttpRequest::GetHeader(const std::string &field) const{
  std::string result;
  auto it = headers_.find(field);
  if(it!=headers_.end()){
    result = it->second;
  }
  return result;
}
const std::map<std::string, std::string> & HttpRequest::GetHeaders() const{
  return headers_;
}


void HttpRequest::SetBody(const std::string &str){
  body_ = std::move(str);
}
const std::string & HttpRequest::GetBody() const{
  return body_;
}


//void HttpRequest::AddBody(const std::string &key, const std::string & value){
//  bodys_[key] = value;
//}
//const std::string HttpRequest::GetBody(const std::string &key) const{
//  std::string result;
//  auto it = bodys_.find(key);
//  if(it!=headers_.end()){
//    result = it->second;
//  }
//  return result;
//}


void HttpRequest::ParseJson2Dom(){
  dom_.Parse(body_.c_str());
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
