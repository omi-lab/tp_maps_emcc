#include "tp_maps_emcc/MapManager.h"
#include "tp_maps_emcc/Map.h"

#include "tp_utils/DebugUtils.h"
#include "tp_utils/TimeUtils.h"

#ifdef TP_ENABLE_MUTEX_TIME
#include "tp_utils/MutexUtils.h"
#endif

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
  MapManager* q;
  std::function<MapDetails*(Map*)> createMapDetails;
  std::vector<MapDetails*> maps;

  int64_t animateInterval{50};
  int64_t nextAnimate{tp_utils::currentTimeMS()+animateInterval};

#ifdef TP_ENABLE_MUTEX_TIME
  int64_t nextSaveMutexStats{tp_utils::currentTimeMS()+60000};
#endif

  //################################################################################################
  Private(MapManager* q_, const std::function<MapDetails*(Map*)>& createMapDetails_):
    q(q_),
    createMapDetails(createMapDetails_)
  {

  }

  //################################################################################################
  void animate()
  {
    if(auto t=tp_utils::currentTimeMS(); t>nextAnimate)
    {
      nextAnimate = t+animateInterval;
      q->animateCallbacks(t);
      for(MapDetails* details : maps)
        details->map->animate(t);
    }
  }

  //################################################################################################
  void processEvents()
  {

    for(MapDetails* details : maps)
      details->map->processEvents();
  }

  //################################################################################################
  void printMutexStats()
  {
#ifdef TP_ENABLE_MUTEX_TIME
      if(auto t=tp_utils::currentTimeMS(); t>nextSaveMutexStats)
      {
        d->nextSaveMutexStats = t+60000;
        tpWarning() << tp_utils::LockStats::takeResults();
      }
#endif
  }

  //################################################################################################
  static void mainLoop(void* opaque)
  {
    Private* d = reinterpret_cast<Private*>(opaque);

    d->animate();
    d->processEvents();
    d->printMutexStats();
  }

  //################################################################################################
  static void slowTimer(void* opaque)
  {
    emscripten_async_call(slowTimer, opaque, 5000);

    Private* d = reinterpret_cast<Private*>(opaque);

    d->animate();
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
  d(new Private(this, createMapDetails))
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
  emscripten_async_call(Private::slowTimer, d, 5000);
  emscripten_set_main_loop_arg(Private::mainLoop, d, 0, 1);
}

//##################################################################################################
void* MapManager::createMap(const char* canvasID)
{
  tp_maps_emcc::MapDetails* details = d->createMapDetails(new tp_maps_emcc::Map(canvasID, false));
  d->maps.push_back(details);
  return details;
}

//##################################################################################################
void MapManager::destroyMap(void* handle)
{
  if(handle)
  {
    MapDetails* details = (MapDetails*)handle;
    tpRemoveOne(d->maps, details);
    delete details;
  }
}

}
