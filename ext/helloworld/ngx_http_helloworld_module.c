/*
 * Nginx hello world module
 *
 */
 
 #include <ngx_config.h>
 #include <ngx_core.h>
 #include <ngx_http.h>
 //采用Nginx的模块命名规范
 u_char ngx_helloworld_string[] = "Hello, World!";
  
 //声明命令初始化函数
 char* ngx_http_helloworld_setup(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);
  
 static ngx_command_t ngx_http_helloworld_commands[] = {
     {
         ngx_string("helloworld"),
         NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
         ngx_http_helloworld_setup, //设置我们的命令初始函数
         0,
         0,
         NULL
     },
  
     ngx_null_command
 };//实例化命令数组，以ngx_null_command结尾
  
 static ngx_http_module_t ngx_http_helloworld_module_ctx = {
     NULL,                          /* preconfiguration */
     NULL,                          /* postconfiguration */
  
     NULL,                          /* create main configuration */
     NULL,                          /* init main configuration */
  
     NULL,                          /* create server configuration */
     NULL,                          /* merge server configuration */
  
     NULL,                          /* create location configuration */
     NULL                           /* merge location configuration */
 };
 //实例化上下文结构，可以一个函数也不管。
  
 ngx_module_t ngx_http_helloworld_module = {
     NGX_MODULE_V1,
     &ngx_http_helloworld_module_ctx,    /* module context */
     ngx_http_helloworld_commands,       /* module directives */
     NGX_HTTP_MODULE,                    /* module type */
     NULL,                               /* init master */
     NULL,                               /* init module */
     NULL,                               /* init process */
     NULL,                               /* init thread */
     NULL,                               /* exit thread */
     NULL,                               /* exit process */
     NULL,                               /* exit master */
     NGX_MODULE_V1_PADDING
 };//实例化模块对象
  
 //helloworld命令的真正处理函数，参数是ngx_http_request_t即一个请求
 //关于ngx_http_request_t就不具体写出来了，懂HTTP协议的人都了解的。
 //此函数实现的也是基本处理流程
 static ngx_int_t ngx_http_helloworld_handler(ngx_http_request_t *r){
     ngx_int_t    rc;
     ngx_buf_t   *b;
     ngx_chain_t  out;
  
     //完善HTTP头
     /* set the 'Content-type' header */
     r->headers_out.content_type.len = sizeof("text/html") - 1;
     r->headers_out.content_type.data = (u_char *) "text/html";
  
     //分配输出内存空间
     /* allocate a buffer */
     b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
     if (b == NULL) {
         return NGX_HTTP_INTERNAL_SERVER_ERROR;
     }
     //输出缓存附加到输出链表上
     /* attach buffer to the buffer chain */
     out.buf = b;
     out.next = NULL;
  
     //填写输出缓存内容
     /* adjust the pointers of the buffer */
     b->pos = ngx_helloworld_string;  /* the begin offset of the buffer */
     b->last = ngx_helloworld_string + sizeof(ngx_helloworld_string) - 1; /* the end offset of the buffer */
     b->memory = 1;    /* this buffer is in memory */
     b->last_buf = 1;  /* this is the last buffer in the buffer chain */
  
     //还是完善HTTP头
     /* set the status line */
     r->headers_out.status = NGX_HTTP_OK;
     r->headers_out.content_length_n = sizeof(ngx_helloworld_string) - 1;
  
     //输出HTTP头
     /* send the headers of your response */
     rc = ngx_http_send_header(r);
  
     if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
         return rc;
 }
 //输出内容
     /* send the buffer chain of your response */
     return ngx_http_output_filter(r, &out);
 }
  
 //实现helloworld命令的初始化函数，此函数指定命令的真正处理函数为ngx_http_helloworld_handler
 char* ngx_http_helloworld_setup(ngx_conf_t *cf, ngx_command_t *cmd, void *conf){
     ngx_http_core_loc_conf_t *clcf;
  
     clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
     clcf->handler = ngx_http_helloworld_handler; /* handler to process the 'helloworld' directive */
  
     return NGX_CONF_OK;
 }