#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "plog/Log.h"
#include <chrono>
#include <future>
#include <iostream>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/follow_me/follow_me.h>
#include <mavsdk/plugins/log_files/log_files.h>
#include <mavsdk/plugins/param/param.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <string>
#include <thread>

using namespace mavsdk;

using std::string;
using std::chrono::seconds;

// ./downloadfclog serial:///dev/ttyS4:115200 600
int main(int argc, char *argv[]) {
  plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);
  PLOGI.printf("开始运行.");

  if (argc != 3) {
    PLOGE.printf(
        "使用规范: ./updateparam serial:///dev/ttyS4:115200 downtimeout");
    return -1;
  }

  string url(argv[1]);
  int dttimeout = stoi(string(argv[2]));

  PLOGI.printf("url=%s,日志下载超时时间=%d", url.c_str(), dttimeout);

  Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
  ConnectionResult connection_result = mavsdk.add_any_connection(url);

  mavsdk.set_timeout_s(2);

  if (connection_result != ConnectionResult::Success) {
    PLOGE.printf("Connection failed:%s", url.c_str());
    return -1;
  }

  auto system = mavsdk.first_autopilot(10.0);
  if (!system) {
    PLOGE.printf("Timed out waiting for system.");
    return -1;
  }

  // Instantiate plugins.
  auto telemetry = Telemetry{system.value()};
  auto action = Action{system.value()};
  auto param = Param{system.value()};
  auto log_files = LogFiles{system.value()};

  // 请求日志列表
  auto loglist_promise = std::promise<void>{};
  auto loglist_future = loglist_promise.get_future();
  std::vector<LogFiles::Entry> loglist;
  log_files.get_entries_async(
      [&loglist_promise, &loglist](LogFiles::Result result,
                                   std::vector<LogFiles::Entry> entries) {
        if (result == LogFiles::Result::Success) {
          PLOGI.printf("成功获取日志列表，日志数量: %d", entries.size());
          loglist = entries;
          loglist_promise.set_value();

        } else {
          PLOGI.printf("获取日志列表失败:%d", static_cast<int>(result));
          exit(-1);
        }
      });

  if (loglist_future.wait_for(seconds(10)) == std::future_status::timeout) {
    PLOGE.printf("获取日志列表超时.");
    return -1;
  }

  if (loglist.size() == 0) {
    PLOGE.printf("无日志，退出.");
    return -1;
  }

  // 下载最后一个(最新的)日志
  LogFiles::Entry log_entry = *(loglist.end() - 1);
  PLOGI.printf("待下载日志 date:%s,大小:%d", log_entry.date.c_str(),
               log_entry.size_bytes);

  auto download_promise = std::promise<void>{};
  auto download_future = download_promise.get_future();
  string logname = "downloaded_log_" + std::to_string(log_entry.id) + "_" +
                   log_entry.date + ".ulog";
  log_files.download_log_file_async(
      log_entry, logname,
      [&download_promise](LogFiles::Result download_result,
                          LogFiles::ProgressData process) {
        PLOGI.printf("下载进度:%f", process.progress);
        if (download_result == LogFiles::Result::Success) {
          PLOGI.printf("日志下载成功!");
          download_promise.set_value();
        } else {
          PLOGE.printf("日志下载失败:%d", static_cast<int>(download_result));
          exit(-1);
        }
      });

  if (download_future.wait_for(seconds(dttimeout)) ==
      std::future_status::timeout) {
    PLOGE.printf("获取日志列表超时.");
    return -1;
  }

  return 0;
}
