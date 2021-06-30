
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
    int has_arg;  // req���������� opt���п��� no�����������
    bool is_must; // true��������
    bool is_int;  // ���false����ô����string
    Val val;
    int lower_bound;    // string������Ϊ-1
    int upper_bound;    // string������Ϊ-1
    int default_val;    // string������Ϊ-1
};

int my_daemon(int,int);

int my_handle_option(Choice options[], int opt_size, int argc, char **argv);

void set_nonblock(int);

// ��������/������socket��ָ�����ض˿ڣ���ָ��IP�Ͷ˿ڵ�server
// ִ�е�bind֮�󷵻�
// �޸�serve_addr
// ����socket
int create_client(const char serv_ip[], int serv_port, int myport, sockaddr_in &serv_addr, bool isNonBlock = true);

// ��������/������socket��ָ������IP�Ͷ˿�
// ִ�е�bind֮�󷵻�
// ����socket
int create_server(const char ip[], int port, bool isNonBlock = true);

// ��ӡָ��sock��tcp recv��������С��������(��λ:�ֽ�)
int get_recv_buf(int sock);
// ��ӡָ��sock��tcp send��������С��������(��λ:�ֽ�)
int get_send_buf(int sock);
// ����ָ��sock��tcp recv��������С��(��λ:KB)
void set_recv_buf(int sock, int size);
// ����ָ��sock��tcp send��������С��(��λ:KB)
void set_send_buf(int sock, int size);