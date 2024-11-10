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

using namespace  std;

enum class ActionResult {
    Unknown,                         /**< @brief Unknown result. */
    Success,                         /**< @brief Request was successful. */
    NoSystem,                        /**< @brief No system is connected. */
    ConnectionError,                 /**< @brief Connection error. */
    Busy,                            /**< @brief Vehicle is busy. */
    CommandDenied,                   /**< @brief Command refused by vehicle. */
    CommandDeniedLandedStateUnknown, /**< @brief Command refused because landed state is
                                        unknown. */
    CommandDeniedNotLanded,          /**< @brief Command refused because vehicle not landed. */
    Timeout,                         /**< @brief Request timed out. */
    VtolTransitionSupportUnknown,    /**< @brief Hybrid/VTOL transition support is unknown. */
    NoVtolTransitionSupport,         /**< @brief Vehicle does not support hybrid/VTOL transitions. */
    ParameterError,                  /**< @brief Error getting or setting parameter. */
    Unsupported,                     /**< @brief Action not supported. */
    Failed,                          /**< @brief Action failed. */
    InvalidArgument,                 /**< @brief Invalid argument. */
};

using ResultCallback = std::function<void(ActionResult)>;

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
    

    action.arm();
    cout<<"after arm";

    action.takeoff_async(const ResultCallback callback)


    sleep_for(seconds(50));
    std::cout << "Finished...\n";

    return 0;
}