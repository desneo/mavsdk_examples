#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/follow_me/follow_me.h>
#include <mavsdk/plugins/param/param.h>
#include <mavsdk/plugins/telemetry/telemetry.h>

#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "plog/Log.h"

using namespace mavsdk;

using std::map;
using std::string;
using std::chrono::seconds;

enum class ParamValueType { FLAOT, INT };

map<string, ParamValueType> paramTypeMap = {
    {"MIS_DIST_1WP", ParamValueType::FLAOT},    {"MPC_Z_VEL_MAX_DN", ParamValueType::FLAOT},
    {"MPC_Z_V_AUTO_DN", ParamValueType::FLAOT}, {"MPC_Z_V_AUTO_UP", ParamValueType::FLAOT},
    {"MPC_XY_CRUISE", ParamValueType::FLAOT},   {"MPC_XY_VEL_MAX", ParamValueType::FLAOT},

    {"COM_OBL_ACT", ParamValueType::INT},

};

// ./updateparam serial:///dev/ttyS4:115200 paramkey [paramvalue]
int main(int argc, char *argv[]) {
    plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);
    PLOGI.printf("开始运行.");

    if (argc < 3) {
        PLOGE.printf(
            "使用规范: ./updateparam serial:///dev/ttyS4:115200 paramkey "
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

    PLOGI.printf("url=%s,paramKey=%s,paramValue=%s", url.c_str(), paramKey.c_str(), valueStr.c_str());

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

    if (paramTypeMap.find(paramKey) == paramTypeMap.end()) {
        PLOGE.printf("不支持的key值.:%s", paramKey.c_str());
        return -1;
    }

    ParamValueType valueType = paramTypeMap.at(paramKey);

    if (valueType == ParamValueType::FLAOT) {
        // 读取参数值以确认修改
        PLOGI.printf("浮点类型参数....");
        float oldValue;
        auto param_value = param.get_param_float(paramKey);
        if (param_value.first == Param::Result::Success) {
            PLOGI.printf("原始参数值,%s=%f", paramKey.c_str(), param_value.second);
            return 0;
        }
        PLOGI.printf("查询参数原始值失败:%s", paramKey.c_str());
        return -1;
    }

    PLOGI.printf("int类型参数....");
    // 读取参数值以确认修改
    int oldValue;
    auto param_value = param.get_param_int(paramKey);
    if (param_value.first == Param::Result::Success) {
        PLOGI.printf("原始参数值,%s=%d", paramKey.c_str(), param_value.second);
        return 0;
    }
    PLOGI.printf("查询参数原始值失败:%s", paramKey.c_str());
    return -1;
}
