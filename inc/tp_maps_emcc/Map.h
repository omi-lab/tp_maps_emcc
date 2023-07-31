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
  const std::string& canvasID()const;

  //################################################################################################
  void processEvents();

  //################################################################################################
  void makeCurrent() override;

  //################################################################################################
  //! Called to queue a refresh
  void update(tp_maps::RenderFromStage renderFromStage=tp_maps::RenderFromStage::Full) override;

  //################################################################################################
  void callAsync(const std::function<void()>& callback) override;

  //################################################################################################
  float pixelScale() const override;

  //################################################################################################
  //! Call this when the window is resized (this will become protected shortly)
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
