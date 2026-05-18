#pragma once

namespace leon_log {

enum LogLevel_e : int;

// 日志相关的设置
struct LogCfg_t {
	/* 用什么KEY值去配置文件中读取选项
		如:"OPT_PASS"就代表用该字符串常量为KEY去配置文件中找对应的配置项(登录密码).
		定义为常量的目的是全系统统一命名,以免改名的时候到处去找 */
	static constexpr char OPT_DIR_PATH[] = "dir-path";
	static constexpr char OPT_WITH_CPU[] = "with-cpu";
	static constexpr char OPT_LOG_LEVL[] = "level";
	static constexpr char OPT_QUE_CAPA[] = "que-size";
	static constexpr char OPT_INTERVAL[] = "interval";

	str_t		app_name;			// 本项不会存至文件,也不从文件读取
	str_t		dir_path = "/tmp";	// 日志输出路径,不含文件名
	str_t		with_cpu = "-1,";	// 日志线程可运行于哪些cpu, 逗号分隔的cpu_id, -1就不限
	LogLevel_e	log_levl = {};		// 日志级别
	int32_t		que_capa = 1024;	// 日志队列容量
	int32_t		interval = 900;		// 状态信息输出间隔时长,以秒为单位
	int32_t		t_precis = 6;		// 秒以下时戳精度(小数位数)
};

};	// namespace leon_log

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4;
