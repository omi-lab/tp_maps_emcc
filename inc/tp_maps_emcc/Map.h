#ifndef tp_maps_emcc_Map_h
#define tp_maps_emcc_Map_h

#include "tp_maps_emcc/Globals.h"

#include "tp_maps/Map.h"

namespace tp_maps_emcc
{

//##################################################################################################
class TP_MAPS_EMCC_SHARED_EXPORT Map : public tp_maps::Map
{
public:
  //################################################################################################
  Map(const char* canvasID, bool enableDepthBuffer = true);

  //################################################################################################
  virtual ~Map();

  //################################################################################################
  void processEvents();

  //################################################################################################
  void makeCurrent()override;

  //################################################################################################
  //! Called to queue a refresh
  void update()override;

  //################################################################################################
  //! Call this when the window is resize (this will become protected shortly)
  void resize();

  //################################################################################################
  void setUsePointerLock(bool usePointerLock);

private:
  struct Private;
  Private* d;
  friend struct Private;
};
}
#endif
