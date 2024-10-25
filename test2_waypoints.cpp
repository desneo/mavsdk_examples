//
// Simple example to demonstrate how takeoff and land using MAVSDK.
//

#include <chrono>
#include <cstdint>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/mission/mission.h>
#include <iostream>
#include <future>
#include <memory>
#include <thread>
#include "plog/Log.h"
#include "plog/Init.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Appenders/RollingFileAppender.h"
#include "plog/Appenders/ConsoleAppender.h"
#include <map>

using namespace mavsdk;
using std::chrono::seconds;
using std::this_thread::sleep_for;
using namespace mavsdk;
using namespace std::chrono;
using std::string;

void usage(const std::string& bin_name)
{
    std::cerr << "Usage : " << bin_name << " <connection_url>\n"
              << "Connection URL format should be :\n"
              << " For TCP : tcp://[server_host][:server_port]\n"
              << " For UDP : udp://[bind_host][:bind_port]\n"
              << " For Serial : serial:///path/to/serial/dev[:baudrate]\n"
              << "For example, to connect to the simulator use URL: udp://:14540\n";
}


void clearMission(Mission &mission){
// 开始前先清除之前残留的航线信息
    PLOGI.printf("开始清除航线动作");
    auto clear_promise = std::promise<void>{};
    auto clear_future = clear_promise.get_future();

    Action::Result actionRst;
    Mission::Result missionRst;
    // 终止航点飞行
    mission.clear_mission_async([&clear_promise, &missionRst](Mission::Result result) {
        missionRst = result;
        if (result != Mission::Result::Success) {
            PLOGE.printf("清除航线任务失败.");
        }
        clear_promise.set_value();
    });
    clear_future.wait_for(seconds(5));
    if (clear_future.wait_for(seconds(2)) == std::future_status::timeout) {
        PLOGE.printf("向飞控下发清除航线指令超时.");
        return ;
    }
    if (missionRst != Mission::Result::Success) {
        return ;
    }
    PLOGI.printf("向飞控下发清除航线指令成功.");
}

static inline string missionRstToStr(Mission::Result missionRst){
    static const std::map<Mission::Result, std::string> Names = {
            {Mission::Result::Unknown, "Unknown"},
            {Mission::Result::Success, "Success"},
            {Mission::Result::Error, "Error"},
            {Mission::Result::TooManyMissionItems, "TooManyMissionItems"},
            {Mission::Result::Busy, "Busy"},
            {Mission::Result::Timeout, "Timeout"},
            {Mission::Result::InvalidArgument, "InvalidArgument"},
            {Mission::Result::Unsupported, "Unsupported"},
            {Mission::Result::NoMissionAvailable, "NoMissionAvailable"},
            {Mission::Result::UnsupportedMissionCmd, "UnsupportedMissionCmd"},
            {Mission::Result::TransferCancelled, "TransferCancelled"},
            {Mission::Result::NoSystem, "NoSystem"},
            {Mission::Result::Next, "Next"},
            {Mission::Result::Denied, "Denied"},
            {Mission::Result::ProtocolError, "ProtocolError"},
            {Mission::Result::IntMessagesNotSupported, "IntMessagesNotSupported"},
    };
    auto it = Names.find(missionRst);
    if (it != Names.end()) {
        return it->second;
    }

    PLOGW.printf("非可识别的missionRst值,请检查,missionRst=%d", missionRst);
    return "非可识别的missionRst值.";
}

void upload_mission(Mission &mission, double curLon, double curLat, double height){
        Action::Result actionRst;
        Mission::Result missionRst;
    Mission::MissionPlan missionPlan;
    //根据飞机当前位置生成20个航点
    for (int i=0; i < 50; i++) {
        Mission::MissionItem item1;
        item1.latitude_deg = curLat + 0.0005;
        item1.longitude_deg = curLon;
        // 因飞机要错开高度，所以航线飞行都保持自己的当前高度
        item1.relative_altitude_m = height;
        item1.speed_m_s = 5.0;
        item1.is_fly_through = true;
        missionPlan.mission_items.push_back(item1);

        Mission::MissionItem item2;
        item2.latitude_deg = curLat ;
        item2.longitude_deg = curLon + 0.005;
        // 因飞机要错开高度，所以航线飞行都保持自己的当前高度
        item2.relative_altitude_m = height;
        item2.speed_m_s = 5.0;
        item2.is_fly_through = true;
        missionPlan.mission_items.push_back(item2);
        
    }


    auto in_promise = std::promise<void>{};
    auto in_future = in_promise.get_future();
    mission.upload_mission_async(missionPlan,
                                    [&in_promise, &missionRst](Mission::Result tmp) {
                                        missionRst = tmp;
                                        if (tmp != Mission::Result::Success) {
                                            string ss = missionRstToStr(tmp);
                                            PLOGE.printf("向飞控上传航点失败:%s", ss.c_str());
                                        }
                                        in_promise.set_value();
                                    });

    in_future.wait_for(seconds(30));
    if (in_future.wait_for(seconds(10)) == std::future_status::timeout) {
        PLOGE.printf("向飞控上传航点超时.");
        return;
    }

    if (missionRst != Mission::Result::Success) {
        return ;
    }

    PLOGI.printf("向飞控上传航线成功.");

}

int main(int argc, char** argv)
{
    // if (argc != 2) {
    //     usage(argv[0]);
    //     return 1;
    // }
    plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::debug, &consoleAppender);
    PLOGI.printf("初始化日志模块%d, %s", 42, "test");

    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
    // ConnectionResult connection_result = mavsdk.add_any_connection(argv[1]);
    ConnectionResult connection_result = mavsdk.add_any_connection("udp://192.168.10.4:14550");
    // ConnectionResult connection_result = mavsdk.add_any_connection("udp://192.168.1.41:4196");

    mavsdk.set_timeout_s(5);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << connection_result << '\n';
        return 1;
    }

    auto system = mavsdk.first_autopilot(10.0);
    if (!system) {
        std::cerr << "Timed out waiting for system\n";
        return 1;
    }

    // Instantiate plugins.
    auto telemetry = Telemetry{system.value()};
    auto mission = Mission{system.value()};
    auto action = Action{system.value()};

    // We want to listen to the altitude of the drone at 1 Hz.
    // const auto set_rate_result = telemetry.set_rate_position(40);
    // if (set_rate_result != Telemetry::Result::Success) {
    //     std::cerr << "Setting rate failed: " << set_rate_result << '\n';
    //     return 1;
    // }

    // 当前位置
    Telemetry::Position g_position;
    telemetry.subscribe_position([&g_position](Telemetry::Position position) {
        // PLOGI.printf("经纬度变化:%12.8f, %12.8f, %f, %f", position.longitude_deg, position.latitude_deg,
                    // position.absolute_altitude_m, position.relative_altitude_m);
        g_position = position;
    });
    
    sleep_for(seconds(5));
    int index = 0;
    while(true){
        printf("第%d次执行\n", index);
        clearMission(mission);
        sleep(1);
        upload_mission(mission, g_position.latitude_deg, g_position.longitude_deg, g_position.relative_altitude_m);
        index++;
    }

    sleep_for(seconds(50));
    std::cout << "Finished...\n";

    return 0;
}