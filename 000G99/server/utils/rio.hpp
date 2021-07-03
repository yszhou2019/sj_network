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
 * 最多等待 delay 秒
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
    printf("socket 超时未读就绪\n");
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
        printf("就绪\n");
        return true;
    }
    printf("socket 超时未写就绪\n");
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
 * 保证socket正常，文件存在，文件大小合适的情况下
 * 将数据写入指定文件中
 * 返回写入字节数量
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
        // printf("socket %s\n", ready ? "就绪" : "放弃");
        if (!ready)
        {
            close(fd);
            close(pipefd[0]);
            close(pipefd[1]);
            // 超出了可以忍受的时间，还没有收到足够的字节数量，就不再等待
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
 * 保证socket正常，文件存在，文件大小合适的情况下
 * 从文件中读取字节，发送给socket
 * 返回发送的字节数量
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
            // 超出了可以忍受的时间，还没有缓冲区可以发送，就不再等待
            printf("发送出错, 准备退出\n");
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
        printf("丢弃 %d bytes\n", actural_read);
        if (actural_read == -1)
            actural_read = 0;
        res -= actural_read;
    }
    printf("准备退出\n");
    read(sock, buffer, 1);
}


// const int N = 65536;

// /**
//  * 用这个class的初衷是，server接收数据的时候，把请求体的数据都存入buffer，然后解析
//  * 但是考虑到可能会把请求体的数据和文件数据混在一起
//  * 再调用splice的时候不方便，需要先把buffer中残留的数据写入(判断逻辑有些复杂，暂时搁置)，然后再splice
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
//  * 从socket读取指定长度的数据到buf中，成功返回0，失败返回-1
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
//  * 用途：客户端上传文件，server调用这个函数，将数据接收到buffer中，然后再写入到文件(如果用尽量读的方式来读取json请求，那么就可能会将数据文件读到buffer中)
//  */ 
// int Readbuf::readn(char *b, int len) {
// 	int n = buf_len;
// 	printf("%d bytes in Readbuf, %d bytes to read", n, len);
// 	// buffer已经有足够数量的字节，直接cpy，然后move即可
// 	if (n >= len) {
// 		memcpy(b, buf, len);
// 		memmove(buf, buf + len, n - len);
// 		buf_len -= len;
// 	}
// 	else {
// 	// buffer中的数据不够，先拷贝一部分，再从sock中直接读取部分数据到b中
// 		int m = len - n;
// 		memcpy(b, buf, n);
// 		buf_len = 0;
// 		if (::readn(sock, b + n, m) < 0)
// 			return -1;
// 	}
// 	return 0;
// }

// /**
//  * TODO 解析了header之后，buf中可能残留文件的data数据，因此需要首先将buffer中残留的数据写入到文件中，然后再调用write_to_file
//  * 根据文件的chunksize，将 buffer 中残留的数据信息写入到文件，返回写入的字节数量
//  */ 
// int Readbuf::write_res_data_to_file(int fd, size_t chunksize)
// {
// 	// TODO 不好处理，假设client连续两个uploadChunk的信息，但是会有很多情况
// 	int should_w_bytes = chunksize > buf_len ? buf_len : chunksize;
// 	int w_bytes = write(fd, buf, should_w_bytes);
// 	buf_len = 0;
// }

// /**
//  * 将buffer中的数据拷贝到指定内存中，直到拷贝到终结符
//  * 返回读取的字节数量
//  * 返回正数，代表拷贝了若干字符到给定内存
//  * 返回-1，代表socket不可读取
//  * 返回0，代表read(sock)返回值为0
//  */ 
// int Readbuf::read_until(char *b, char del) {
	
// 	// buffer还有数据
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
// 		// 如果在已有的buf中找到了终结符，那么就将终结符之前的所有字符拷贝过去，然后返回拷贝的字节数量
// 		if(if_find){
// 			i++;
// 			memcpy(b, buf, i);
// 			buf_len -= i;
// 			memmove(buf, buf + i, buf_len);
// 			return i;
// 		}
// 	}

// 	// 如果buffer中没有数据，或者有数据但是没有找到

// 	// 根据buffer中剩余的空间，如果剩余空间比较多，就继续读，边读边判断是否遇到终结符；遇到就返回
// 	while(buf_len < N / 2){
// 		bool ready = sock_ready_to_read(sock);
// 		if(!ready)
// 			return -1;

// 		// 尽可能多读取数据到buffer
// 		int r_bytes = read(sock, buf + buf_len, N - buf_len);
// 		if(r_bytes<=0)
// 			return r_bytes;

// 		// 判断是否读取到了终结符
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

// 	// 如果直到空间不足够，也没有遇到终结符，那么就把这部分数据直接拷贝过去
// 	int len = buf_len;
// 	memcpy(b, buf, buf_len);
// 	buf_len = 0;

// 	return len;
// }