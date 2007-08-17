// Copyright (C) Infiscape Corporation 2005-2007

#ifdef IOV_WITH_DIRECT_SHOW

#ifndef _INF_DIRECT_SHOW_ENCODER_H_
#define _INF_DIRECT_SHOW_ENCODER_H_

#include <IOV/Config.h>

#include <atlbase.h>
#include <strmif.h>     /* For IBaseFilter */
#include <control.h>    /* For IMediaControl */
#include <string>
#include <vector>
#include <vpr/vprTypes.h>

#include <IOV/Video/Encoder.h>

// Register the graph so that we can look at it in graphedt.
#define REGISTER_GRAPH

namespace inf
{

class ByteSource;
class ByteStream;

class DirectShowEncoder : public Encoder
{
public:
   typedef std::vector< CComPtr<IMoniker> > moniker_list_t;
protected:
   DirectShowEncoder();

public:
   static EncoderPtr create();

   /**
    * Destructor closes the movie file and flushes all the frames
    */
   ~DirectShowEncoder();

   /**
    * Takes care of creating the memory, streams, compression options etc. required for the movie
    */
   virtual EncoderPtr init();

   virtual vpr::Uint32 width() const
   {
      return mWidth;
   }

   virtual vpr::Uint32 height() const
   {
      return mHeight;
   }

   /** @name Encoding interface. */
   //@{
   virtual void startEncoding();

   /**
    * Takes care of releasing the memory and movie related handles
    */
   virtual void stopEncoding();

   /**
    * Inserts the given bitmap bits into the movie as a new Frame at the end.
    * The width, height and bitsPerPixel are the width, height and bits per pixel
    * of the bitmap pointed to by the input pBits.
    */
   virtual void writeFrame(vpr::Uint8* data);

   virtual OSG::Image::PixelFormat getPixelFormat()
   {
      return OSG::Image::OSG_BGR_PF;
   }
   //@}

   /**
    * Return a list of valid codecs.
    */
   static codec_list_t getCodecs();

   static std::string getName()
   {
      return "DirectShow Encoder";
   }

   static void printVideoEncoders();

protected:
   static moniker_list_t getVideoMonikers();
   static CComPtr<IBaseFilter> getEncoder(const std::string name);
   static void printPins(CComPtr<IBaseFilter> filter);

   void setAviOptions(IBaseFilter *ppf, InterleavingMode INTERLEAVE_MODE);

private:
   vpr::Uint32          mWidth;
   vpr::Uint32          mHeight;

   CComPtr<IGraphBuilder> mGraphBuilder;
   CComPtr<IMediaControl> mMediaController;
   ByteSource* mByteSource;
   ByteStream* mByteStream;
#ifdef REGISTER_GRAPH
   DWORD mGraphRegister;
#endif
};

}

#endif /*_INF_DIRECT_SHOW_ENCODER_H_*/

#endif /*IOV_WITH_DIRECT_SHOW*/