// Copyright (C) Infiscape Corporation 2005-2007

#include <OpenSG/OSGFrustumVolume.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGGeoPropPtrs.h>
#include <OpenSG/OSGImage.h>
#include <OpenSG/OSGMergeGraphOp.h>
#include <OpenSG/OSGGraphOpSeq.h>
#include <OpenSG/OSGSimpleGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGSolidBackground.h>

#include <vrj/Display/Projection.h>

#include <IOV/Video/VideoGrabber.h>
#include <IOV/Video/FboVideoCamera.h>

namespace inf
{

FboVideoCamera::FboVideoCamera()
   : mFboVP(OSG::NullFC)
   , mVideoGrabber()
   , mTransform(OSG::NullFC)
   , mLeftTexture(OSG::NullFC)
   , mRightTexture(OSG::NullFC)
   , mFboCam(OSG::NullFC)
   , mBorderSize(2.0)
   , mFrameDist(100.0)
   , mStereo(false)
{;}

FboVideoCameraPtr FboVideoCamera::create()
{
   return FboVideoCameraPtr(new FboVideoCamera);
}

FboVideoCamera::~FboVideoCamera()
{
}

void FboVideoCamera::contextInit(OSG::WindowPtr window)
{
   mFboVP->setParent(window);
}

FboVideoCameraPtr FboVideoCamera::init(const OSG::UInt32 width, const OSG::UInt32 height,
				       const OSG::Real32 fov, const OSG::Real32 borderSize,
				       const OSG::Real32 frameDist)
{
   mBorderSize = borderSize;
   mFrameDist = frameDist;

   // Create a transform to contain the location and orientation of the camera.
   mTransform = OSG::Transform::create();

   OSG::NodePtr beacon = OSG::Node::create();
   OSG::beginEditCP(beacon);
      beacon->setCore(mTransform);
   OSG::endEditCP(beacon);

   // Create the frame root.
   mFrameRoot = OSG::Node::create();
   OSG::beginEditCP(mFrameRoot);
      mFrameRoot->setCore(mTransform);
   OSG::endEditCP(mFrameRoot);

   // Create the FBO textures.
   mLeftTexture = OSG::TextureChunk::create();
   OSG::beginEditCP(mLeftTexture);
      mLeftTexture->setEnvMode(GL_MODULATE);
   OSG::endEditCP(mLeftTexture);

   mRightTexture = OSG::TextureChunk::create();
   OSG::beginEditCP(mRightTexture);
      mRightTexture->setEnvMode(GL_MODULATE);
   OSG::endEditCP(mRightTexture);

   // setup camera
   mFboCam = OSG::PerspectiveCamera::create();
   OSG::beginEditCP(mFboCam);
      // If fov is in degrees, convert to radians first.
      if (fov > OSG::Pi)
      {
         mFboCam->setFov(OSG::osgdegree2rad(fov));
      }
      else
      {
         mFboCam->setFov(fov);
      }
      mFboCam->setNear(0.01);
      mFboCam->setFar(10000);
      mFboCam->setBeacon(beacon);
   OSG::endEditCP(mFboCam);

   // Setup FBO textures.
   OSG::beginEditCP(mLeftTexture);
      mLeftTexture->setMinFilter(GL_LINEAR);
      mLeftTexture->setMagFilter(GL_LINEAR);
      mLeftTexture->setTarget(GL_TEXTURE_2D);
      mLeftTexture->setInternalFormat(GL_RGBA8);
   OSG::endEditCP(mLeftTexture);

   OSG::beginEditCP(mRightTexture);
      mRightTexture->setMinFilter(GL_LINEAR);
      mRightTexture->setMagFilter(GL_LINEAR);
      mRightTexture->setTarget(GL_TEXTURE_2D);
      mRightTexture->setInternalFormat(GL_RGBA8);
   OSG::endEditCP(mRightTexture);

   
   OSG::SolidBackgroundPtr bg = OSG::SolidBackground::create();
   OSG::beginEditCP(bg);
      bg->setColor(OSG::Color3f(0, 0, 0));
   OSG::endEditCP(bg);

   // create FBOViewport
   mFboVP = OSG::FBOViewport::create();
   OSG::beginEditCP(mFboVP);
      mFboVP->setBackground(bg);
      mFboVP->setCamera(mFboCam);
      mFboVP->getTextures().push_back(mLeftTexture);
   OSG::endEditCP(mFboVP);

   // Set the correct size of FBO.
   // This also generates the frame geometry around the captured scene.
   setSize(width, height);

   mVideoGrabber = VideoGrabber::create()->init(mFboVP);

   return shared_from_this();
}

void FboVideoCamera::setSize(const OSG::UInt32 width, const OSG::UInt32 height)
{
   // Resize the viewport.
   OSG::beginEditCP(mFboVP);
      mFboVP->setSize(0, 0, width - 1, height - 1);
      mFboVP->setStorageWidth(width);
      mFboVP->setStorageHeight(height);
   OSG::endEditCP(mFboVP);

   // Resize the debug texture.
   OSG::ImagePtr left_img = OSG::Image::create();
   OSG::beginEditCP(left_img);
      left_img->set(OSG::Image::OSG_RGBA_PF, width, height);
   OSG::endEditCP(left_img);
   OSG::beginEditCP(mLeftTexture);
      mLeftTexture->setImage(left_img);
   OSG::endEditCP(mLeftTexture);

   OSG::ImagePtr right_img = OSG::Image::create();
   OSG::beginEditCP(right_img);
      right_img->set(OSG::Image::OSG_RGBA_PF, width, height);
   OSG::endEditCP(right_img);
   OSG::beginEditCP(mRightTexture);
      mRightTexture->setImage(right_img);
   OSG::endEditCP(mRightTexture);

   // We already compensated aspect ratio with the texture/fbo sizes
   mFboCam->setAspect(width/height);
   generateFrame();
}

void FboVideoCamera::setCameraPos(const OSG::Matrix camPos)
{
   OSG::beginEditCP(mTransform, OSG::Transform::MatrixFieldMask);
      mTransform->setMatrix(camPos);
   OSG::endEditCP(mTransform, OSG::Transform::MatrixFieldMask);
}

void FboVideoCamera::record(const std::string& filename, const std::string& codec,
                            const OSG::UInt32 framesPerSecond, const bool stereo)
{
   mStereo = stereo;
   mVideoGrabber->record(filename, codec, framesPerSecond, stereo);
}

void FboVideoCamera::pause()
{
   mVideoGrabber->pause();
}

void FboVideoCamera::resume()
{
   mVideoGrabber->resume();
}

void FboVideoCamera::stop()
{
   mVideoGrabber->stop();
}

void FboVideoCamera::render(OSG::RenderAction* ra, const OSG::Matrix camPos,
			   const OSG::Real32 interocular)
{
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glPushMatrix();
   /* XXX: Disable rendering if we are not recording. For right now
    *      render anyway so that we can see the debug panels.
   if (!mVideoGrabber->isRecording())
   {
      return;
   }
   */

   // If we are rendering stereo, then offset the camera position.
   if (mStereo)
   {
      // Distance to move eye
      float eye_offset = interocular/2.0f;
      // XXX: Exagerate to debug code.
      eye_offset *= 10;

      OSG::Matrix offset;
      OSG::Matrix camera_pos;

      // Render left eye.
      OSG::beginEditCP(mFboVP);
        mFboVP->getTextures()[0] = mLeftTexture;
      OSG::endEditCP(mFboVP);
      
      camera_pos = camPos;
      offset.setTranslate(-eye_offset, 0.0f, 0.0f);
      camera_pos.multLeft(offset);
      setCameraPos(camera_pos);

      // Do the actual rendering.
      //glClear(GL_DEPTH_BUFFER_BIT);
      mFboVP->render(ra);

      // XXX: Only capturing right eye right now.
      mFboVP->bind(ra->getWindow());
      mVideoGrabber->grabFrame(true);
      mFboVP->stop(ra->getWindow());

      // Render right eye.
      OSG::beginEditCP(mFboVP);
         mFboVP->getTextures()[0] = mRightTexture;
      OSG::endEditCP(mFboVP);

      // Set the correct camera position.
      camera_pos = camPos;
      offset.setTranslate(eye_offset, 0.0f, 0.0f);
      camera_pos.multLeft(offset);
      setCameraPos(camera_pos);

      // Do the actual rendering.
      //glClear(GL_DEPTH_BUFFER_BIT);
      mFboVP->render(ra);

      // XXX: Only capturing right eye right now.
      mFboVP->bind(ra->getWindow());
      mVideoGrabber->grabFrame(false);
      mFboVP->stop(ra->getWindow());
   }
   else
   {
      OSG::beginEditCP(mFboVP);
         mFboVP->getTextures()[0] = mLeftTexture;
      OSG::endEditCP(mFboVP);

      setCameraPos(camPos);

      // Do the actual rendering.
      //glClear(GL_DEPTH_BUFFER_BIT);
      mFboVP->render(ra);

      mFboVP->bind(ra->getWindow());
      mVideoGrabber->grabFrame();
      mFboVP->stop(ra->getWindow());
   }

   mVideoGrabber->writeFrame();
   glPopMatrix();
   glPopAttrib();
}

OSG::NodePtr FboVideoCamera::getDebugPlane() const
{
   // Create material for left eye.
   OSG::SimpleMaterialPtr left_mat = OSG::SimpleMaterial::create();
   OSG::beginEditCP(left_mat);
      left_mat->addChunk(mLeftTexture);
      left_mat->setSpecular(OSG::Color3f(0.7f, 0.7f, 0.7f));
      left_mat->setDiffuse(OSG::Color3f(0.22f, 0.2f, 0.2f));
   OSG::endEditCP(left_mat);

   // Create material for right eye.
   OSG::SimpleMaterialPtr right_mat = OSG::SimpleMaterial::create();
   OSG::beginEditCP(right_mat);
      right_mat->addChunk(mRightTexture);
      right_mat->setSpecular(OSG::Color3f(0.7f, 0.7f, 0.7f));
      right_mat->setDiffuse(OSG::Color3f(0.22f, 0.2f, 0.2f));
   OSG::endEditCP(right_mat);

   // Create geometry for left eye.
   OSG::GeometryPtr left_geom = OSG::makePlaneGeo(5, 5, 2, 2);
   OSG::NodePtr left_node = OSG::Node::create();
   OSG::beginEditCP(left_geom);
      left_geom->setMaterial(left_mat);
   OSG::endEditCP(left_geom);
   OSG::beginEditCP(left_node);
      left_node->setCore(left_geom);
   OSG::endEditCP(left_node);

   // Create geometry for right eye.
   OSG::GeometryPtr right_geom = OSG::makePlaneGeo(5, 5, 2, 2);
   OSG::NodePtr right_node = OSG::Node::create();
   OSG::beginEditCP(right_geom);
      right_geom->setMaterial(right_mat);
   OSG::endEditCP(right_geom);
   OSG::beginEditCP(right_node);
      right_node->setCore(right_geom);
   OSG::endEditCP(right_node);

   // Create the xforms for each eye's geometry.
   OSG::Matrix leftm, rightm;
   leftm.setTranslate(-2.5, 2.5, 0.0);
   rightm.setTranslate(2.5, 2.5, 0.0);

   // Create the transform nodes for each eye's geometry.
   OSG::TransformNodePtr left_xform = OSG::TransformNodePtr::create();
   OSG::TransformNodePtr right_xform = OSG::TransformNodePtr::create();
   OSG::beginEditCP(left_xform);
   OSG::beginEditCP(right_xform);
      left_xform->setMatrix(leftm);
      right_xform->setMatrix(rightm);
      left_xform.node()->addChild(left_node);
      right_xform.node()->addChild(right_node);
   OSG::endEditCP(left_xform);
   OSG::endEditCP(right_xform);

   /* XXX: Fro some reason this causes a crash.
   OSG::GroupNodePtr group = OSG::GroupNodePtr::create();
   OSG::beginEditCP(group);
      group.node()->addChild(left_xform.node());
      group.node()->addChild(right_xform.node());
   OSG::endEditCP(group);
   return group.node();
   */

   OSG::GroupPtr group = OSG::Group::create();
   OSG::NodePtr group_node = OSG::Node::create();
   OSG::beginEditCP(group_node);
      group_node->setCore(group);
      group_node->addChild(left_xform.node());
      group_node->addChild(right_xform.node());
   OSG::endEditCP(group_node);
   return group_node;
}

// XXX: This has not been updated to behave correctly in stereo mode.
void FboVideoCamera::generateFrame()
{
   // The size of the internal frame.
   OSG::Real32 frame_height = 2.0 * (OSG::osgtan(mFboCam->getFov()/2.0) * mFrameDist);
   OSG::Real32 frame_width = frame_height * mFboCam->getAspect();

   // =============
   // |           |
   // |           |
   // |           |
   // |           |
   // =============

   // Top and bottom lines need to be long enough to go past left and
   // right sides.
   OSG::Real32 real_frame_width = frame_width + (2.0*mBorderSize);

   // Create the geometry nodes.
   OSG::GeometryNodePtr top(OSG::makeBox(real_frame_width, mBorderSize, mBorderSize, 2, 2, 2));
   OSG::GeometryNodePtr bottom(OSG::makeBox(real_frame_width, mBorderSize, mBorderSize, 2, 2, 2));
   OSG::GeometryNodePtr left(OSG::makeBox(mBorderSize, frame_height, mBorderSize, 2, 2, 2));
   OSG::GeometryNodePtr right(OSG::makeBox(mBorderSize, frame_height, mBorderSize, 2, 2, 2));

   OSG::TransformNodePtr top_xform = OSG::TransformNodePtr::create();
   OSG::TransformNodePtr bottom_xform = OSG::TransformNodePtr::create();
   OSG::TransformNodePtr left_xform = OSG::TransformNodePtr::create();
   OSG::TransformNodePtr right_xform = OSG::TransformNodePtr::create();

   // Create material for the frame.
   OSG::SimpleMaterialPtr mat = OSG::SimpleMaterial::create();
   OSG::beginEditCP(mat);
      //mat->setLit(false);
      //mat->setDiffuse(OSG::Color3f(1.0f, 0.0f, 0.0f));
      //mat->setAmbient(OSG::Color3f(0.2f, 0.2f, 0.2f));
      mat->setDiffuse(OSG::Color3f(0.8f, 0.0f, 0.0f));
      //mat->setSpecular(OSG::Color3f(0.2f, 0.2f, 0.2f));
      //mat->setShininess(2);
   OSG::endEditCP(mat);

   // Set the material for all parts of the frame.
   OSG::beginEditCP(top);
   OSG::beginEditCP(bottom);
   OSG::beginEditCP(left);
   OSG::beginEditCP(right);
      top->setMaterial(mat);
      bottom->setMaterial(mat);
      left->setMaterial(mat);
      right->setMaterial(mat);
   OSG::endEditCP(top);
   OSG::endEditCP(bottom);
   OSG::endEditCP(left);
   OSG::endEditCP(right);

   // We need to pull the frame in half the border size to ensure that we don't
   // see it.
   OSG::Real32 zoffset = -mFrameDist + (mBorderSize/2.0);
   OSG::Real32 xoffset = (frame_width/2.0) + (mBorderSize/2.0);
   OSG::Real32 yoffset = (frame_height/2.0) + (mBorderSize/2.0);

   // Create the xforms for each box.
   OSG::Matrix topm, bottomm, leftm, rightm;
   topm.setTranslate(0.0, yoffset, zoffset);
   bottomm.setTranslate(0.0, -yoffset, zoffset);
   leftm.setTranslate(-xoffset, 0.0, zoffset);
   rightm.setTranslate(xoffset, 0.0, zoffset);


   OSG::beginEditCP(top_xform);
   OSG::beginEditCP(bottom_xform);
   OSG::beginEditCP(left_xform);
   OSG::beginEditCP(right_xform);
      top_xform->setMatrix(topm);
      bottom_xform->setMatrix(bottomm);
      left_xform->setMatrix(leftm);
      right_xform->setMatrix(rightm);

      top_xform.node()->addChild(top.node());
      bottom_xform.node()->addChild(bottom.node());
      left_xform.node()->addChild(left.node());
      right_xform.node()->addChild(right.node());
   OSG::endEditCP(top_xform);
   OSG::endEditCP(bottom_xform);
   OSG::endEditCP(left_xform);
   OSG::endEditCP(right_xform);

   // Create a node that will contain all geometry after the the merge op.
   OSG::NodePtr frame = OSG::Node::create();
   OSG::GroupPtr frame_group = OSG::Group::create();
   OSG::beginEditCP(frame);
      frame->setCore(frame_group);
      frame->addChild(top_xform.node());
      frame->addChild(bottom_xform.node());
      frame->addChild(left_xform.node());
      frame->addChild(right_xform.node());
   OSG::endEditCP(frame);

   // Merge all geometry into one node.
   OSG::MergeGraphOp merge;
   merge.traverse(frame);

   // Remove old geometry and add new.
   OSG::beginEditCP(mFrameRoot);
      while (mFrameRoot->getNChildren() > 0)
      {
         mFrameRoot->subChild(0);
      }
      mFrameRoot->addChild(frame);
   OSG::endEditCP(mFrameRoot);
}

}
