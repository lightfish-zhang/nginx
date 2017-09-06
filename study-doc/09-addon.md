## 编写Nginx插件

本篇文章以Nginx的插件helloworld为例子

### 文件结构

- 在Nginx源代码目录下，新建一个`ext`目录

```
./ext
└── helloworld
    ├── config
    └── ngx_http_helloworld_module.c
```

### 简单的代码示例

- 参考`http`模块，注意关键的地方
    + 配置项
    + 处理缓存
    + 按照http协议，返回header与body

### 编译与测试

- 配置，编译

```
./auto/configure --add-module=ext/helloworld
make
```

- 调试就不执行`make install`安装了，直接跑，注意指定配置文件时使用绝对路径

```
sudo ./objs/nginx -c `pwd`/ext/helloworld/example.conf
```

- 配置文件的配置项

```
        location / {
            helloworld;
        }

```

- 访问

```
curl -i http://localhost:8000 
HTTP/1.1 200 OK
Server: nginx/1.2.9
Date: Wed, 06 Aug 2017 06:40:50 GMT
Content-Type: text/html
Content-Length: 13
Connection: keep-alive

Hello, World!
```