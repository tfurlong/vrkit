#ifndef USER_H
#define USER_H

#include <OpenSG/VRJ/Viewer/IOV/UserPtr.h>
#include <OpenSG/VRJ/Viewer/IOV/InterfaceTrader.h>
#include <OpenSG/VRJ/Viewer/IOV/ViewPlatform.h>
#include <OpenSG/VRJ/Viewer/IOV/ViewerPtr.h>

#include <boost/enable_shared_from_this.hpp>

namespace inf
{

/**
 * This class holds all information related to a user of the application.
 * This includes devices they are using and their position in the virtual world.
 *
 * @see Scene for definition of the coordinate frames used.
 */
class User : public boost::enable_shared_from_this<User>
{
public:
   static UserPtr create();

   void init()
   {;}

   /** Update user and user associated information.
    * @post: User and viewplatform are updated.
    */
   void update(ViewerPtr viewer);

   InterfaceTrader& getInterfaceTrader()
   {
      return mInterfaceTrader;
   }

   /** Return the view platform that we are using. */
   ViewPlatform& getViewPlatform()
   {
      return mViewPlatform;
   }

protected:
   User()
   {;}

private:
   /** Devices abstraction for the user. */
   InterfaceTrader    mInterfaceTrader;

   ViewPlatform   mViewPlatform;    /** The user's view platform. */
};

}
#endif