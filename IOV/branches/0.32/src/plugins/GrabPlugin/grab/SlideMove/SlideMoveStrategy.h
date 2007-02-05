// Copyright (C) Infiscape Corporation 2005-2007

#ifndef _INF_SLIDE_MOVE_STRATEGY_H_
#define _INF_SLIDE_MOVE_STRATEGY_H_

#include <string>
#include <boost/enable_shared_from_this.hpp>

#include <jccl/Config/ConfigElementPtr.h>

#include <IOV/Grab/MoveStrategy.h>


namespace inf
{

class SlideMoveStrategy
   : public inf::MoveStrategy
   , public boost::enable_shared_from_this<SlideMoveStrategy>
{
protected:
   SlideMoveStrategy();

public:
   static std::string getId()
   {
      return "SlideMove";
   }

   static inf::MoveStrategyPtr create()
   {
      return inf::MoveStrategyPtr(new SlideMoveStrategy());
   }

   virtual ~SlideMoveStrategy()
   {
      /* Do nothing. */ ;
   }

   virtual inf::MoveStrategyPtr init(inf::ViewerPtr viewer);

   virtual void objectsGrabbed(inf::ViewerPtr viewer,
                               const std::vector<SceneObjectPtr>& obj,
                               const gmtl::Point3f& intersectPoint,
                               const gmtl::Matrix44f& vp_M_wand);

   virtual void objectsReleased(inf::ViewerPtr viewer,
                                const std::vector<SceneObjectPtr>& obj);

   // Note: Use the curObjPos instead of obj->getMatrix()
   virtual gmtl::Matrix44f computeMove(inf::ViewerPtr viewer,
                                       SceneObjectPtr obj,
                                       const gmtl::Matrix44f& vp_M_wand,
                                       const gmtl::Matrix44f& curObjPos);

   /**
    * Invokes the global scope delete operator.  This is required for proper
    * releasing of memory in DLLs on Windows.
    */
   void operator delete(void* p)
   {
      ::operator delete(p);
   }

protected:
   /**
    * Deletes this object.  This is an implementation of the pure virtual
    * inf::MoveStrategy::destroy() method.
    */
   virtual void destroy()
   {
      delete this;
   }

private:
   static std::string getElementType()
   {
      return "slide_move_strategy";
   }

   /**
    * Configures this move strategy.
    *
    * @pre The type of the given config element matches the identifier
    *      returned by getElementType().
    *
    * @param cfgElt The config element to use for configuring this object.
    *
    * @throw inf::PluginException is thrown if the version of the given
    *        config element is too old.
    */
   void configure(jccl::ConfigElementPtr cfgElt);

   float mTransValue;
   gmtl::Point3f mIntersectPoint;

   /** @name Slide Behavior Properties */
   //@{
   enum SlideTarget
   {
      ISECT_POINT = 0,
      WAND_CENTER
   };

   SlideTarget  mSlideTarget;   /**< */
   int          mAnalogNum;     /**< The wand interface analog index */
   float        mForwardValue;  /**< Value for foroward sliing (0 or 1) */
   float        mSlideEpsilon;  /**< The slide start threshold */
   //@}
};

}


#endif /* _INF_BASIC_MOVE_STRATEGY_H_ */
