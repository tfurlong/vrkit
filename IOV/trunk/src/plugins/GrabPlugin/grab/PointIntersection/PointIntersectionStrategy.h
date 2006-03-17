// Copyright (C) Infiscape Corporation 2005-2006

#ifndef _INF_POINT_INTERSECTION_STRATEGY_H_
#define _INF_POINT_INTERSECTION_STRATEGY_H_

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <OpenSG/OSGTransform.h>
#include <gmtl/Point.h>
#include <IOV/ViewerPtr.h>
#include <IOV/GrabDataPtr.h>
#include <IOV/Grab/IntersectionStrategy.h>

namespace inf
{

class PointIntersectionStrategy
   : public IntersectionStrategy
   , public boost::enable_shared_from_this<PointIntersectionStrategy>
{
protected:
   PointIntersectionStrategy()
      : inf::IntersectionStrategy()
   {;}

public:
   virtual ~PointIntersectionStrategy()
   {;}

   static std::string getId()
   {
      return "PointIntersection";
   }

   static IntersectionStrategyPtr create()
   {
      return IntersectionStrategyPtr(new PointIntersectionStrategy());
   }
   virtual void init(ViewerPtr viewer);

   virtual OSG::TransformNodePtr findIntersection(ViewerPtr viewer, gmtl::Point3f& intersectPoint);

private:
   inf::GrabDataPtr mGrabData;
};

}
#endif /*_INF_POINT_INTERSECTION_STRATEGY_H_*/