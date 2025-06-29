#ifndef PING_H
#define PING_H

#include 	<netinet/in_systm.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>
#include	<netinet/icmp6.h>
#include	<netinet/ip6.h>
#include	<sys/types.h>	/* 基本系统数据类型 */
#include	<sys/socket.h>	/* 基本套接字定义 */
#include	<sys/time.h>	/* select()使用的timeval{} */
#include	<time.h>		/* pselect()使用的timespec{} */
#include	<netinet/in.h>	/* sockaddr_in{}和其他Internet定义 */
#include	<arpa/inet.h>	/* inet(3)函数 */
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<errno.h>
#include 	<pwd.h>
#include	<unistd.h>
#include	<sys/un.h>		/* Unix域套接字 */
#include	<sys/ioctl.h>
#include	<net/if.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/time.h>
#include <assert.h>
#include <getopt.h>
#include <math.h>
#include <stdint.h>
#ifdef  HAVE_SOCKADDR_DL_STRUCT
# include       <net/if_dl.h>
#endif

#define IPV6

#define BUFSIZE		1500
#define MAXLINE         4096

/* 全局变量 */
char	 recvbuf[BUFSIZE];
char	 sendbuf[BUFSIZE];

int    datalen;	/* ICMP头后面的数据字节数 */
char	*host;
int	 nsent;			/* 每次sendto()加1 */
pid_t pid;			/* 进程ID */
int	 sockfd;    //本地套接字
int	 verbose;
int    daemon_proc;            /* daemon_init()设置为非零 */

/* 函数原型 */
void	 proc_v4(char *, ssize_t, struct timeval *);
void	 proc_v6(char *, ssize_t, struct timeval *);
void	 send_v4(void);
void	 send_v6(void);
void	 readloop(void);
void	 sig_alrm(int);
void	 tv_sub(struct timeval *, struct timeval *);

char * Sock_ntop_host(const struct sockaddr *sa, socklen_t salen);
struct addrinfo* host_serv(const char *host, const char *serv, int family, int socktype);
static void err_doit(int errnoflag, int level, const char *fmt, va_list ap);
void err_quit(const char *fmt, ...);
void err_sys(const char *fmt, ...);

struct proto {
  void	 (*fproc)(char *, ssize_t, struct timeval *);
  void	 (*fsend)(void);
  struct sockaddr  *sasend;	/* 发送用的sockaddr{}，来自getaddrinfo */
  struct sockaddr  *sarecv;	/* 接收用的sockaddr{} */
  socklen_t	    salen;		/* sockaddr{}的长度 */
  int	   	    icmpproto;	/* ICMP的IPPROTO_xxx值 */
} *pr;
/*消息格式，可以看到里面有
1、2、根据不同ip类型（ipv4/ipv6）采用的不同首发函数
3、4、socket套接字
5、套接字长度，ipv4与ipv6长度不同
6、icmp协议号，表现ipv与ipv6
*/

/*****************new add************/
enum mismark{
    FREQINVALID,
    FREQNOTNUMBER
};
struct sdata{
    int send;// 标签为0
    int recv;// 标签为1
    int size;
    int mxsize;
    double min;
    double max;
    double *data;// 标签为2
};
typedef struct sdata sdata;
void sigint_handler(int sig);
int getnum(const char *str);
void errorhandle(int m);
void init_sd(void);
void pushsd(int label,double num);
void show_help(void);
int validate_data_size(int size);
char* get_timestamp(void);
void timestamp_printf(const char *fmt, ...);
void print_ipv4_header_verbose(struct ip *ip);
void print_icmp_verbose(struct icmp *icmp, int icmplen);
const char* get_icmp_type_name(int type);
void adjust_adaptive_interval(double rtt);
void handle_packet_loss(void);
int check_deadline(void);
void fill_data_payload(char *buffer, int len);
void fill_with_string(char *buffer, int len, const char *str);
void output_json_results(void);
void build_json_output(char *buffer, size_t bufsize);
void get_iso_timestamp(char *buffer, size_t size);
char* json_escape_string(const char *input);
int init_log_system(const char *log_path);
void write_log(const char *level, const char *format, ...);
void log_info(const char *format, ...);
void log_ping(const char *format, ...);
void log_timeout(const char *format, ...);
void log_error(const char *format, ...);
void log_debug(const char *format, ...);
void log_stats(const char *format, ...);
void cleanup_log_system(void);
void get_log_timestamp(char *buffer, size_t size);
int freq=0; /*用于c选项*/
int willfreq=0;
int flowing =0;/*用于flow选项*/
int quiet=0;/*用于q选项*/
int broadcast=0;/*用于b选项 - 广播*/
int audible=0;/*用于a选项 - 声音提示*/
int timestamp=0;/*用于time选项 - 时间戳*/
int ttl_value=0;/*用于t选项 - TTL设置，仅IPv4*/
int deadline=0;/*用于w选项 - 总运行时间截止期限(秒)*/
time_t start_time=0;/*记录程序开始时间用于截止期限检查*/
int adaptive=0;/*用于A选项 - 自适应ping模式*/
double adaptive_interval=1.0;/*自适应模式的动态间隔*/
double last_rtt=0.0;/*用于自适应计算的最后RTT*/
int packet_loss_count=0;/*连续丢包计数*/
int interval=1;/*用于i选项 - 数据包间隔时间(秒)*/
char *data_string=NULL;/*用于data-string选项 - 自定义数据载荷*/
int custom_data=0;/*标志：是否使用自定义数据字符串*/
int json_output=0;/*用于json选项 - JSON格式输出*/
char *json_output_file=NULL;/*JSON输出文件路径*/
int log_output=0;/*用于log选项 - 日志文件输出*/
char *log_file_path=NULL;/*日志文件路径*/
FILE *log_fp=NULL;/*日志文件指针*/
sdata sd;
/*****************new add************/

/* 新增功能的全局变量 */
char *interface_name = NULL;     /* 用于--interface选项 - 指定网卡 */
int show_checksum = 0;           /* 用于--checksum选项 - 显示校验和 */
int resolve_only = 0;            /* 用于--resolve-only选项 - 只解析域名 */
int loss_threshold = 0;          /* 用于--loss-threshold选项 - 丢包阈值 */
int color_output = 0;            /* 用于--color选项 - 彩色输出 */
int rtt_graph = 0;               /* 用于--rtt-graph选项 - RTT图表 */
int raw_output = 0;              /* 用于--raw选项 - 显示原始数据 */
int no_dns = 0;                  /* 用于--no-dns选项 - 不进行DNS解析 */
int uid_check = 0;               /* 用于--uid-check选项 - 检查用户权限 */
int export_csv = 0;              /* 用于--export-csv选项 - CSV输出 */
char *csv_file = NULL;           /* CSV输出文件路径 */
FILE *csv_fp = NULL;             /* CSV文件指针 */

/* RTT图表相关 */
#define GRAPH_WIDTH 50
#define GRAPH_HEIGHT 10
double rtt_history[100];         /* RTT历史记录用于图表 */
int rtt_history_count = 0;

/* 新增函数声明 */
int bind_to_interface(int sockfd, const char *ifname);
void print_checksum(unsigned short cksum);
void resolve_host_only(const char *hostname);
void check_loss_threshold(void);
void print_colored_rtt(double rtt);
void update_rtt_graph(double rtt);
void print_rtt_graph(void);
void print_raw_packet(char *packet, ssize_t len);
void check_uid_permissions(void);
int init_csv_output(const char *csv_path);
void write_csv_header(void);
void write_csv_row(int seq, double rtt, int ttl);
void cleanup_csv_output(void);
const char* get_color_code(double rtt);
const char* get_color_reset(void);

/* 新增功能2的全局变量 */
int packet_size_sweep = 0;       /* 用于--packet-size-sweep选项 */
int sweep_min_size = 8;          /* 扫描最小包大小 */
int sweep_max_size = 1472;       /* 扫描最大包大小 */
int sweep_step = 100;            /* 扫描步长 */
int show_jitter = 0;             /* 用于--jitter选项 */
char *pattern_str = NULL;        /* 用于--pattern选项 */
unsigned char *pattern_data = NULL; /* 解析后的模式数据 */
int pattern_len = 0;             /* 模式长度 */
int flood_limit = 0;             /* 用于--flood-limit选项 */
int flood_pps = 0;               /* 每秒包数限制 */
int geo_location = 0;            /* 用于--geo-location选项 */

/* 抖动计算相关 */
double jitter_sum = 0.0;
int jitter_count = 0;
double last_rtt_for_jitter = -1.0;

/* 新增功能2的函数声明 */
void perform_packet_size_sweep(void);
void calculate_jitter(double rtt);
void display_jitter_stats(void);
int parse_hex_pattern(const char *str);
void fill_with_pattern(char *buffer, int len);
void controlled_flood_mode(void);
void show_geo_location(const char *ip_str);
void estimate_geo_location(const char *ip_str, char *location, size_t len);
int get_optimal_packet_size(void);

#endif