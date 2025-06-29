#include "ping.h"            

struct proto	proto_v4 = { proc_v4, send_v4, NULL, NULL, 0, IPPROTO_ICMP };
//v4采用的数据包

#ifdef	IPV6
struct proto	proto_v6 = { proc_v6, send_v6, NULL, NULL, 0, IPPROTO_ICMPV6 };
#endif
//同上 ipv6数据包

int	datalen = 56;		/* ICMP回显请求携带的数据长度 */

int
main(int argc, char **argv)
{
	int				c;
	struct addrinfo	*ai;

/**************new add ***************************/
	signal(SIGINT, sigint_handler);
	init_sd();
/**************new add ***************************/

	/* 定义长选项 */
	static struct option long_options[] = {
		{"data-string", required_argument, 0, 1000},
		{"json", no_argument, 0, 1001},
		{"json-output", required_argument, 0, 1002},
		{"log", required_argument, 0, 1003},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};

	opterr = 0;		/* 不希望getopt()向stderr写入错误信息 */
	while ( (c = getopt_long(argc, argv, "vc:fqhs:bat:Tw:Ai:", long_options, NULL)) != -1) {
		//分析命令行参数的
		switch (c) {
		case 'v':
			verbose++;
			break;
/**************new add ***************************/
		case 'c':
			willfreq=1;
			freq=getnum(optarg);
			break;
		case 'f':
			flowing=1;
			break;
		case 'q':
			quiet=1;
			break;
		case 'h':
			show_help();
			exit(0);
			break;
		case 's':
			datalen = getnum(optarg);
			if (!validate_data_size(datalen)) {
				err_quit("invalid data size: %d (must be 0-1472)", datalen);
			}
			break;
		case 'b':
			broadcast = 1;
			break;
		case 'a':
			audible = 1;
			break;
		case 't':
			ttl_value = getnum(optarg);
			if (ttl_value <= 0 || ttl_value > 255) {
				err_quit("invalid TTL value: %d (must be 1-255)", ttl_value);
			}
			break;
		case 'T':
			timestamp = 1;
			break;
		case 'w':
			deadline = getnum(optarg);
			if (deadline <= 0) {
				err_quit("invalid deadline value: %d (must be > 0)", deadline);
			}
			break;
		case 'A':
			adaptive = 1;
			break;
		case 'i':
			interval = getnum(optarg);
			if (interval <= 0) {
				err_quit("invalid interval value: %d (must be > 0)", interval);
			}
			if (interval > 3600) {
				err_quit("invalid interval value: %d (must be <= 3600)", interval);
			}
			break;
		case 1000:  /* --data-string */
			if (strlen(optarg) == 0) {
				err_quit("data string cannot be empty");
			}
			if (strlen(optarg) > 1472) {
				err_quit("data string too long (max 1472 characters)");
			}
			data_string = strdup(optarg);
			custom_data = 1;
			break;
		case 1001:  /* --json */
			json_output = 1;
			break;
		case 1002:  /* --json-output */
			json_output = 1;
			json_output_file = strdup(optarg);
			break;
		case 1003:  /* --log */
			log_output = 1;
			log_file_path = strdup(optarg);
			break;
/**************new add ***************************/
		case '?':
			err_quit("unrecognized option: %c", c);
		}
	}

	if (optind != argc-1)
		err_quit("usage: ping [options] <hostname>");
	host = argv[optind];
	//host类似本地dns
	
	/* 检查选项冲突和智能交互 */
	if (adaptive && interval != 1) {
		timestamp_printf("Warning: -A (adaptive) will override -i (interval) setting\n");
	}
	if (flowing && interval != 1) {
		timestamp_printf("Warning: -f (flow) ignores -i (interval) setting\n");
	}
	
	/* 处理--data-string与-s的交互 */
	if (custom_data && data_string) {
		int string_len = strlen(data_string);
		if (datalen == 56) {  /* 如果用户没有指定-s，使用默认值56 */
			/* 自动调整数据长度为字符串长度，但至少保持最小值 */
			if (string_len < 56) {
				datalen = 56;  /* 保持最小数据包大小 */
			} else {
				datalen = string_len;
			}
		}
		/* 如果用户明确指定了-s，保持用户设置，字符串会重复填充 */
	}
	
		/* 处理--json模式的交互 */
	if (json_output) {
		/* JSON模式下显示一个进度提示 */
		fprintf(stderr, "Running ping test (Press Ctrl+C to stop and see results)...\n");
		/* 注意：JSON模式不自动设置quiet，让用户自己决定是否要看到ping过程 */
		/* 增加调试信息以帮助诊断问题 */
		if (verbose) {
			fprintf(stderr, "Debug: JSON mode enabled, verbose output will show packet processing details\n");
		} else {
			fprintf(stderr, "Tip: Use '-v' with --json for detailed debugging if no packets are received\n");
		}
	}
	
	/* 初始化日志系统 */
	if (log_output) {
		if (init_log_system(log_file_path) != 0) {
			fprintf(stderr, "Warning: Failed to initialize log system, logging disabled\n");
			log_output = 0;
		}
	}
	
	pid = getpid();
	signal(SIGALRM, sig_alrm);
	//第一个参数：用alarm函数设置的timer超时或setitimer函数设置的interval timer超时
	//处理函数
	//设置了一个闹钟信号及其对应的处理函数，触发后就无限发送了

	ai = host_serv(host, NULL, 0, 0);
	//主机名、端口号（icmp为不需要），不限制地址类型和协议类型

	if (custom_data && data_string) {
		timestamp_printf("ping %s (%s) with data string \"%s\": %d data bytes\n", 
				ai->ai_canonname, Sock_ntop_host(ai->ai_addr, ai->ai_addrlen), 
				data_string, datalen);
		log_info("PING %s (%s) with data string \"%s\": %d data bytes", 
				ai->ai_canonname, Sock_ntop_host(ai->ai_addr, ai->ai_addrlen), 
				data_string, datalen);
	} else {
		timestamp_printf("ping %s (%s): %d data bytes\n", ai->ai_canonname,
		   		Sock_ntop_host(ai->ai_addr, ai->ai_addrlen), datalen);
		log_info("PING %s (%s): %d data bytes", ai->ai_canonname,
		   		Sock_ntop_host(ai->ai_addr, ai->ai_addrlen), datalen);
	}
	
	/* 如果设置了TTL，给出说明 */
	if (ttl_value > 0) {
		timestamp_printf("Note: Outgoing packets will use TTL=%d (reply_ttl shows the TTL of incoming reply packets)\n", ttl_value);
	}

		/* 根据协议类型进行初始化 */
	if (ai->ai_family == AF_INET) {
		pr = &proto_v4;
#ifdef	IPV6
	} else if (ai->ai_family == AF_INET6) {
		pr = &proto_v6;
		if (IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *)
								 ai->ai_addr)->sin6_addr)))
			err_quit("cannot ping IPv4-mapped IPv6 address");
#endif
	} else
		err_quit("unknown address family %d", ai->ai_family);

	pr->sasend = ai->ai_addr;
	pr->sarecv = calloc(1, ai->ai_addrlen);
	pr->salen = ai->ai_addrlen;

	/* 现在所有参数都已解析完成，如果设置了deadline，记录开始时间 */
	if (deadline > 0) {
		start_time = time(NULL);
		log_info("Deadline set to %d seconds, ping will stop at %ld", deadline, start_time + deadline);
		
		/* 如果deadline非常短，给出警告 */
		if (deadline < 3) {
			timestamp_printf("Warning: Very short deadline (%d seconds), may not get meaningful results\n", deadline);
		}
		
		/* 立即检查一次，以防deadline已经过期 */
		check_deadline();
	}

	readloop();

	exit(0);
}

void
proc_v4(char *ptr, ssize_t len, struct timeval *tvrecv)
{
	//len是总长度，ptr是数据包的首地址（包括ip头部），tvrecv是时间戳

	int				hlen1, icmplen;
	double			rtt;
	struct ip		*ip;
	struct icmp		*icmp;
	struct timeval	*tvsend;

	ip = (struct ip *) ptr;		/* IP头部开始位置 */
	hlen1 = ip->ip_hl << 2;		/* IP头部长度 */
	//IP长度是以4字节为标准的？

	icmp = (struct icmp *) (ptr + hlen1);	/* ICMP头部开始位置 */
	//获得icmp数据包的头地址
	if ( (icmplen = len - hlen1) < 8)
		err_quit("icmplen (%d) < 8", icmplen);
	//连最小的长度都不够,得到对应数据包长度

	//如果看到数据包是reply才接受，否则看一个标志量
	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != pid)
			return;			/* 不是对我们回显请求的响应 */
		//检测标识符，这里应该是用进程pid来看的
		if (icmplen < 16)
			err_quit("icmplen (%d) < 16", icmplen);

		/* 从网络字节序的32位整数对中安全地读取时间戳 */
		uint32_t *time_data = (uint32_t *) icmp->icmp_data;
		struct timeval tvsend_safe;
		tvsend_safe.tv_sec = (time_t)ntohl(time_data[0]);   /* 转换秒部分 */
		tvsend_safe.tv_usec = (suseconds_t)ntohl(time_data[1]); /* 转换微秒部分 */
		tvsend = &tvsend_safe;
		
		/* 调试：检查时间戳合理性 */
		time_t current_time = time(NULL);
		if (verbose) {
			timestamp_printf("Debug: Send timestamp: tv_sec=%ld, tv_usec=%ld\n", tvsend->tv_sec, tvsend->tv_usec);
			timestamp_printf("Debug: Recv timestamp: tv_sec=%ld, tv_usec=%ld\n", tvrecv->tv_sec, tvrecv->tv_usec);
			timestamp_printf("Debug: Current time: %ld\n", current_time);
		}
		
		/* 检查发送时间戳的合理性 */
		if (tvsend->tv_sec < 0 || tvsend->tv_usec < 0 || tvsend->tv_usec >= 1000000) {
			if (verbose) {
				timestamp_printf("Warning: Invalid timestamp format (%ld.%06ld), packet ignored\n", 
				                tvsend->tv_sec, tvsend->tv_usec);
			}
			return;  /* 忽略格式损坏的数据包 */
		}
		
		/* 检查时间戳是否在合理范围内 */
		if (tvsend->tv_sec < current_time - 86400 || tvsend->tv_sec > current_time + 60) {
			if (verbose) {
				timestamp_printf("Warning: Timestamp out of reasonable range (%ld.%06ld vs current %ld), packet ignored\n", 
				                tvsend->tv_sec, tvsend->tv_usec, current_time);
			}
			return;  /* 忽略时间异常的数据包 */
		}
		
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
		
		/* 检查RTT合理性 - 只过滤明显异常的值 */
		if (rtt < -1000.0 || rtt > 300000.0) {  /* 只过滤极端异常：负1秒到5分钟 */
			if (verbose) {
				timestamp_printf("Warning: Extreme RTT %.3f ms (send: %ld.%06ld, recv: %ld.%06ld), packet ignored\n", 
				                rtt, tvsend->tv_sec, tvsend->tv_usec, tvrecv->tv_sec, tvrecv->tv_usec);
			}
			return;  /* 忽略这个异常的数据包 */
		}
		//服务器返回的时间戳是我们一开始发给他的
		/***************change********** */
		if(!flowing&&!quiet){
			if (ttl_value > 0) {
				timestamp_printf("%d bytes from %s: seq=%u, reply_ttl=%d (sent_ttl=%d), rtt=%.3f ms\n",
					icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
					icmp->icmp_seq, ip->ip_ttl, ttl_value, rtt);
			} else {
				timestamp_printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
					icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
					icmp->icmp_seq, ip->ip_ttl, rtt);
			}
		}
		
		/* 记录ping响应日志 */
		if (ttl_value > 0) {
			log_ping("%d bytes from %s: seq=%u, reply_ttl=%d (sent_ttl=%d), rtt=%.3f ms",
				icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp->icmp_seq, ip->ip_ttl, ttl_value, rtt);
		} else {
			log_ping("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms",
				icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp->icmp_seq, ip->ip_ttl, rtt);
		}
		
		if(flowing)
			printf("\b");
		
		/* 详细模式显示额外信息 */
		if (verbose && !flowing && !quiet) {
			print_ipv4_header_verbose(ip);
			print_icmp_verbose(icmp, icmplen);
		}
		
		/* 记录详细信息到调试日志 */
		if (verbose && log_output) {
			log_debug("IP Header: version=%d, ttl=%d, protocol=%d", 
			         ip->ip_v, ip->ip_ttl, ip->ip_p);
			log_debug("ICMP Header: type=%d, code=%d, id=%d, seq=%d",
			         icmp->icmp_type, icmp->icmp_code, icmp->icmp_id, icmp->icmp_seq);
		}
		
		/* 声音提示 */
		if (audible) {
			printf("\a");
			fflush(stdout);
		}
		
		/* JSON模式下的进度提示 */
		if (json_output && quiet) {
			fprintf(stderr, ".");
			fflush(stderr);
		}
		
		sd.recv++;
		/* 更新最大值和最小值 */
		if (sd.recv == 1) {
			/* 第一个RTT值，初始化min和max */
			sd.min = rtt;
			sd.max = rtt;
		} else {
			sd.max = (rtt > sd.max) ? rtt : sd.max;
			sd.min = (rtt < sd.min) ? rtt : sd.min;
		}
		sd.data[sd.size]=rtt;
		sd.size++;
		
		/* 适应性模式调整 */
		if (adaptive) {
			/* 收到回复说明网络正常，重置连续丢包计数 */
			if (packet_loss_count > 0) {
				if (verbose && !quiet) {
					timestamp_printf("    Network recovered, resetting loss count from %d to 0\n", packet_loss_count);
				}
				packet_loss_count = 0;
			}
			adjust_adaptive_interval(rtt);
		}
		
		/* 收到IPv4数据包后检查deadline */
		if (deadline > 0) {
			check_deadline();
		}
		
		/***************change********** */
	} else if (verbose) {
		//v参数？
		timestamp_printf("  %d bytes from %s: type = %d (%s), code = %d\n",
				icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp->icmp_type, get_icmp_type_name(icmp->icmp_type), icmp->icmp_code);
		print_ipv4_header_verbose(ip);
		print_icmp_verbose(icmp, icmplen);
	}
}

void
proc_v6(char *ptr, ssize_t len, struct timeval* tvrecv)
{
#ifdef	IPV6
	int					hlen1, icmp6len;
	double				rtt;
	struct ip6_hdr		*ip6;
	struct icmp6_hdr	*icmp6;
	struct timeval		*tvsend;

	/* 对于IPv6，我们需要处理IP头以获取hlim */
	ip6 = (struct ip6_hdr *) ptr;		/* IPv6头部开始位置 */
	hlen1 = sizeof(struct ip6_hdr);
	
	icmp6 = (struct icmp6_hdr *) (ptr + hlen1);
	if ( (icmp6len = len - hlen1) < 8)
		err_quit("icmp6len (%d) < 8", icmp6len);


	if (icmp6->icmp6_type == ICMP6_ECHO_REPLY) {
		if (icmp6->icmp6_id != pid)
			return;			/* 不是对我们回显请求的响应 */
		if (icmp6len < 16)
			err_quit("icmp6len (%d) < 16", icmp6len);

		/* 从网络字节序的32位整数对中安全地读取时间戳 */
		uint32_t *time_data = (uint32_t *) (icmp6 + 1);
		struct timeval tvsend_safe;
		tvsend_safe.tv_sec = (time_t)ntohl(time_data[0]);   /* 转换秒部分 */
		tvsend_safe.tv_usec = (suseconds_t)ntohl(time_data[1]); /* 转换微秒部分 */
		tvsend = &tvsend_safe;
		
		/* 调试：检查时间戳合理性 */
		time_t current_time = time(NULL);
		if (verbose) {
			timestamp_printf("Debug IPv6: Send timestamp: tv_sec=%ld, tv_usec=%ld\n", tvsend->tv_sec, tvsend->tv_usec);
			timestamp_printf("Debug IPv6: Recv timestamp: tv_sec=%ld, tv_usec=%ld\n", tvrecv->tv_sec, tvrecv->tv_usec);
			timestamp_printf("Debug IPv6: Current time: %ld\n", current_time);
		}
		
		/* 检查发送时间戳的合理性 */
		if (tvsend->tv_sec < 0 || tvsend->tv_usec < 0 || tvsend->tv_usec >= 1000000) {
			if (verbose) {
				timestamp_printf("Warning: Invalid IPv6 timestamp format (%ld.%06ld), packet ignored\n",
				                tvsend->tv_sec, tvsend->tv_usec);
			}
			return;  /* 忽略格式损坏的数据包 */
		}
		
		/* 检查时间戳是否在合理范围内 */
		if (tvsend->tv_sec < current_time - 86400 || tvsend->tv_sec > current_time + 60) {
			if (verbose) {
				timestamp_printf("Warning: IPv6 timestamp out of reasonable range (%ld.%06ld vs current %ld), packet ignored\n",
				                tvsend->tv_sec, tvsend->tv_usec, current_time);
			}
			return;  /* 忽略时间异常的数据包 */
		}
		
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;
		
		/* 检查RTT合理性 - 只过滤明显异常的值 */
		if (rtt < -1000.0 || rtt > 300000.0) {  /* 只过滤极端异常：负1秒到5分钟 */
			if (verbose) {
				timestamp_printf("Warning: Extreme IPv6 RTT %.3f ms (send: %ld.%06ld, recv: %ld.%06ld), packet ignored\n", 
				                rtt, tvsend->tv_sec, tvsend->tv_usec, tvrecv->tv_sec, tvrecv->tv_usec);
			}
			return;  /* 忽略这个异常的数据包 */
		}
		/***************change********** */
		if(!flowing&&!quiet){
			timestamp_printf("%d bytes from %s: seq=%u, hlim=%d, rtt=%.3f ms\n",
				icmp6len, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp6->icmp6_seq, ip6->ip6_hlim, rtt);
		}
		
		/* 记录IPv6 ping响应日志 */
		log_ping("%d bytes from %s: seq=%u, hlim=%d, rtt=%.3f ms",
			icmp6len, Sock_ntop_host(pr->sarecv, pr->salen),
			icmp6->icmp6_seq, ip6->ip6_hlim, rtt);
		
		if(flowing)
			printf("\b");
		
		/* 详细模式显示额外信息（IPv6简化版） */
		if (verbose && !flowing && !quiet) {
			printf("    ICMPv6 Details:\n");
			printf("      Type: %d, Code: %d\n", icmp6->icmp6_type, icmp6->icmp6_code);
			printf("      ID: %d, Sequence: %d\n", icmp6->icmp6_id, icmp6->icmp6_seq);
			printf("      Data Length: %d bytes\n", icmp6len - 8);
		}
		
		/* 记录IPv6详细信息到调试日志 */
		if (verbose && log_output) {
			log_debug("IPv6 Header: version=%d, hlim=%d", 
			         (ip6->ip6_vfc >> 4) & 0xf, ip6->ip6_hlim);
			log_debug("ICMPv6 Header: type=%d, code=%d, id=%d, seq=%d",
			         icmp6->icmp6_type, icmp6->icmp6_code, icmp6->icmp6_id, icmp6->icmp6_seq);
		}
		
		/* 声音提示 */
		if (audible) {
			printf("\a");
			fflush(stdout);
		}
		
		/* JSON模式下的进度提示 */
		if (json_output && quiet) {
			fprintf(stderr, ".");
			fflush(stderr);
		}
		
		sd.recv++;
		/* 更新最大值和最小值 */
		if (sd.recv == 1) {
			/* 第一个RTT值，初始化min和max */
			sd.min = rtt;
			sd.max = rtt;
		} else {
			sd.max = (rtt > sd.max) ? rtt : sd.max;
			sd.min = (rtt < sd.min) ? rtt : sd.min;
		}
		sd.data[sd.size]=rtt;
		sd.size++;
		
		/* 适应性模式调整 */
		if (adaptive) {
			/* 收到回复说明网络正常，重置连续丢包计数 */
			if (packet_loss_count > 0) {
				if (verbose && !quiet) {
					timestamp_printf("    IPv6 network recovered, resetting loss count from %d to 0\n", packet_loss_count);
				}
				packet_loss_count = 0;
			}
			adjust_adaptive_interval(rtt);
		}
		
		/* 收到IPv6数据包后检查deadline */
		if (deadline > 0) {
			check_deadline();
		}
		
		/***************change********** */
	} else if (verbose) {
		timestamp_printf("  %d bytes from %s: type = %d, code = %d\n",
				icmp6len, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp6->icmp6_type, icmp6->icmp6_code);
		printf("    ICMPv6 Details:\n");
		printf("      Type: %d, Code: %d\n", icmp6->icmp6_type, icmp6->icmp6_code);
		printf("      Data Length: %d bytes\n", icmp6len - 8);
		
		/* 显示数据负载内容（如果有自定义数据字符串） */
		if (custom_data && data_string && icmp6len > 16) {
			printf("      Custom Data Payload: \"");
			/* 显示时间戳后的数据部分 */
			char *data_part = (char *)(icmp6 + 1) + 8;
			int data_len = icmp6len - 16;  /* 减去ICMPv6头部8字节和时间戳8字节 */
			
			for (int i = 0; i < data_len && i < 50; i++) {  /* 最多显示50个字符 */
				if (data_part[i] >= 32 && data_part[i] <= 126) {  /* 可打印字符 */
					printf("%c", data_part[i]);
				} else {
					printf("\\x%02x", (unsigned char)data_part[i]);
				}
			}
			if (data_len > 50) {
				printf("...");
			}
			printf("\"\n");
		}
	}
#endif	/* IPV6 */
}

unsigned short
in_cksum(unsigned short *addr, int len)
{
        int                             nleft = len;
        int                             sum = 0;
        unsigned short  *w = addr;
        unsigned short  answer = 0;

        /*
         * 校验算法很简单：使用32位累加器(sum)，将16位字依次相加，
         * 最后将高16位的进位折叠回低16位
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

                /* 如果需要的话，处理剩余的单个字节 */
        if (nleft == 1) {
                *(unsigned char *)(&answer) = *(unsigned char *)w ;
                sum += answer;
        }

                /* 将高16位的进位加回到低16位 */
        sum = (sum >> 16) + (sum & 0xffff);     /* 将高16位加到低16位 */
        sum += (sum >> 16);                     /* 加上进位 */
        answer = ~sum;                          /* 截断为16位 */
        return(answer);
}

void
send_v4(void)
{
	int			len;
	struct icmp	*icmp;
	//似乎是icmp数据包

	icmp = (struct icmp *) sendbuf;
	icmp->icmp_type = ICMP_ECHO;//类型8 request
	icmp->icmp_code = 0;//代码0
	icmp->icmp_id = pid;//标识符
	icmp->icmp_seq = nsent++;//序号
	
	/* 安全的时间戳存储方式 - 避免对齐和字节序问题 */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	/* 将时间戳转换为网络字节序的32位整数对存储 */
	uint32_t *time_data = (uint32_t *) icmp->icmp_data;
	time_data[0] = htonl((uint32_t)tv.tv_sec);   /* 秒部分 */
	time_data[1] = htonl((uint32_t)tv.tv_usec);  /* 微秒部分 */
	
	/* 填充自定义数据到时间戳之后的区域 */
	if (datalen > 8) {  /* 如果有额外的数据空间 */
		fill_data_payload((char *)icmp->icmp_data + 8, datalen - 8);
	}

	len = 8 + datalen;		/* 对ICMP头部和数据进行校验 *///8个字节代表首部+data
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum((u_short *) icmp, len);

	sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
	
	/* 统一的发送计数管理 */
	sd.send++;
	
	/* 记录发送日志 */
	if (verbose && log_output) {
		log_debug("Sent ICMP packet: seq=%u, len=%d bytes", icmp->icmp_seq, len);
	}
	
	/*用于发送信息
	参数
	1、sockfd 指定发送端套接字描述符 int
	2、sendbuf 存放发送数据的缓存区 const void *
	3、实际要发送的字节数 size_t
	4、默认0 int 
	5、存放目的主机的ip地址和端口信息 const struct socket
	6 to的长度 socket_len
	*/
	}

void
send_v6()
{
#ifdef	IPV6
	int					len;
	struct icmp6_hdr	*icmp6;

	icmp6 = (struct icmp6_hdr *) sendbuf;
	icmp6->icmp6_type = ICMP6_ECHO_REQUEST;
	icmp6->icmp6_code = 0;
	icmp6->icmp6_id = pid;
	icmp6->icmp6_seq = nsent++;
	
	/* 安全的时间戳存储方式 - 避免对齐和字节序问题 */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	/* 将时间戳转换为网络字节序的32位整数对存储 */
	uint32_t *time_data = (uint32_t *) (icmp6 + 1);
	time_data[0] = htonl((uint32_t)tv.tv_sec);   /* 秒部分 */
	time_data[1] = htonl((uint32_t)tv.tv_usec);  /* 微秒部分 */
	
	/* 填充自定义数据到时间戳之后的区域 */
	if (datalen > 8) {  /* 如果有额外的数据空间 */
		fill_data_payload((char *)(icmp6 + 1) + 8, datalen - 8);
	}

	len = 8 + datalen;		/* 8字节的ICMPv6头部 */

	sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
	
	/* 统一的发送计数管理 */
	sd.send++;
	
	/* 记录IPv6发送日志 */
	if (verbose && log_output) {
		log_debug("Sent ICMPv6 packet: seq=%u, len=%d bytes", icmp6->icmp6_seq, len);
	}
	
		/* 内核会为我们计算并存储校验和 */
#endif	/* IPV6 */
}

void
readloop(void)
{
	int				size;
	char			recvbuf[BUFSIZE];
	socklen_t		len;
	ssize_t			n;
	struct timeval	tval;

	sockfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);
	//创建套接字，选中地址的地址族，使用原始接口，协议号
	if (sockfd < 0) {
		err_sys("socket error - need root privileges to create raw socket");
	}
	setuid(getuid());		/* 不再需要特殊权限 */

	size = 60 * 1024;		/* 如果setsockopt失败也没关系 */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	//socket，通用套接字选项，              发送缓冲区大小
	
	/* 设置TTL值（仅IPv4） */
	if (ttl_value > 0 && pr->sasend->sa_family == AF_INET) {
		if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl_value, sizeof(ttl_value)) < 0) {
			err_sys("setsockopt IP_TTL error");
		}
	} else if (ttl_value > 0 && pr->sasend->sa_family == AF_INET6) {
		timestamp_printf("Warning: TTL option (-t) is only supported for IPv4\n");
	}
	
	/* 如果指定了广播选项且是IPv4，则设置广播权限 */
	if (broadcast && pr->sasend->sa_family == AF_INET) {
		int on = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
			err_sys("setsockopt SO_BROADCAST error");
		}
		/* 检查是否为广播地址并给出提示 */
		struct sockaddr_in *sin = (struct sockaddr_in *)pr->sasend;
		if ((ntohl(sin->sin_addr.s_addr) & 0xFF) == 0xFF) {
			timestamp_printf("Note: Broadcasting to %s - most devices don't respond to broadcast pings (this is normal)\n", 
			                Sock_ntop_host(pr->sasend, pr->salen));
		}
	} else if (broadcast && pr->sasend->sa_family == AF_INET6) {
		timestamp_printf("Warning: broadcast option (-b) is not supported for IPv6\n");
	}    
	sig_alrm(SIGALRM);		/* 发送第一个数据包 */

	for ( ; ; ) {
		/* 在每次循环开始时检查deadline */
		if (deadline > 0) {
			check_deadline();
		}
		
		len = pr->salen;
		n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, pr->sarecv, &len);
		//                                                保存发送数据的源地址，以及对应的地址长度
		if (n < 0) {
			if (errno == EINTR) {
				/* 被信号中断，这是检查deadline的好时机 */
				if (deadline > 0) {
					check_deadline();
				}
				continue;
			} else
				err_sys("recvfrom error");
		}

		gettimeofday(&tval, NULL);
		//接受到返回消息时的时间
		(*pr->fproc)(recvbuf, n, &tval);
		
		/* 处理完数据包后再次检查deadline */
		if (deadline > 0) {
			check_deadline();
		}
	}
}

void
sig_alrm(int signo)
{
/*******************change*********************** */
	/* 检查deadline */
	if (deadline > 0) {
		check_deadline();
	}
	
	/* 自适应模式：如果再次收到alarm说明上一个包可能超时了 */
	if (adaptive && sd.send > sd.recv) {
		/* 有未收到回复的数据包，可能是超时了 */
		int lost_packets = sd.send - sd.recv;
		if (lost_packets > 0) {
			packet_loss_count++;
			if (verbose && !quiet) {
				timestamp_printf("    Timeout detected, loss count: %d (sent: %d, recv: %d)\n", 
				                packet_loss_count, sd.send, sd.recv);
			}
			
			/* 处理连续丢包情况 */
			if (packet_loss_count > 1) {
				handle_packet_loss();
			}
		}
	}
	
	/* 检查是否达到发送次数限制 */
	if (willfreq && freq <= 0) {
		/* 已经发送完指定次数，退出程序 */
		sigint_handler(SIGINT);
		return;
	}
	
	/* 发送数据包 */
	(*pr->fsend)();
	/* 注意：sd.send的计数现在在send_v4/send_v6函数中进行 */
	
	/* 如果指定了发送次数，减少剩余次数 */
	if (willfreq) {
		freq--;
	}
	
	/* 设置下一次alarm */
	if (adaptive) {
		/* 确保自适应间隔至少为0.1秒，避免过快发送 */
		double next_interval = adaptive_interval;
		if (flowing) {
			next_interval = 0.2;  /* 流模式下使用200ms间隔，快速但可靠 */
		}
		if (next_interval < 0.1) next_interval = 0.1;  /* 最小100ms */
		
		unsigned int alarm_interval = (unsigned int)(next_interval * 1000000);  /* 转换为微秒 */
		struct itimerval timer;
		timer.it_value.tv_sec = alarm_interval / 1000000;
		timer.it_value.tv_usec = alarm_interval % 1000000;
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);
	} else if (flowing) {
		/* 流模式：使用200ms间隔实现快速但可靠的发送 */
		struct itimerval timer;
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 200000;  /* 200ms - 每秒5个包，快速但不会导致拥塞 */
		timer.it_interval.tv_sec = 0;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);
	} else {
		/* 普通模式：使用指定间隔 */
		alarm(interval);
	}
	
	return;         /* 可能会中断recvfrom()调用 */
/*******************change*********************** */
}

void
tv_sub(struct timeval *out, struct timeval *in)
{
	if ( (out->tv_usec -= in->tv_usec) < 0) {	/* out 减去 in */
		--out->tv_sec;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;
}




char *
sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
    static char str[128];               /* Unix域套接字地址最长 */

        switch (sa->sa_family) {
        case AF_INET: {
                struct sockaddr_in      *sin = (struct sockaddr_in *) sa;

                if (inet_ntop(AF_INET, &sin->sin_addr, str, sizeof(str)) == NULL)
                        return(NULL);
                return(str);
        }

#ifdef  IPV6
        case AF_INET6: {
                struct sockaddr_in6     *sin6 = (struct sockaddr_in6 *) sa;

                if (inet_ntop(AF_INET6, &sin6->sin6_addr, str, sizeof(str)) == NULL)
                        return(NULL);
                return(str);
        }
#endif

#ifdef  HAVE_SOCKADDR_DL_STRUCT
        case AF_LINK: {
                struct sockaddr_dl      *sdl = (struct sockaddr_dl *) sa;

                if (sdl->sdl_nlen > 0)
                        snprintf(str, sizeof(str), "%*s",
                                         sdl->sdl_nlen, &sdl->sdl_data[0]);
                else
                        snprintf(str, sizeof(str), "AF_LINK, index=%d", sdl->sdl_index);
                return(str);
        }
#endif
        default:
                snprintf(str, sizeof(str), "sock_ntop_host: unknown AF_xxx: %d, len %d",
                                 sa->sa_family, salen);
                return(str);
        }
    return (NULL);
}

char *
Sock_ntop_host(const struct sockaddr *sa, socklen_t salen)
{
        char    *ptr;

        if ( (ptr = sock_ntop_host(sa, salen)) == NULL)
                err_sys("sock_ntop_host error");        /* inet_ntop()会设置errno */
        return(ptr);
}

struct addrinfo *
host_serv(const char *host, const char *serv, int family, int socktype)
{
        int                             n;
        struct addrinfo hints, *res;

        bzero(&hints, sizeof(struct addrinfo));
        hints.ai_flags = AI_CANONNAME;  /* 总是返回规范名称 */
        hints.ai_family = family;               /* AF_UNSPEC, AF_INET, AF_INET6等 */
        hints.ai_socktype = socktype;   /* 0, SOCK_STREAM, SOCK_DGRAM等 */
		//要求返回规范主机名，根据family地址族返回不同的地址，socktype指定协议tcp/udp类型
        if ( (n = getaddrinfo(host, serv, &hints, &res)) != 0)
                return(NULL);

        return(res);    /* 返回链表中第一个元素的指针 */
}
/* host_serv函数结束 */

static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
        int             errno_save, n;
        char    buf[MAXLINE];

        errno_save = errno;             /* 调用者可能需要打印的值 */
#ifdef  HAVE_VSNPRINTF
        vsnprintf(buf, sizeof(buf), fmt, ap);   /* 这是安全的 */
#else
        vsprintf(buf, fmt, ap);                                 /* 这是不安全的 */
#endif
        n = strlen(buf);
        if (errnoflag)
                snprintf(buf+n, sizeof(buf)-n, ": %s", strerror(errno_save));
        strcat(buf, "\n");

        if (daemon_proc) {
                syslog(level, "%s", buf);
        } else {
                fflush(stdout);         /* 防止stdout和stderr是同一个文件 */
                fputs(buf, stderr);
                fflush(stderr);
        }
        return;
}


/* 与系统调用无关的致命错误
 * 打印消息并终止程序 */

void
err_quit(const char *fmt, ...)
{
        va_list         ap;

        va_start(ap, fmt);
        err_doit(0, LOG_ERR, fmt, ap);
        va_end(ap);
        exit(1);
}

/* 与系统调用相关的致命错误
 * 打印消息并终止程序 */

void
err_sys(const char *fmt, ...)
{
        va_list         ap;

        va_start(ap, fmt);
        err_doit(1, LOG_ERR, fmt, ap);
        va_end(ap);
        exit(1);
}
/********************add**************** */
void sigint_handler(int sig){
	if(sig == SIGINT){
		// ctrl+c退出时执行的代码
		
		if (json_output) {
			/* JSON模式输出 */
			fprintf(stderr, "\n");  /* 清理进度提示行 */
			output_json_results();
		} else {
			/* 传统文本模式输出 */
			timestamp_printf("\n--- %s ping statistics ---\n", host);
			timestamp_printf("%d packets transmitted, %d received, %.0f%% packet loss\n",
			   sd.send, sd.recv, 
			   sd.send > 0 ? (100 - (double)sd.recv/sd.send*100) : 0);
			
			/* 在广播模式下提供额外说明 */
			if (broadcast && sd.recv == 0) {
				timestamp_printf("Note: 100%% packet loss in broadcast mode is normal - most devices don't respond to broadcast pings\n");
			}
			
			if (sd.recv > 0) {
				double sum=0;
				if(sd.size>1000)
					sd.size=1000;
				for(int i=0;i<sd.size;i++)
					sum+=sd.data[i];
				double avg = sum/sd.size;
				
				timestamp_printf("round-trip min/avg/max = %.3f/%.3f/%.3f ms\n",
				   sd.min, avg, sd.max);
			}
		}
		
		/* 记录统计日志 */
		if (log_output) {
			log_stats("--- %s ping statistics ---", host);
			log_stats("%d packets transmitted, %d received, %.0f%% packet loss",
			   sd.send, sd.recv, 
			   sd.send > 0 ? (100 - (double)sd.recv/sd.send*100) : 0);
			
			if (sd.recv > 0) {
				double sum=0;
				int count = sd.size > 1000 ? 1000 : sd.size;
				for(int i=0;i<count;i++)
					sum+=sd.data[i];
				double avg = sum/count;
				
				log_stats("round-trip min/avg/max = %.3f/%.3f/%.3f ms",
				   sd.min, avg, sd.max);
			}
		}
	}
	
	/* 清理日志系统 */
	if (log_output) {
		cleanup_log_system();
	}
	
	exit(1);
}
#define ISDIGIT0TO9(ch) ch>='0'&&ch <='9'
int getnum(const char *str){
	if(str==NULL){
		printf("NULL\n");
		exit(1);
	}
	int ans=0;
	const char *ptr=str;
	if(*ptr=='+')
		ptr++;
	while(*ptr!='\0'){
		if(ISDIGIT0TO9(*ptr))
			ans=ans*10+*ptr-'0';
		else{
			errorhandle(FREQNOTNUMBER);
		}
		ptr++;
	}
	if(ans<0)
		errorhandle(FREQINVALID);
	return ans;
}
void errorhandle(int m){
	switch(m){
		case FREQINVALID:
			printf("\nfreq should >0\n");
			break;
		case FREQNOTNUMBER:
			printf("\n input shoule be number");
			break;
	}
}
void init_sd(void){
	sd.size=0;
	sd.send=0;
	sd.recv=0;
	sd.data=(double*)malloc(sizeof(double)*1000);
	sd.min=999999.0;  /* 初始化为一个大值，会被第一个有效RTT替换 */
	sd.max=0.0;
	sd.mxsize=1;
}

void show_help(void){
    printf("用法: ping [选项] <主机名>\n");
    printf("选项:\n");
    printf("  -v          详细模式 - 显示详细信息\n");
    printf("  -c <数量>   发送指定数量的数据包后停止\n");
    printf("  -f          流模式 - 连续快速发送\n");
    printf("  -q          静默模式 - 减少输出信息\n");
    printf("  -s <大小>   指定发送的数据字节数 (0-1472)\n");
    printf("  -h          显示此帮助信息\n");
    printf("  -b          允许ping广播地址 (仅IPv4, 通常无回复)\n");
    printf("  -a          成功回复时发出声音提示\n");
    printf("  -t <ttl>    设置发送包的TTL值 (1-255, 仅IPv4)\n");
    printf("  -T          为每行添加时间戳\n");
    printf("  -w <秒数>   指定秒数后停止 (运行时间限制)\n");
    printf("  -A          适应性模式 - 根据RTT自动调整间隔\n");
    printf("  -i <秒数>   设置数据包间隔时间 (1-3600秒)\n");
    printf("  --data-string <字符串>  使用指定字符串填充数据包\n");
    printf("  --json      以JSON格式输出统计结果\n");
    printf("  --json-output <文件>  将JSON结果保存到文件\n");
    printf("  --log <文件>  将ping过程记录到日志文件\n");
    printf("\n使用示例:\n");
    printf("  ping baidu.com\n");
    printf("  ping -c 4 -s 1000 baidu.com\n");
    printf("  ping -b 192.168.1.255    # 广播ping (通常无回复)\n");
    printf("  ping -a -T baidu.com\n");
    printf("  ping -t 64 baidu.com\n");
    printf("  ping -w 10 baidu.com\n");
    printf("  ping -A -v baidu.com\n");
    printf("  ping -i 5 baidu.com\n");
    printf("  ping -i 2 -c 10 baidu.com\n");
    printf("  ping --data-string \"hello\" baidu.com\n");
    printf("  ping --data-string \"test\" -s 100 baidu.com\n");
    printf("  ping --json -c 5 baidu.com\n");
    printf("  ping --json-output results.json -c 10 baidu.com\n");
    printf("  ping --log network.log -c 5 baidu.com\n");
    printf("  ping --log debug.log -v -T baidu.com\n");
}

int validate_data_size(int size){
    // 数据大小必须在合理范围内
    // 最小0字节，最大1472字节 (1500 MTU - 20 IP header - 8 ICMP header)
    if (size < 0 || size > 1472) {
        return 0; // 无效
    }
    return 1; // 有效
}

char* get_timestamp(void){
    static char timestamp_buf[64];
    struct timeval tv;
    struct tm *tm_info;
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    snprintf(timestamp_buf, sizeof(timestamp_buf), 
             "[%04d-%02d-%02d %02d:%02d:%02d.%03ld]",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             tv.tv_usec / 1000);
    
    return timestamp_buf;
}

void timestamp_printf(const char *fmt, ...){
    if (timestamp) {
        printf("%s ", get_timestamp());
    }
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

const char* get_icmp_type_name(int type){
    switch(type) {
        case ICMP_ECHOREPLY: return "Echo Reply";
        case ICMP_ECHO: return "Echo Request";
        case ICMP_UNREACH: return "Destination Unreachable";
        case ICMP_SOURCEQUENCH: return "Source Quench";
        case ICMP_REDIRECT: return "Redirect";
        case ICMP_TIMXCEED: return "Time Exceeded";
        case ICMP_PARAMPROB: return "Parameter Problem";
        case ICMP_TSTAMP: return "Timestamp Request";
        case ICMP_TSTAMPREPLY: return "Timestamp Reply";
        case ICMP_IREQ: return "Information Request";
        case ICMP_IREQREPLY: return "Information Reply";
        default: return "Unknown";
    }
}

void print_ipv4_header_verbose(struct ip *ip){
    printf("    IP Header Info:\n");
    printf("      Version: %d, IHL: %d bytes, TOS: 0x%02x\n", 
           ip->ip_v, ip->ip_hl * 4, ip->ip_tos);
    printf("      Total Length: %d, ID: 0x%04x\n", 
           ntohs(ip->ip_len), ntohs(ip->ip_id));
    printf("      Flags: 0x%02x%s%s, Fragment Offset: %d\n",
           (ntohs(ip->ip_off) & 0xe000) >> 13,
           (ntohs(ip->ip_off) & IP_DF) ? " [DF]" : "",
           (ntohs(ip->ip_off) & IP_MF) ? " [MF]" : "",
           ntohs(ip->ip_off) & 0x1fff);
    printf("      TTL: %d, Protocol: %d, Checksum: 0x%04x\n",
           ip->ip_ttl, ip->ip_p, ntohs(ip->ip_sum));
}

void print_icmp_verbose(struct icmp *icmp, int icmplen){
    printf("    ICMP Details:\n");
    printf("      Type: %d (%s), Code: %d\n", 
           icmp->icmp_type, get_icmp_type_name(icmp->icmp_type), icmp->icmp_code);
    printf("      ID: %d, Sequence: %d\n", icmp->icmp_id, icmp->icmp_seq);
    printf("      Checksum: 0x%04x, Data Length: %d bytes\n",
           icmp->icmp_cksum, icmplen - 8);
    
    /* 显示数据负载内容（如果有自定义数据字符串） */
    if (custom_data && data_string && icmplen > 16) {
        printf("      Custom Data Payload: \"");
        /* 显示时间戳后的数据部分 */
        char *data_part = (char *)icmp->icmp_data + 8;
        int data_len = icmplen - 16;  /* 减去ICMP头部8字节和时间戳8字节 */
        
        for (int i = 0; i < data_len && i < 50; i++) {  /* 最多显示50个字符 */
            if (data_part[i] >= 32 && data_part[i] <= 126) {  /* 可打印字符 */
                printf("%c", data_part[i]);
            } else {
                printf("\\x%02x", (unsigned char)data_part[i]);
            }
        }
        if (data_len > 50) {
            printf("...");
        }
        printf("\"\n");
    }
}

/* 适应性间隔调整函数 */
void adjust_adaptive_interval(double rtt) {
    /* 基于RTT调整发送间隔，类似TCP拥塞控制 */
    
    /* 如果RTT很低(<50ms)，缩短间隔以获得更快的探测 */
    if (rtt < 50.0) {
        adaptive_interval = adaptive_interval * 0.9;
        if (adaptive_interval < 1.0) {
            adaptive_interval = 1.0; /* 最小间隔1秒，避免alarm(0)问题 */
        }
    }
    /* 如果RTT适中(50-200ms)，保持当前间隔 */
    else if (rtt >= 50.0 && rtt <= 200.0) {
        /* 轻微增加间隔以稳定网络负载 */
        adaptive_interval = adaptive_interval * 1.05;
        if (adaptive_interval > 2.0) {
            adaptive_interval = 2.0;
        }
    }
    /* 如果RTT很高(>200ms)，显著增加间隔 */
    else {
        adaptive_interval = adaptive_interval * 1.5;
        if (adaptive_interval > 5.0) {
            adaptive_interval = 5.0; /* 最大间隔5秒 */
        }
    }
    
    /* 如果RTT比上次显著增加，也增加间隔 */
    if (last_rtt > 0 && rtt > last_rtt * 1.5) {
        adaptive_interval = adaptive_interval * 1.2;
        if (adaptive_interval > 5.0) {
            adaptive_interval = 5.0;
        }
    }
    
    last_rtt = rtt;
    
    if (verbose && !quiet) {
        timestamp_printf("    Adaptive interval adjusted to %.2f seconds (RTT: %.3f ms)\n", 
                        adaptive_interval, rtt);
    }
}

/* 检查deadline是否到期 */
int check_deadline(void) {
	/* 如果没有设置deadline，直接返回 */
	if (deadline <= 0) return 0;
	
	if (start_time > 0) {
		time_t current_time = time(NULL);
		time_t elapsed = current_time - start_time;
		
		if (elapsed >= deadline) {
			if (verbose) {
				timestamp_printf("Deadline reached: %ld seconds elapsed (limit: %d)\n", 
				                elapsed, deadline);
			}
			log_info("Deadline reached after %ld seconds", elapsed);
			
			/* 输出统计信息并退出 */
			if (json_output) {
				fprintf(stderr, "\n");  /* 清理进度提示行 */
				output_json_results();
			} else {
				timestamp_printf("\n--- %s ping statistics (deadline reached) ---\n", host);
				timestamp_printf("%d packets transmitted, %d received, %.0f%% packet loss\n",
				   sd.send, sd.recv, 
				   sd.send > 0 ? (100 - (double)sd.recv/sd.send*100) : 0);
				
				if (sd.recv > 0) {
					double sum=0;
					int count = sd.size > 1000 ? 1000 : sd.size;
					for(int i=0;i<count;i++)
						sum+=sd.data[i];
					double avg = sum/count;
					
					timestamp_printf("round-trip min/avg/max = %.3f/%.3f/%.3f ms\n",
					   sd.min, avg, sd.max);
				}
			}
			
			/* 清理并退出 */
			if (log_output) {
				cleanup_log_system();
			}
			exit(0);
		}
		
		/* 如果接近deadline，给出警告 */
		if (elapsed > deadline * 0.9 && elapsed < deadline) {
			static int warning_given = 0;
			if (!warning_given && verbose) {
				timestamp_printf("Warning: Approaching deadline (%.1f%% elapsed)\n", 
				                (double)elapsed / deadline * 100);
				warning_given = 1;
			}
		}
	}
	return 0;  /* 未到期 */
}

/* 处理丢包情况 */
void handle_packet_loss(void) {
    /* 连续丢包时，根据丢包次数逐步增加发送间隔 */
    if (packet_loss_count == 1) {
        adaptive_interval = adaptive_interval * 1.2;  /* 第一次丢包，适度增加 */
    } else if (packet_loss_count <= 3) {
        adaptive_interval = adaptive_interval * 1.5;  /* 连续丢包，进一步增加 */
    } else {
        adaptive_interval = adaptive_interval * 2.0;  /* 严重丢包，大幅增加 */
    }
    
    if (adaptive_interval > 10.0) {
        adaptive_interval = 10.0; /* 最大间隔10秒 */
    }
    
    if (verbose && !quiet) {
        timestamp_printf("    Packet loss detected (count: %d), interval adjusted to %.2f seconds\n", 
                        packet_loss_count, adaptive_interval);
    }
}

/* 数据负载填充函数 */
void fill_data_payload(char *buffer, int len) {
    if (custom_data && data_string) {
        fill_with_string(buffer, len, data_string);
    } else {
        /* 默认填充模式 - 使用递增的字节值 */
        int i;
        for (i = 0; i < len; i++) {
            buffer[i] = (char)(i & 0xff);
        }
    }
}

/* 使用字符串循环填充缓冲区 */
void fill_with_string(char *buffer, int len, const char *str) {
    int str_len = strlen(str);
    int i;
    
    if (str_len == 0) return;  /* 防止除零错误 */
    
    for (i = 0; i < len; i++) {
        buffer[i] = str[i % str_len];  /* 循环重复字符串 */
    }
}

/* 获取ISO 8601格式的时间戳 */
void get_iso_timestamp(char *buffer, size_t size) {
    struct timeval tv;
    struct tm *tm_info;
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    /* ISO 8601格式: 2024-01-15T10:30:00.123Z */
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%S", tm_info);
    
    /* 添加毫秒精度 */
    char ms_buffer[32];
    snprintf(ms_buffer, sizeof(ms_buffer), ".%03ld", tv.tv_usec / 1000);
    strncat(buffer, ms_buffer, size - strlen(buffer) - 1);
    strncat(buffer, "Z", size - strlen(buffer) - 1);
}

/* JSON字符串转义 */
char* json_escape_string(const char *input) {
    static char escaped[512];
    int i, j = 0;
    
    if (!input) {
        escaped[0] = '\0';
        return escaped;
    }
    
    for (i = 0; input[i] && j < sizeof(escaped) - 2; i++) {
        switch (input[i]) {
            case '"':
                escaped[j++] = '\\';
                escaped[j++] = '"';
                break;
            case '\\':
                escaped[j++] = '\\';
                escaped[j++] = '\\';
                break;
            case '\n':
                escaped[j++] = '\\';
                escaped[j++] = 'n';
                break;
            case '\r':
                escaped[j++] = '\\';
                escaped[j++] = 'r';
                break;
            case '\t':
                escaped[j++] = '\\';
                escaped[j++] = 't';
                break;
            default:
                escaped[j++] = input[i];
                break;
        }
    }
    escaped[j] = '\0';
    return escaped;
}

/* 构建JSON输出 */
void build_json_output(char *buffer, size_t bufsize) {
    char temp[1024];
    char timestamp[32];
    double loss_percent = 0.0;
    double avg_rtt = 0.0;
    double stddev_rtt = 0.0;
    
    /* 计算统计数据 */
    if (sd.send > 0) {
        loss_percent = (1.0 - (double)sd.recv / sd.send) * 100.0;
    }
    
    if (sd.recv > 0 && sd.size > 0) {
        double sum = 0.0;
        int count = sd.size > 1000 ? 1000 : sd.size;
        
        for (int i = 0; i < count; i++) {
            sum += sd.data[i];
        }
        avg_rtt = sum / count;
        
        /* 计算标准差 */
        double variance = 0.0;
        for (int i = 0; i < count; i++) {
            variance += (sd.data[i] - avg_rtt) * (sd.data[i] - avg_rtt);
        }
        stddev_rtt = sqrt(variance / count);
    }
    
    get_iso_timestamp(timestamp, sizeof(timestamp));
    
    /* 构建JSON */
    snprintf(buffer, bufsize, "{\n");
    
    /* target信息 */
    snprintf(temp, sizeof(temp),
        "  \"target\": {\n"
        "    \"host\": \"%s\",\n"
        "    \"resolved_at\": \"%s\"\n"
        "  },\n",
        json_escape_string(host),
        timestamp);
    strncat(buffer, temp, bufsize - strlen(buffer) - 1);
    
    /* test_config信息 */
    snprintf(temp, sizeof(temp),
        "  \"test_config\": {\n"
        "    \"packet_size\": %d,\n"
        "    \"interval\": %d\n"
        "  },\n",
        datalen, interval);
    strncat(buffer, temp, bufsize - strlen(buffer) - 1);
    
    /* summary统计 */
    snprintf(temp, sizeof(temp),
        "  \"summary\": {\n"
        "    \"packets_sent\": %d,\n"
        "    \"packets_received\": %d,\n"
        "    \"packet_loss_percent\": %.1f\n"
        "  },\n",
        sd.send, sd.recv, loss_percent);
    strncat(buffer, temp, bufsize - strlen(buffer) - 1);
    
    /* rtt_stats统计 */
    if (sd.recv > 0) {
        snprintf(temp, sizeof(temp),
            "  \"rtt_stats\": {\n"
            "    \"min_ms\": %.3f,\n"
            "    \"avg_ms\": %.3f,\n"
            "    \"max_ms\": %.3f,\n"
            "    \"stddev_ms\": %.3f\n"
            "  },\n",
            sd.min, avg_rtt, sd.max, stddev_rtt);
    } else {
        snprintf(temp, sizeof(temp),
            "  \"rtt_stats\": null,\n");
    }
    strncat(buffer, temp, bufsize - strlen(buffer) - 1);
    
    /* timestamp和version */
    snprintf(temp, sizeof(temp),
        "  \"timestamp\": \"%s\",\n"
        "  \"version\": \"1.0\"\n"
        "}\n",
        timestamp);
    strncat(buffer, temp, bufsize - strlen(buffer) - 1);
}

/* 输出JSON结果 */
void output_json_results(void) {
    char json_buffer[4096];
    
    build_json_output(json_buffer, sizeof(json_buffer));
    
    if (json_output_file) {
        /* 输出到文件 */
        FILE *fp = fopen(json_output_file, "w");
        if (fp) {
            fprintf(fp, "%s", json_buffer);
            fclose(fp);
        } else {
            fprintf(stderr, "错误: 无法写入JSON文件 %s\n", json_output_file);
            printf("%s", json_buffer);  /* 回退到标准输出 */
        }
    } else {
        /* 输出到标准输出 */
        printf("%s", json_buffer);
    }
}

/* 获取日志时间戳 */
void get_log_timestamp(char *buffer, size_t size) {
    struct timeval tv;
    struct tm *tm_info;
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    /* 日志时间戳格式: [YYYY-MM-DD HH:MM:SS.mmm] */
    if (size >= 30) {  /* 确保缓冲区足够大 */
        snprintf(buffer, size, "[%04d-%02d-%02d %02d:%02d:%02d.%03ld]",
                 tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                 tv.tv_usec / 1000);
    }
}

/* 初始化日志系统 */
int init_log_system(const char *log_path) {
    if (!log_path) {
        return -1;
    }
    
    log_fp = fopen(log_path, "a");  /* 以追加模式打开 */
    if (!log_fp) {
        return -1;
    }
    
    /* 设置行缓冲模式，确保立即写入 */
    setvbuf(log_fp, NULL, _IOLBF, 0);
    
    return 0;
}

/* 通用日志写入函数 */
void write_log(const char *level, const char *format, ...) {
    if (!log_output || !log_fp) {
        return;
    }
    
    char timestamp[32];
    get_log_timestamp(timestamp, sizeof(timestamp));
    
    /* 写入时间戳和级别 */
    fprintf(log_fp, "%s %s ", timestamp, level);
    
    /* 写入格式化消息 */
    va_list args;
    va_start(args, format);
    vfprintf(log_fp, format, args);
    va_end(args);
    
    fprintf(log_fp, "\n");
    fflush(log_fp);  /* 确保立即将数据刷新到文件 */
}

/* 便捷日志函数 */
void log_info(const char *format, ...) {
    if (!log_output) return;
    
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log("INFO", "%s", buffer);
}

void log_ping(const char *format, ...) {
    if (!log_output) return;
    
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log("PING", "%s", buffer);
}

void log_timeout(const char *format, ...) {
    if (!log_output) return;
    
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log("TIMEOUT", "%s", buffer);
}

void log_error(const char *format, ...) {
    if (!log_output) return;
    
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log("ERROR", "%s", buffer);
}

void log_debug(const char *format, ...) {
    if (!log_output || !verbose) return;  /* 只在详细模式下记录调试信息 */
    
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log("DEBUG", "%s", buffer);
}

void log_stats(const char *format, ...) {
    if (!log_output) return;
    
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    write_log("STATS", "%s", buffer);
}

/* 清理日志系统 */
void cleanup_log_system(void) {
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
}

/********************add**************** */