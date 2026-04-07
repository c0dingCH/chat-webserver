> 简介：基于linux网络库开发chat软件， tcp + http2.0 + mysql8 + redies + webSocket + 客户端 + docker(maybe用来部署)
#chat项目正在进行中， 以下为网络层的介绍



### 主架构  
**Tcp** ： Reactor 主从模式 ， 事件驱动模型    

多 Reactor 多进程 / 线程：  
<img width="720" height="513" alt="image" src="https://github.com/user-attachments/assets/6b7e90ed-ee26-4d28-81d7-29b8bcf47c27" />


**工具**：
  - 计时器 ：timer
  - 日志（前端同步，后端异步）： log
  - Cmake(简易的动/静态连接)
  - git(后期使用)
  
**简易http实现**： 静态文件的上传下载、get静态文件内容、简易账号密码的check

### 项目说明：
- 参考 Muduo 架构，采用主从 Reactor 模式 + 线程池，实现高并发请求处理
- 封装 Socket、Channel、EventLoop、Connection 核心类，构建事件驱动架构
- 非阻塞 IO+ET 边缘触发，减少系统调用开销，提升并发性能
- Connection 生命周期管理：使用 std::shared_ptr 管理连接对象，结合 enable_shared_from_this 保证在回调
- 中安全获取自身指针，避免悬空指针与内存泄漏
- 回调函数机制：通过回调函数解耦网络事件与业务逻辑，实现新连接、读写、关闭等事件的灵活处理
- 定时器管理：设计基于小根堆或std::set的定时器管理空闲连接，自动剔除超时客户端，有效防止内存与文件描述符泄露
- Buffer 封装：统一管理读写缓冲区，支持高效的数据拼接与分片处理
- 支持 HTTP 协议解析与响应，实现静态文件服务、POST 请求处理、文件上传下载功能
- 构建 日志系统（同步/异步），提升服务器稳定性与可维护性
  
技术栈：C++、Linux、epoll、Reactor、线程池、智能指针、回调函数、STL、HTTP 协议、日志系统




### 压力测试截图

<img width="1034" height="171" alt="c7b69ae6103d71d4ba65084e51ff163b" src="https://github.com/user-attachments/assets/89487de8-0596-48a9-bd82-49197aafe5e3" />
