#include "tp_maps_emcc/MapManager.h"
#include "tp_maps_emcc/Map.h"

#include "tp_utils/DebugUtils.h"
#include "tp_utils/TimeUtils.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace tp_maps_emcc
{

//##################################################################################################
MapDetails::MapDetails(Map* map_):
  map(map_)
{

}

//##################################################################################################
MapDetails::~MapDetails()
{
  delete map;
}

//##################################################################################################
struct MapManager::Private
{
  std::function<MapDetails*(Map*)> createMapDetails;
  std::vector<MapDetails*> maps;

  //################################################################################################
  Private(std::function<MapDetails*(Map*)> createMapDetails_):
    createMapDetails(createMapDetails_)
  {

  }

  //################################################################################################
  static void mainLoop(void* opaque)
  {
    Private* d = reinterpret_cast<Private*>(opaque);
    for(MapDetails* details : d->maps)
    {
      details->map->animate(tp_utils::currentTimeMS());
      details->map->processEvents();
    }
  }

  //################################################################################################
  static EM_BOOL resizeCallback(int eventType, const EmscriptenUiEvent* uiEvent, void* opaque)
  {
    TP_UNUSED(uiEvent);
    Private* d = reinterpret_cast<Private*>(opaque);

    switch(eventType)
    {
    case EMSCRIPTEN_EVENT_RESIZE:
    {
      for(MapDetails* details : d->maps)
        details->map->resize();
      break;
    }
    default:
    {
      break;
    }
    }
    return EM_FALSE;
  }
};

//##################################################################################################
MapManager::MapManager(std::function<MapDetails*(Map*)> createMapDetails):
  d(new Private(createMapDetails))
{
  if(emscripten_set_resize_callback(0,
                                    d,
                                    EM_TRUE,
                                    Private::resizeCallback) != EMSCRIPTEN_RESULT_SUCCESS)
    tpWarning() << "Failed to install resize callback.";
}

//##################################################################################################
MapManager::~MapManager()
{
  delete d;
}

//##################################################################################################
void MapManager::exec()
{
  emscripten_set_main_loop_arg(Private::mainLoop, d, 0, 1);
}

//##################################################################################################
void* MapManager::createMap(char* canvasID)
{
  tp_maps_emcc::MapDetails* details = d->createMapDetails(new tp_maps_emcc::Map(canvasID));
  d->maps.push_back(details);
  return details;
}

//##################################################################################################
void MapManager::destroyMap(void* handle)
{
  if(handle)
  {
    MapDetails* details = (MapDetails*)handle;
    tp_utils::removeOne(d->maps, details);
    delete details;
  }
}

}
