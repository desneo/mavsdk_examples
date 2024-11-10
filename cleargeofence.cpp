#include "mavsdk/mavsdk.h"
#include "mavsdk/plugins/action/action.h"
#include "mavsdk/plugins/geofence/geofence.h"
#include "nlohmann/json.hpp"
#include "plog/Appenders/ConsoleAppender.h"
#include "plog/Formatters/TxtFormatter.h"
#include "plog/Init.h"
#include "plog/Log.h"
#include <chrono>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

using namespace mavsdk;

using std::string;
using std::vector;
using std::chrono::seconds;

// ./updateparam  serial:///dev/ttyS4:115200 1023_zyh_2_0303.plan
int main(int argc, char *argv[]) {
  plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::debug, &consoleAppender);
  PLOGI.printf("开始运行.");

  if (argc < 2) {
    PLOGE.printf("使用规范: ./updategeofence serial:///dev/ttyS4:115200");
    return -1;
  }

  string url(argv[1]);


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
  auto action = Action{system.value()};
  auto geofence = Geofence{system.value()};

  geofence.clear_geofence();

  std::cout << " 清除电子围栏设置成功。" << std::endl;

  return 0;
}
