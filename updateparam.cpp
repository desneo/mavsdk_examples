#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "plog/Log.h"
#include <chrono>
#include <iostream>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/follow_me/follow_me.h>
#include <mavsdk/plugins/param/param.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <string>
#include <thread>

using namespace mavsdk;

using std::string;
using std::chrono::seconds;

// ./updateparam serial:///dev/ttyS4:115200 paramkey [paramvalue]
int main(int argc, char *argv[]) {
  plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);
  PLOGI.printf("开始运行.");

  if (argc < 3) {
    PLOGE.printf("使用规范: ./updateparam serial:///dev/ttyS4:115200 paramkey "
                 "[paramvalue]");
    PLOGE.printf(" --paramvalue可选, 如果未传递, 则只查询不更新.");

    return -1;
  }

  string url(argv[1]);
  string paramKey(argv[2]);

  string valueStr;
  float paramValue = -1;
  if (argc > 3) {
    valueStr = string(argv[3]);
    paramValue = std::stof(string(argv[3]));
  }

  PLOGI.printf("url=%s,paramKey=%s,paramValue=%s", url.c_str(),
               paramKey.c_str(), valueStr.c_str());

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

  // 读取参数值以确认修改
  float oldValue;
  auto param_value = param.get_param_float(paramKey);
  if (param_value.first == Param::Result::Success) {
    PLOGI.printf("原始参数值,%s=%f", paramKey.c_str(), param_value.second);
  } else {
    PLOGI.printf("查询参数原始值失败:%s", paramKey.c_str());
    return 1;
  }

  oldValue = param_value.second;
  if (valueStr.empty()) {
    return 0;
  }

  // 发送参数修改请求
  Param::Result param_set_result = param.set_param_float(paramKey, paramValue);
  if (param_set_result != Param::Result::Success) {
    PLOGE.printf("设置参数命令下发失败:%d", param_set_result);
    return 1;
  }

  PLOGI.printf("参数设置指令下发成功.");

  // 读取参数值以确认修改
  param_value = param.get_param_float(paramKey);
  if (param_value.first == Param::Result::Success &&
      param_value.second == paramValue) {
    PLOGI.printf("参数已修改成功,%s:%f-->%f", paramKey.c_str(), oldValue,
                 param_value.second);
  } else {
    PLOGI.printf("确认参数修改失败,当前值是:%f", param_value.second);
    return 1;
  }

  return 0;
}
