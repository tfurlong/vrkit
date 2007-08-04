// Copyright (C) Infiscape Corporation 2005-2007

#ifndef _INF_STATUS_PANEL_H_
#define _INF_STATUS_PANEL_H_

#include <IOV/Config.h>

#include <vector>
#include <string>
#include <deque>
#include <map>
#include <boost/signal.hpp>

#include <IOV/Util/SignalProxy.h>


namespace inf
{

class StatusPanel
{
public:
   StatusPanel();
   
   typedef std::map<std::string, std::vector<std::string> > center_text_t;

   /** Sets the header title. */
   void setHeaderTitle(const std::string& txt);
   
   /** Sets the center title. */
   void setCenterTitle(const std::string& txt);
   
   /** Sets the bottom title. */
   void setBottomTitle(const std::string& txt);

   /** The text for the header. */
   void setHeaderText(const std::string& header);

   /**
    * Sets the text description for the command identified by \p cmds.
    * This overwrites whatever was previously in the buffer for the
    * identified command.
    *
    * @since 0.10.0
    */
   void setControlText(const std::string& cmdText, const std::string& desc);

   /**
    * Adds the given text description ifor the command identified by \p cmds
    * to this panel's control section.
    *
    * @since 0.10.0
    */
   void addControlText(const std::string& cmds, const std::string& desc,
                       const unsigned int priority = 1);

   /**
    * Removes the given text description for the command identified by \cmds
    * from this panel's control section.  If the given text is not associated
    * with the identified command, this has no effect.
    *
    * @since 0.10.0
    */
   void removeControlText(const std::string& cmdText,
                          const std::string& desc);

   /**
    * Determines if the named line contains the given text.
    *
    * @since 0.10.0
    */
   void hasControlText(const std::string& cmdText, const std::string& desc,
                       bool& flag)
   {
      flag = false;

      if ( mCenterText.count(cmdText) != 0 )
      {
         std::vector<std::string>& vec = mCenterText[cmdText];
         flag = std::find(vec.begin(), vec.end(), desc) != vec.end();
      }

      mChangeSignal();
   }

   /** Add another message to the status panel. */
   void addStatusMessage(const std::string& msg);
   
   /** Gets the header title. */
   const std::string& getHeaderTitle()
   {
      return mHeaderTitle;
   }
   
   /** Gets the center title. */
   const std::string& getCenterTitle()
   {
      return mCenterTitle;
   }
   
   /** Gets the bottom title. */
   const std::string& getBottomTitle()
   {
      return mBottomTitle;
   }
   
   /** Gets the header text. */
   const std::string& getHeaderText()
   {
      return mHeaderText;
   }
   
   /** Gets the center text. */
   const center_text_t& getCenterText()
   {
      return mCenterText;
   }
   
   /** Gets the status lines. */
   const std::deque<std::string> getStatusLines()
   {
      return mStatusLines;
   }
   
   typedef boost::signal<void ()> signal_t;
   
   inf::SignalProxy<signal_t> statusPanelChanged()
   {
      return inf::SignalProxy<signal_t>(mChangeSignal);
   }

public:  // Configuration params //

   /**
    * Sets the size of the buffer for the status history.  This
    * affects the number of status lines displayed.
    *
    * @since 0.7.2
    */
   void setStatusHistorySize(const unsigned int size);

protected:
   
   std::string    mHeaderTitle;     /**< Header title */
   std::string    mCenterTitle;     /**< Center title */
   std::string    mBottomTitle;     /**< Bottom title */

   std::string    mHeaderText;      /**< Header text */

   center_text_t mCenterText;       /**< Center text */
   std::deque<std::string> mStatusLines;  /**< Status lines */
   unsigned int mStatusHistorySize; /**< Number of status lines to keep around. */
   
   signal_t mChangeSignal;          /**< Change signal */
};

}


#endif
