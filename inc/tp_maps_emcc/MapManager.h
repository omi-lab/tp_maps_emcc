#ifndef tp_maps_emcc_MapManager_h
#define tp_maps_emcc_MapManager_h

#include "tp_maps_emcc/Globals.h"

#include "tp_utils/CallbackCollection.h"

#include <functional>

namespace tp_maps_emcc
{
class Map;

//##################################################################################################
struct MapDetails
{
  Map* map;

  //################################################################################################
  MapDetails(Map* map_);

  //################################################################################################
  virtual ~MapDetails();
};

//##################################################################################################
class TP_MAPS_EMCC_SHARED_EXPORT MapManager
{
public:
  //################################################################################################
  MapManager(const std::function<MapDetails*(Map*)>& createMapDetails);

  //################################################################################################
  virtual ~MapManager();

  //################################################################################################
  void exec();

  //################################################################################################
  void* createMap(const char* canvasID);

  //################################################################################################
  void destroyMap(void* handle);

  //################################################################################################
  tp_utils::CallbackCollection<void(double)> animateCallbacks;

private:
  struct Private;
  Private* d;
  friend struct Private;
};
}
#endif
