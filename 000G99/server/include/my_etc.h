
const int req_arg = 1;
const int opt_arg = 2;
const int no_arg = 3;
const bool must_appear = true;
const bool not_must = false;
const bool is_int = true;
const bool is_string = false;

union Val{
    int *int_val;
    char *str_val;
};

struct Choice{
    const char *name;
    int has_arg;  // req则必须带参数 opt可有可无 no不允许带参数
    bool is_must; // true则必须出现
    bool is_int;  // 如果false，那么就是string
    Val val;
    int lower_bound;    // string类型则为-1
    int upper_bound;    // string类型则为-1
    int default_val;    // string类型则为-1
};

int my_daemon(int,int);

int my_handle_option(Choice options[], int opt_size, int argc, char **argv);

void set_nonblock(int);

// 创建阻塞/非阻塞socket，指定本地端口，绑定指定IP和端口的server
// 执行到bind之后返回
// 修改serve_addr
// 返回socket
int create_client(const char serv_ip[], int serv_port, int myport, sockaddr_in &serv_addr, bool isNonBlock = true);

// 创建阻塞/非阻塞socket，指定本地IP和端口
// 执行到bind之后返回
// 返回socket
int create_server(const char ip[], int port, bool isNonBlock = true);

// 打印指定sock的tcp recv缓冲区大小，并返回(单位:字节)
int get_recv_buf(int sock);
// 打印指定sock的tcp send缓冲区大小，并返回(单位:字节)
int get_send_buf(int sock);
// 设置指定sock的tcp recv缓冲区大小，(单位:KB)
void set_recv_buf(int sock, int size);
// 设置指定sock的tcp send缓冲区大小，(单位:KB)
void set_send_buf(int sock, int size);