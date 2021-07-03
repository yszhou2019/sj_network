#include <string>

/* memcpy, memmove */
#include <string.h>

/* pipe */
#include <unistd.h>

/* splice */
#include <fcntl.h>

/* select */
#include <sys/select.h>

/* fstat */
#include <sys/stat.h>

using string = std::string;

const int delay = 2;

/**
 * ���ȴ� delay ��
 */ 
bool sock_ready_to_read(int sock)
{
    fd_set rfds;
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    int res = select(sock + 1, &rfds, NULL, NULL, &tv);
    if(FD_ISSET(sock, &rfds))
    {
        return true;
    }
    printf("socket ��ʱδ������\n");
    return false;
}

bool sock_ready_to_write(int sock)
{
    fd_set wfds;
    timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    int res = select(sock + 1, NULL, &wfds, NULL, &tv);
    if(FD_ISSET(sock, &wfds))
    {
        printf("����\n");
        return true;
    }
    printf("socket ��ʱδд����\n");
    return false;
}

off_t get_file_size(const string& filename)
{
    int fd = open(filename.c_str(), O_RDONLY);
    struct stat buf;
    fstat(fd, &buf);
    close(fd);
    return buf.st_size;
}

/**
 * ��֤socket�������ļ����ڣ��ļ���С���ʵ������
 * ������д��ָ���ļ���
 * ����д���ֽ�����
 */ 
ssize_t write_to_file(int sock, const string& filename, loff_t offset, size_t chunksize)
{

    off_t filesize = get_file_size(filename);
    if(offset + chunksize > filesize)
        return 0;

    int pipefd[2];
    pipe(pipefd);

    // socket -> data -> file
    int fd = open(filename.c_str(), O_WRONLY, 766);
    lseek(fd, offset, SEEK_SET);

    ssize_t cnt = 0;
    size_t size = chunksize;

    while(cnt != chunksize)
    {
        bool ready = sock_ready_to_read(sock);
        // printf("socket %s\n", ready ? "����" : "����");
        if (!ready)
        {
            close(fd);
            close(pipefd[0]);
            close(pipefd[1]);
            // �����˿������ܵ�ʱ�䣬��û���յ��㹻���ֽ��������Ͳ��ٵȴ�
            return 0;
        }

        // int should_trans_bytes = (size > 4000 ? 4000 : size);
        // ssize_t r_bytes = splice(sock, NULL, pipefd[1], NULL, should_trans_bytes, SPLICE_F_MORE | SPLICE_F_MOVE);
        // ssize_t w_bytes = splice( pipefd[0], NULL, fd, &offset, should_trans_bytes, SPLICE_F_MORE | SPLICE_F_MOVE );
        printf("sock ready\n");
        ssize_t r_bytes = splice(sock, NULL, pipefd[1], NULL, size, SPLICE_F_MORE | SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
        printf("read sock to pipe\n");
        ssize_t w_bytes = splice( pipefd[0], NULL, fd, NULL, size, SPLICE_F_MORE | SPLICE_F_MOVE | SPLICE_F_NONBLOCK );
        printf("read pipe to file\n");
        printf("read %ld bytes from sock, write %ld bytes to file\n", r_bytes, w_bytes);

        if(w_bytes == 0)
            break;
        if(w_bytes == -1)
            return cnt;
        cnt += w_bytes;
        offset += w_bytes;
        size -= w_bytes;
    }

    close(fd);
    close(pipefd[0]);
    close(pipefd[1]);
    char temp;
    read(sock, &temp, 1);
    if(temp !='\0')
    {
        printf("read chunks without end_zero\n");
    }
    return cnt;
}

/**
 * ��֤socket�������ļ����ڣ��ļ���С���ʵ������
 * ���ļ��ж�ȡ�ֽڣ����͸�socket
 * ���ط��͵��ֽ�����
 */ 
ssize_t send_to_socket(int sock, const string& filename, loff_t offset, size_t chunksize)
{
    int pipefd[2];
    pipe(pipefd);

    // file -> data -> socket
    int fd = open(filename.c_str(), O_RDONLY, 766);
    lseek(fd, offset, SEEK_SET);

    ssize_t cnt = 0;
    size_t size = chunksize;

    while(cnt != chunksize)
    {
        bool ready = sock_ready_to_write(sock);
        if (!ready)
        {
            close(fd);
            close(pipefd[0]);
            close(pipefd[1]);
            // �����˿������ܵ�ʱ�䣬��û�л��������Է��ͣ��Ͳ��ٵȴ�
            printf("���ͳ���, ׼���˳�\n");
            return 0;
        }

        // int should_trans_bytes = (size > 4000 ? 4000 : size);
        // ssize_t r_bytes = splice(fd, &offset, pipefd[1], NULL, should_trans_bytes, SPLICE_F_MORE | SPLICE_F_MOVE);
        // ssize_t w_bytes = splice(pipefd[0], NULL, sock, NULL, should_trans_bytes, SPLICE_F_MORE | SPLICE_F_MOVE);
        printf("sock ready\n");
        ssize_t r_bytes = splice(fd, NULL, pipefd[1], NULL, size, SPLICE_F_MORE | SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
        printf("read file to pipe\n");
        ssize_t w_bytes = splice(pipefd[0], NULL, sock, NULL, size, SPLICE_F_MORE | SPLICE_F_MOVE | SPLICE_F_NONBLOCK);
        printf("read pipe to socket\n");

        printf("read %ld from file, write %ld bytes to sock\n", r_bytes, w_bytes);
        if(w_bytes == 0)
            break;
        if(w_bytes == -1)
            return cnt;
        cnt += w_bytes;
        offset += w_bytes;
        size -= w_bytes;
    }


    close(fd);
    close(pipefd[0]);
    close(pipefd[1]);
    char temp = '\0';
    write(sock, &temp, 1);
    return cnt;
}

void discard_extra(int sock, size_t chunksize)
{
    bool ready = sock_ready_to_read(sock);
    if (!ready)
    {
        return;
    }
    u_char buffer[2048];
    size_t res = chunksize;
    while(res != 0)
    {
        auto ready = sock_ready_to_read(sock);
        if(!ready)
            return;
        auto should_read_bytes = (res >= 2048 ? 2048 : res);
        auto actural_read = read(sock, buffer, should_read_bytes);
        printf("���� %d bytes\n", actural_read);
        if (actural_read == -1)
            actural_read = 0;
        res -= actural_read;
    }
    printf("׼���˳�\n");
    read(sock, buffer, 1);
}


// const int N = 65536;

// /**
//  * �����class�ĳ����ǣ�server�������ݵ�ʱ�򣬰�����������ݶ�����buffer��Ȼ�����
//  * ���ǿ��ǵ����ܻ������������ݺ��ļ����ݻ���һ��
//  * �ٵ���splice��ʱ�򲻷��㣬��Ҫ�Ȱ�buffer�в���������д��(�ж��߼���Щ���ӣ���ʱ����)��Ȼ����splice
//  */ 
// class Readbuf {
// private:
//     int sock;
//     char buf[N];
// 	int buf_len;

// public:
// 	Readbuf(int _sock):sock(_sock), buf_len(0) {}
// 	/* return len */
//     int read_until(char *b, char del = '\0');
// 	/* return 0 for success */
// 	int readn(char *b, int len);
// 	int write_res_data_to_file(int fd, size_t chunksize);
// };

// /**
//  * ��socket��ȡָ�����ȵ����ݵ�buf�У��ɹ�����0��ʧ�ܷ���-1
//  */ 
// int readn(int sock, char *buf, int len) {
// 	int n = 0, m = 0;
// 	while (n < len) {
// 		bool ready = sock_ready_to_read(sock);
// 		if(!ready)
// 			return -1;

// 		if ((m = read(sock, buf + n, len - n)) < 0) 
// 			return -1;
		
// 		n += m;
// 	}
// 	return 0;
// }

// /**
//  * ��;���ͻ����ϴ��ļ���server������������������ݽ��յ�buffer�У�Ȼ����д�뵽�ļ�(����þ������ķ�ʽ����ȡjson������ô�Ϳ��ܻὫ�����ļ�����buffer��)
//  */ 
// int Readbuf::readn(char *b, int len) {
// 	int n = buf_len;
// 	printf("%d bytes in Readbuf, %d bytes to read", n, len);
// 	// buffer�Ѿ����㹻�������ֽڣ�ֱ��cpy��Ȼ��move����
// 	if (n >= len) {
// 		memcpy(b, buf, len);
// 		memmove(buf, buf + len, n - len);
// 		buf_len -= len;
// 	}
// 	else {
// 	// buffer�е����ݲ������ȿ���һ���֣��ٴ�sock��ֱ�Ӷ�ȡ�������ݵ�b��
// 		int m = len - n;
// 		memcpy(b, buf, n);
// 		buf_len = 0;
// 		if (::readn(sock, b + n, m) < 0)
// 			return -1;
// 	}
// 	return 0;
// }

// /**
//  * TODO ������header֮��buf�п��ܲ����ļ���data���ݣ������Ҫ���Ƚ�buffer�в���������д�뵽�ļ��У�Ȼ���ٵ���write_to_file
//  * �����ļ���chunksize���� buffer �в�����������Ϣд�뵽�ļ�������д����ֽ�����
//  */ 
// int Readbuf::write_res_data_to_file(int fd, size_t chunksize)
// {
// 	// TODO ���ô�������client��������uploadChunk����Ϣ�����ǻ��кܶ����
// 	int should_w_bytes = chunksize > buf_len ? buf_len : chunksize;
// 	int w_bytes = write(fd, buf, should_w_bytes);
// 	buf_len = 0;
// }

// /**
//  * ��buffer�е����ݿ�����ָ���ڴ��У�ֱ���������ս��
//  * ���ض�ȡ���ֽ�����
//  * �����������������������ַ��������ڴ�
//  * ����-1������socket���ɶ�ȡ
//  * ����0������read(sock)����ֵΪ0
//  */ 
// int Readbuf::read_until(char *b, char del) {
	
// 	// buffer��������
// 	if (buf_len > 0) {
// 		bool if_find = false;
// 		int i = 0;
// 		for (; i < buf_len; i++)
// 		{
// 			if(buf[i] == del){
// 				if_find = true;
// 				break;
// 			}
// 		}
// 		// ��������е�buf���ҵ����ս������ô�ͽ��ս��֮ǰ�������ַ�������ȥ��Ȼ�󷵻ؿ������ֽ�����
// 		if(if_find){
// 			i++;
// 			memcpy(b, buf, i);
// 			buf_len -= i;
// 			memmove(buf, buf + i, buf_len);
// 			return i;
// 		}
// 	}

// 	// ���buffer��û�����ݣ����������ݵ���û���ҵ�

// 	// ����buffer��ʣ��Ŀռ䣬���ʣ��ռ�Ƚ϶࣬�ͼ��������߶����ж��Ƿ������ս���������ͷ���
// 	while(buf_len < N / 2){
// 		bool ready = sock_ready_to_read(sock);
// 		if(!ready)
// 			return -1;

// 		// �����ܶ��ȡ���ݵ�buffer
// 		int r_bytes = read(sock, buf + buf_len, N - buf_len);
// 		if(r_bytes<=0)
// 			return r_bytes;

// 		// �ж��Ƿ��ȡ�����ս��
// 		bool if_find = false;
// 		int i = buf_len;
// 		buf_len += r_bytes;
// 		for (; i < buf_len;i++){
// 			if(buf[i] == del){
// 				if_find = true;
// 				break;
// 			}
// 		}
// 		if(if_find){
// 			i++;
// 			memcpy(b, buf, i);
// 			buf_len -= i;
// 			memmove(buf, buf + i, buf_len);
// 			return i;
// 		}
// 	}

// 	// ���ֱ���ռ䲻�㹻��Ҳû�������ս������ô�Ͱ��ⲿ������ֱ�ӿ�����ȥ
// 	int len = buf_len;
// 	memcpy(b, buf, buf_len);
// 	buf_len = 0;

// 	return len;
// }