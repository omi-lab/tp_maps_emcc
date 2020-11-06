#include "tp_maps_emcc/Map.h"

#include "tp_maps/MouseEvent.h"

#include "tp_utils/DebugUtils.h"
#include "tp_utils/TimeUtils.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace tp_maps_emcc
{
struct Map::Private
{
  Map* q;

  bool error{false};
  EmscriptenWebGLContextAttributes attributes;
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context{0};
  std::string canvasID;

  glm::ivec2 mousePos{0,0};
  bool pointerLock{false};
  bool usePointerLock{false};

  bool updateRequested{true};

  bool isDownLeftButton {false};
  bool isDownRightButton{false};

  enum class TouchMode_lt
  {
    New,
    Pan,
    ZoomRotate,
    Invalid
  };

  TouchMode_lt touchMode{TouchMode_lt::New};
  glm::ivec2 touchStartPos{0,0};

  glm::vec2 zoomRotateAPos{0.0f,0.0f};
  glm::vec2 zoomRotateBPos{0.0f,0.0f};
  float zoomRotateDist{0.0f};

  int64_t firstPress{0};
  int64_t secondPress{0};

  //################################################################################################
  Private(Map* q_, std::string canvasID_):
    q(q_),
    canvasID(canvasID_)
  {

  }

  //################################################################################################
  void initializeWebGL2()
  {
    tpWarning() << "Trying WebGL 2.0";

    attributes.alpha                           = EM_FALSE;
    attributes.depth                           = EM_TRUE;
    attributes.stencil                         = EM_TRUE;
    attributes.antialias                       = EM_TRUE;
    attributes.premultipliedAlpha              = EM_TRUE;
    attributes.preserveDrawingBuffer           = EM_FALSE;
    attributes.failIfMajorPerformanceCaveat    = EM_FALSE;
    attributes.majorVersion                    = 2;
    attributes.minorVersion                    = 0;
    attributes.enableExtensionsByDefault       = EM_TRUE;

    context = emscripten_webgl_create_context(canvasID.c_str(), &attributes);

    if(context!=0)
      q->setOpenGLProfile(tp_maps::OpenGLProfile::VERSION_300_ES);
  }

  //################################################################################################
  void initializeWebGL1()
  {
    tpWarning() << "Trying WebGL 1.0";

    attributes.alpha                           = EM_FALSE;
    attributes.depth                           = EM_TRUE;
    attributes.stencil                         = EM_TRUE;
    attributes.antialias                       = EM_TRUE;
    attributes.premultipliedAlpha              = EM_TRUE;
    attributes.preserveDrawingBuffer           = EM_FALSE;
    attributes.failIfMajorPerformanceCaveat    = EM_FALSE;
    attributes.majorVersion                    = 1;
    attributes.minorVersion                    = 0;
    attributes.enableExtensionsByDefault       = EM_TRUE;

    context = emscripten_webgl_create_context(canvasID.c_str(), &attributes);

    if(context!=0)
      q->setOpenGLProfile(tp_maps::OpenGLProfile::VERSION_100_ES);
  }


  //################################################################################################
  void update()
  {
    q->makeCurrent();
    q->paintGL();
  }

  //################################################################################################
  void invalidateDoubleTap()
  {
    firstPress  = 0;
    secondPress = 0;
  }

  //################################################################################################
  static EM_BOOL mouseCallback(int eventType, const EmscriptenMouseEvent* event, void *userData)
  {
    Private* d = static_cast<Private*>(userData);

    switch(eventType)
    {
    case EMSCRIPTEN_EVENT_CLICK: //-----------------------------------------------------------------
    {
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEDOWN: //-------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Press);
      d->mousePos = glm::ivec2(event->targetX, event->targetY);
      e.pos = d->mousePos;

      if(d->usePointerLock)
      {
        emscripten_request_pointerlock(d->canvasID.data(), false);
        d->pointerLock = true;
      }
      //0 : Left button
      //1 : Middle button (if present)
      //2 : Right button
      switch (event->button)
      {
      case 0:  e.button = tp_maps::Button::LeftButton;  d->isDownLeftButton  = true; break;
      case 2:  e.button = tp_maps::Button::RightButton; d->isDownRightButton = true; break;
      default: e.button = tp_maps::Button::NoButton; break;
      }
      d->q->mouseEvent(e);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP: //---------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Release);
      d->mousePos = glm::ivec2(event->targetX, event->targetY);
      e.pos = d->mousePos;

      if(d->usePointerLock)
      {
        emscripten_exit_pointerlock();
        d->pointerLock = false;
      }

      //0 : Left button
      //1 : Middle button (if present)
      //2 : Right button
      switch (event->button)
      {
      case 0:  e.button = tp_maps::Button::LeftButton;  d->isDownLeftButton  = false;  break;
      case 2:  e.button = tp_maps::Button::RightButton; d->isDownRightButton = false;  break;
      default: e.button = tp_maps::Button::NoButton; break;
      }
      d->q->mouseEvent(e);
      break;
    }
    case EMSCRIPTEN_EVENT_DBLCLICK: //--------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::DoubleClick);
      //d->mousePos = glm::ivec2(event->targetX, event->targetY);
      e.pos = d->mousePos;

      //0 : Left button
      //1 : Middle button (if present)
      //2 : Right button
      switch (event->button)
      {
      case 0:  e.button = tp_maps::Button::LeftButton;  break;
      case 2:  e.button = tp_maps::Button::RightButton; break;
      default: e.button = tp_maps::Button::NoButton;    break;
      }
      d->q->mouseEvent(e);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE: //-------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Move);

      if(d->pointerLock)
      {
        EmscriptenPointerlockChangeEvent pointerlockStatus;
        if(emscripten_get_pointerlock_status(&pointerlockStatus)==EMSCRIPTEN_RESULT_SUCCESS &&
           pointerlockStatus.isActive == EM_TRUE)
        {
          d->mousePos += glm::ivec2(tpBound(-10, int(event->movementX), 10),
                                    tpBound(-10, int(event->movementY), 10));
        }
      }
      else
        d->mousePos = glm::ivec2(event->targetX, event->targetY);

      e.pos = d->mousePos;
      d->q->mouseEvent(e);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEENTER: //------------------------------------------------------------
    {
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSELEAVE: //------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Release);
      d->mousePos = glm::ivec2(event->targetX, event->targetY);
      e.pos = d->mousePos;

      if(d->usePointerLock)
      {
        emscripten_exit_pointerlock();
        d->pointerLock = false;
      }

      if(d->isDownLeftButton  == true)
      {
        d->isDownLeftButton = false;
        e.button = tp_maps::Button::LeftButton;
        d->q->mouseEvent(e);
      }

      if(d->isDownRightButton == true)
      {
        d->isDownRightButton = false;
        e.button = tp_maps::Button::RightButton;
        d->q->mouseEvent(e);
      }

      break;
    }

    default: //-------------------------------------------------------------------------------------
    {
      break;
    }
    }
    return EM_TRUE;
  }

  //################################################################################################
  static EM_BOOL wheelCallback(int eventType, const EmscriptenWheelEvent* event, void* userData)
  {
    Private* d = static_cast<Private*>(userData);

    switch(eventType)
    {
    case EMSCRIPTEN_EVENT_WHEEL: //-----------------------------------------------------------------
    {
      tp_maps::MouseEvent e(tp_maps::MouseEventType::Wheel);
      e.pos = d->mousePos;
      e.delta = -event->deltaY;
      d->q->mouseEvent(e);
      break;
    }
    default:
    {
      break;
    }
    }
    return EM_TRUE;
  }

  //################################################################################################
  static EM_BOOL touchCallback(int eventType, const EmscriptenTouchEvent* touchEvent, void* userData)
  {
    Private* d = static_cast<Private*>(userData);

    switch(eventType)
    {
    case EMSCRIPTEN_EVENT_TOUCHSTART: //------------------------------------------------------------
    {
      if(touchEvent->numTouches == 1)
      {
        d->touchMode = TouchMode_lt::New;
        const EmscriptenTouchPoint* event = &(touchEvent->touches[0]);
        d->mousePos = glm::ivec2(event->targetX, event->targetY);
        d->touchStartPos = d->mousePos;

        d->firstPress = d->secondPress;
        d->secondPress = tp_utils::currentTimeMS();
      }
      else if(touchEvent->numTouches == 2)
      {
        if(d->touchMode == TouchMode_lt::Pan)
        {
          tp_maps::MouseEvent e(tp_maps::MouseEventType::Release);
          e.pos = d->mousePos;
          e.button = tp_maps::Button::LeftButton;
          d->q->mouseEvent(e);
        }

        d->touchMode = TouchMode_lt::ZoomRotate;
        d->invalidateDoubleTap();

        d->zoomRotateAPos = glm::vec2(touchEvent->touches[0].targetX, touchEvent->touches[0].targetY);
        d->zoomRotateBPos = glm::vec2(touchEvent->touches[1].targetX, touchEvent->touches[1].targetY);
        d->zoomRotateDist = glm::length(d->zoomRotateAPos - d->zoomRotateBPos);

        glm::vec2 m = d->zoomRotateBPos + ((d->zoomRotateAPos-d->zoomRotateBPos) / 2.0f);
        d->mousePos = glm::ivec2(m.x, m.y);
      }
      else
      {
        d->touchMode = TouchMode_lt::Invalid;
        d->invalidateDoubleTap();
      }

      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHEND: //--------------------------------------------------------------
    {
      if(touchEvent->numTouches == 1)
      {
        if(d->touchMode == TouchMode_lt::Pan)
        {
          const EmscriptenTouchPoint* event = &(touchEvent->touches[0]);
          tp_maps::MouseEvent e(tp_maps::MouseEventType::Release);
          d->mousePos = glm::ivec2(event->targetX, event->targetY);
          e.pos = d->mousePos;
          e.button = tp_maps::Button::LeftButton;
          d->q->mouseEvent(e);
        }
        else
        {
          if((tp_utils::currentTimeMS() - d->firstPress) < 400)
          {
            tp_maps::MouseEvent e(tp_maps::MouseEventType::DoubleClick);
            e.pos = d->mousePos;
            e.button = tp_maps::Button::LeftButton;
            d->q->mouseEvent(e);
          }
          else if(d->touchMode == TouchMode_lt::New)
          {
            if((tp_utils::currentTimeMS() - d->secondPress) < 400)
            {
              {
                tp_maps::MouseEvent e(tp_maps::MouseEventType::Press);
                e.pos = d->touchStartPos;
                e.button = tp_maps::Button::LeftButton;
                d->q->mouseEvent(e);
              }

              {
                const EmscriptenTouchPoint* event = &(touchEvent->touches[0]);
                tp_maps::MouseEvent e(tp_maps::MouseEventType::Release);
                d->mousePos = glm::ivec2(event->targetX, event->targetY);
                e.pos = d->mousePos;
                e.button = tp_maps::Button::LeftButton;
                d->q->mouseEvent(e);
              }
            }
          }
        }
      }
      else
      {
      }

      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHMOVE: //-------------------------------------------------------------
    {
      if(touchEvent->numTouches == 1)
      {
        if(d->touchMode == TouchMode_lt::Pan || d->touchMode == TouchMode_lt::New)
        {
          const EmscriptenTouchPoint* event = &(touchEvent->touches[0]);
          d->mousePos = glm::ivec2(event->targetX, event->targetY);

          if(d->touchMode == TouchMode_lt::Pan)
          {
            tp_maps::MouseEvent e(tp_maps::MouseEventType::Move);
            e.pos = d->mousePos;
            d->q->mouseEvent(e);
          }
          else
          {
            int ox = abs(d->touchStartPos.x - d->mousePos.x);
            int oy = abs(d->touchStartPos.y - d->mousePos.y);
            if((ox+oy) > 10)
            {
              d->touchMode = TouchMode_lt::Pan;
              tp_maps::MouseEvent e(tp_maps::MouseEventType::Press);
              e.pos = d->touchStartPos;
              e.button = tp_maps::Button::LeftButton;
              d->q->mouseEvent(e);
              d->invalidateDoubleTap();
            }
          }
        }
      }
      else if(touchEvent->numTouches == 2)
      {
        if(d->touchMode == TouchMode_lt::New)
          d->touchMode = TouchMode_lt::ZoomRotate;

        if(d->touchMode == TouchMode_lt::ZoomRotate)
        {
          glm::vec2 zoomRotateAPos = glm::vec2(touchEvent->touches[0].targetX, touchEvent->touches[0].targetY);
          glm::vec2 zoomRotateBPos = glm::vec2(touchEvent->touches[1].targetX, touchEvent->touches[1].targetY);
          float zoomRotateDist = glm::length(zoomRotateAPos - zoomRotateBPos);

          tp_maps::MouseEvent e(tp_maps::MouseEventType::Wheel);
          e.delta = (zoomRotateDist - d->zoomRotateDist) / 10.0f;
          if(abs(e.delta)>1)
          {
            e.pos = d->mousePos;
            d->q->mouseEvent(e);

            d->zoomRotateAPos = zoomRotateAPos;
            d->zoomRotateBPos = zoomRotateBPos;
            d->zoomRotateDist = zoomRotateDist;
          }
        }
      }
      break;
    }
    case EMSCRIPTEN_EVENT_TOUCHCANCEL:
    {
      break;
    }
    }

    return EM_TRUE;
  }
};

//##################################################################################################
Map::Map(const char* canvasID, bool enableDepthBuffer):
  tp_maps::Map(enableDepthBuffer),
  d(new Private(this, canvasID))
{
  emscripten_webgl_init_context_attributes(&d->attributes);

  // Try to use WebGL 2.0 first
  d->initializeWebGL2();

  // If WebGL 2.0 fails try WebGL 1.0
  if(d->context == 0)
    d->initializeWebGL1();

  if(d->context == 0)
  {
    d->error = true;
    tpWarning() << "Failed to get OpenGL context for: " << canvasID;
    return;
  }

  for(auto callback : {
      emscripten_set_click_callback_on_thread     ,
      emscripten_set_mousedown_callback_on_thread ,
      emscripten_set_mouseup_callback_on_thread   ,
      emscripten_set_dblclick_callback_on_thread  ,
      emscripten_set_mousemove_callback_on_thread ,
      emscripten_set_mouseenter_callback_on_thread,
      emscripten_set_mouseleave_callback_on_thread})
  {
    if(callback(canvasID,
                d,
                EM_TRUE,
                Private::mouseCallback,
                EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD) != EMSCRIPTEN_RESULT_SUCCESS)
    {
      d->error = true;
      tpWarning() << "Failed to install mouse callback for: " << canvasID;
      return;
    }
  }

  if(emscripten_set_wheel_callback(canvasID,
                                   d,
                                   EM_TRUE,
                                   Private::wheelCallback) != EMSCRIPTEN_RESULT_SUCCESS)
  {
    d->error = true;
    tpWarning() << "Failed to install wheel callback for: " << canvasID;
    return;
  }

  for(auto callback : {
      emscripten_set_touchstart_callback_on_thread ,
      emscripten_set_touchend_callback_on_thread   ,
      emscripten_set_touchmove_callback_on_thread  ,
      emscripten_set_touchcancel_callback_on_thread})
  {
    if(callback(canvasID,
                d,
                EM_TRUE,
                Private::touchCallback,
                EM_CALLBACK_THREAD_CONTEXT_CALLING_THREAD) != EMSCRIPTEN_RESULT_SUCCESS)
    {
      d->error = true;
      tpWarning() << "Failed to install touch callback for: " << canvasID;
      return;
    }
  }

  initializeGL();

  resize();
}

//##################################################################################################
Map::~Map()
{
  preDelete();
  if(emscripten_webgl_destroy_context(d->context) != EMSCRIPTEN_RESULT_SUCCESS)
    tpWarning() << "Failed to delete context: " << d->context;
  delete d;
}

//##################################################################################################
const std::string& Map::canvasID()const
{
  return d->canvasID;
}

//##################################################################################################
void Map::processEvents()
{
  if(!d->updateRequested)
    return;

  d->update();
  d->updateRequested = false;
}

//##################################################################################################
void Map::makeCurrent()
{
  if(emscripten_webgl_make_context_current(d->context) != EMSCRIPTEN_RESULT_SUCCESS)
  {
    d->error = true;
    tpWarning() << "Failed to make current.";
  }
}

//##################################################################################################
void Map::update()
{
  d->updateRequested = true;
}

//##################################################################################################
void Map::resize()
{
  makeCurrent();
  double width{0};
  double height{0};
  emscripten_get_element_css_size(d->canvasID.data(), &width, &height);

  int w = int(width);
  int h = int(height);

  emscripten_set_canvas_element_size(d->canvasID.data(), w, h);

  resizeGL(w, h);
}

//##################################################################################################
void Map::setUsePointerLock(bool usePointerLock)
{
  d->usePointerLock = usePointerLock;
}

}
