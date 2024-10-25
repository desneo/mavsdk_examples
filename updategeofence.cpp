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

  if (argc != 3) {
    PLOGE.printf("使用规范: ./updategeofence url filename.plane");
    return -1;
  }

  string url(argv[1]);
  string planfile(argv[2]);
  PLOGI.printf("url=%s, planfile=%s", url.c_str(), planfile.c_str());

  // 解析qgc导出的航线文件
  std::ifstream f(argv[2]);
  json data = json::parse(f);
  json points = data["geoFence"]["polygons"]["polygon"];
  int len = points.size();
  PLOGI.printf("地理围栏中点数:%d", len);
  vector<Geofence::Point> pts;
  for (json::iterator iterator = points.begin(); iterator != points.end();
       ++iterator) {
    Geofence::Point p;
    p.latitude_deg = (*iterator)[0];
    p.longitude_deg = (*iterator)[1];
    pts.push_back(p);
  }

  Geofence::Polygon pg;
  pg.points = pts;
  Geofence::GeofenceData ge;
  ge.polygons.push_back(pg);

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

  // 设置电子围栏
  std::cout << "设置电子围栏..." << std::endl;
  auto geofence_result = geofence.upload_geofence(ge);
  if (geofence_result != Geofence::Result::Success) {
    PLOGE.printf("设置电子围栏失败:%d", geofence_result);
    return 1;
  }

  std::cout << "电子围栏设置成功。" << std::endl;

  return 0;
}
