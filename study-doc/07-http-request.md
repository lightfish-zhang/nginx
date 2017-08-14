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
- 创建套接字，在Nginx主进程做好，在fork之前，这样工作进程都会继承已经初始化好的监听套接口
    + 遍历`cycle->listening`数组，根据用户配置而赋予不同的默认特性，比如收包/发包缓存区大小，
    + `cycle->listening`数组的每一个ls元素，其fd就是一个可用的监听套接口描述符

- worker进程初始化和调用过程
    + 初始化函数`ngx_worker_process_cycle()`->`ngx_worker_process_init()`->`ngx_event_process_init()`
    + 在`ngx_event_process_init()`中，使用到了全局变量`ngx_use_accept_mutex`，当Nginx开启多进程负载均衡，该变量赋值1
    + 在`ngx_event_process_init()`中，对每一个监听套接口创建对于的`connection`连接对象，且事件的回调函数设置为`ngx_event_accept()`
```c
/* ngx_event_process_init() */
    for (i = 0; i < cycle->listening.nelts; i++) {
        c = ngx_get_connection(ls[i].fd, cycle->log);
        /* ... */
        rev->handler = ngx_event_accept;
    }
```

    + 工作进程的主要执行体`ngx_process_cycle.c`的`ngx_worker_process_cycle()`中主要是一个无限for循环，最重要的函数是`ngx_process_events_and_timers()`

### worker进程获取socket的客户端连接

```c
/* ngx_event.c  ngx_process_events_and_timers() 部分代码 */
if (ngx_use_accept_mutex) {
    if (ngx_accept_disabled > 0) {
        ngx_accept_disabled--;

    } else {
        if (ngx_trylock_accept_mutex(cycle) == NGX_ERROR) {
            return;
        }

        if (ngx_accept_mutex_held) {
            flags |= NGX_POST_EVENTS;

        } else {
            if (timer == NGX_TIMER_INFINITE
                || timer > ngx_accept_mutex_delay)
            {
                timer = ngx_accept_mutex_delay;
            }
        }
    }
}
delta = ngx_current_msec;
(void) ngx_process_events(cycle, timer, flags);
```

```c
/* ngx_event_accept.c  ngx_event_accept() 相关代码 */
ngx_accept_disabled = ngx_cycle->connection_n / 8
                        - ngx_cycle->free_connection_n;
```

- 多进程对一个connection的可读事件的抢占
    + worker进程抢占`accept_mutex`锁
    + 互斥锁`ngx_trylock_accept_mutex()`等相关函数，通过Nginx的共享内存机制实现
    + 抢到锁的进程，会给`flag`打上`NGX_POST_EVENTS`标记，将大部分事件延迟到释放锁之后再去处理，把锁尽快释放，缩短自身持有锁的时间
    + 没签到锁的进程，会把事件监控机制阻塞点如`epoll_wait()`的超时时间`timer`限制为较短的范围，默认500ms, 配置项`accept_mutex_delay`，这样，频繁地从阻塞跳出来去争抢互斥锁
    
- 抢占`accept_mutex`后，执行的堆栈：`ngx_process_events()`->`ngx_epoll_process_events()`->`epoll_wait()`->`rev->handler(rev)`(上文提到的`ngx_event_accept()`)

- `ngx_event_accept()`函数分析
    + 判断`ngx_accept_disabled`值是否大于0，判断当前进程是否过载
    + `ngx_cycle->connection_n` 表示一个工作进程的最大承受连接数，配置项是`worker_connections`