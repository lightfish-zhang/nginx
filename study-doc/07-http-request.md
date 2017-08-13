# http请求与处理

## 配置文件解析

配置文件

```
http{
    server{
        listen 80;
        server_name www.xxx.com;
    }
}
```

在`ngx_http_core_module.c`对应的配置文件解析的结构体

```c
{ ngx_string("http"),
    NGX_MAIN_CONF|NGX_CONF_BLOCK|NGX_CONF_NOARGS,
    ngx_http_block,
    0,
    0,
    NULL },

    ngx_null_command
};
{ ngx_string("listen"),
    NGX_HTTP_SRV_CONF|NGX_CONF_1MORE, // 在server块内|一个以上的token
    ngx_http_core_listen,             // 配置指令处理回调函数
    NGX_HTTP_SRV_CONF_OFFSET,         // 主要由NGX_HTTP_MODULE类型模块所使用，其指定当前配置项所在的大致位置
    0,
    NULL },
{ ngx_string("server_name"),
    NGX_HTTP_SRV_CONF|NGX_CONF_1MORE,
    ngx_http_core_server_name,
    NGX_HTTP_SRV_CONF_OFFSET,
    0,
    NULL },
```

### 配置项解析过程

- 以上配置的回调函数在`ngx_http.c`
- `listen`配置项的解析之路: `ngx_http_core_listen()`->`ngx_http_add_listen()`->`ngx_http_add_address()`->`ngx_http_add_server()`
- 当Nginx的http配置块解析完毕，在`ngx_http_block()`最后会调用`ngx_http_optimize_servers()`创建对应的监听套接口的"结构体"，未真正创建套接口，结构体`ngx_listening_t`放在全局变量`cycle->listening`
- 真正创建套接口以及套接口选项操作在`ngx_connection.c`
    + 函数是`ngx_open_listening_sockets()`和`ngx_configure_listening_sockets()`
    + 上两个函数在Nginx主进程初始化`ngx_init_cycle()`结尾处调用
    + 创建套接字的`ngx_socket()`，根据宏定义看，unix环境下调用`socket()`, win32环境下调用`WSASocket()`
    + 以unix环境下，创建套接字以及套接口选项设置的套路是: `socket()`->`setsocketopt()`->`bind()`->`listen()`

## work进程监听socket

- 关键问题是避免竞争与负载均衡

