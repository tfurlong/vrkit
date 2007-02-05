// Copyright (C) Infiscape Corporation 2005-2007

#ifndef _INF_GRAB_SIGNAL_DATA_H_
#define _INF_GRAB_SIGNAL_DATA_H_

#include <IOV/Config.h>

#include <vector>
#include <boost/signal.hpp>

#include <gmtl/Point.h>

#include <vpr/Util/GUID.h>

#include <IOV/SceneData.h>
#include <IOV/Event.h>
#include <IOV/SceneObjectPtr.h>
#include <IOV/GrabSignalDataPtr.h>


namespace inf
{

/**
 * Scene data used for centralized access to signals emitted when various
 * grab events occur. Currently, these signals are emitted by a strategy
 * plug-in (inf::SignalGrabStrategy) used by the Grab Plug-in.
 *
 * @since 0.30.3
 */
class IOV_CLASS_API GrabSignalData
   : public inf::SceneData
{
protected:
   GrabSignalData();

public:
   /**
    * The unique type identifier for scene data used by plug-ins that manage
    * grabbing of objects in the scene.
    */
   static const vpr::GUID type_guid;

   static GrabSignalDataPtr create()
   {
      return GrabSignalDataPtr(new GrabSignalData());
   }

   virtual ~GrabSignalData();

   /** @name Signals */
   //@{
   /** Signal type for choosing objects to grab based on user input. */
   typedef boost::signal<Event::ResultType (), Event::ResultOperator>
      choose_action_t;

   /** Signal type for grabbing chosen objects based on user input. */
   typedef boost::signal<
      Event::ResultType (std::vector<SceneObjectPtr>&, gmtl::Point3f&),
      Event::ResultOperator
   > grab_action_t;

   /** Signal type for releasing grabbed objects based on user input. */
   typedef boost::signal<
      Event::ResultType (std::vector<SceneObjectPtr>&), Event::ResultOperator
   > release_action_t;

   /**
    * Signal type for releasing grabbed objects that may or may not be the
    * result of user input.
    *
    * @since 0.31.0
    *
    * @see asyncRelease
    */
   typedef boost::signal<void (const std::vector<SceneObjectPtr>&)>
      async_release_action_t;

   /**
    * Signal emitted when the user chooses an object to add to the
    * collection of objects to be grabbed later.
    */
   choose_action_t choose;

   /**
    * Signal emitted when the user grabs one or more objects. The slot(s)
    * receiving this signal populate the given vector reference with all the
    * objects that are grabbed as a result of this signal being emitted.
    */
   grab_action_t grab;

   /**
    * Signal emitted when the user releases one or more of the grabbed
    * objects. The slot(s) receiving this signal populate the given vector
    * reference with all the objects that are grabbed as a result of this
    * signal being emitted.
    */
   release_action_t release;

   /**
    * Signal emitted when a specific set of grabbed objects are released
    * without the release action being performed. The idea here is to provide
    * a means for the code that would normally receive grab and release
    * signals to be able to communicate in the reverse direction when it
    * determines on its own that one or more objects must be released.
    *
    * An example of why this is needed shows up when handling objects that
    * become ungrabbable. If a grabbed object becomes ungrabbable, a reasonable
    * thing to do is to release it. However, this release operation probably
    * occurs without the usual release input from the user. Morever, it is
    * quite conceivable that multiple objects could be grabbed and only one of
    * that set would become ungrabbable. As such, it should not be the case
    * that all objects are released simply because one becomes ungrabbable.
    *
    * @since 0.31.0
    */
   async_release_action_t asyncRelease;
   //@}
};

}


#endif /* _INF_GRAB_SIGNAL_DATA_H_ */
